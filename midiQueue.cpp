#include "myDebug.h"
#include "midiQueue.h"
#include "defines.h"
#include "prefs.h"
#include "ftp.h"
#include "ftp_defs.h"
#include "myMidiHost.h"


#define MAX_PROCESS_QUEUE   8192
#define MAX_SYSEX_BUFFER    1024
#define MAX_OUTGOING_QUEUE  1024


// display filters

// int  showSysex = 2;
// bool showActiveSense = 0;
// bool showTuningMessages = 1;
// bool showNoteInfoMessages = 1;
//     // Has a hard time running from the timer_handler()
//     // with all of the serial output, FTP barfs often.
//     // Especially with showSysex == 2
// bool showVolumeLevel = 1;
// bool showBatteryLevel = 1;
//     // useful to turn these off while trying to debug other messages
// bool showPerformanceCCs = 1;


// queues

int process_head = 0;
int process_tail = 0;
int outgoing_head = 0;
int outgoing_tail = 0;

uint32_t outgoing_queue[MAX_OUTGOING_QUEUE];
uint32_t process_queue[MAX_PROCESS_QUEUE];


// sysex buffers

int sysex_buflen[NUM_MIDI_PORTS]     = {0,0,0,0,0,0,0,0};
bool sysex_buf_ready[NUM_MIDI_PORTS] = {0,0,0,0,0,0,0,0};
uint8_t sysex_buffer[NUM_MIDI_PORTS][MAX_SYSEX_BUFFER];


// note processing

uint8_t most_recent_note_val = 0;
uint8_t most_recent_note_vel = 0;
    // these values are cached from the most recent NoteOn/NoteOff
    // messages and used to create (or delete) my note_t's upon 1E
    // NoteInfo messages.


// ftp command processing

uint8_t last_command[NUM_MIDI_PORTS]  = {0,0,0,0,0,0,0,0};
    // as we are processing messages, we keep track of the most recent
    // B7 1F "command or reply" (i.e. 07==FTP_CMD_BATTERY_LEVEL),
    // to be able to hook it up to th following "command_or_reply" value
    // message (B7 1F "value") for processing and display purposes.
    //
    // If coming from the designed FTP controller port, this is used to
    // set the state of certain FTP variables (battery level, sensitivy etc)
    // as well as to clear any pending outGoing commands to the controller.


// outgoing command processing

uint32_t pending_command        = 0;        // note that these are the full messages
uint32_t pending_command_value  = 0;
int command_retry_count         = 0;
elapsedMillis command_time      = 0;
    // These four variables are used to implement an asynychronous
    // command and reply conversation with the host.  When we send
    // a command (and value), we save them here, and in processing
    // (if and) when the host replies with the correct values, we
    // clear them, which allows for the next command to be sent.

#define GET_COMMAND_VALUE(w)    ((w)>>24)
    // outgoing pending commands are full 32bit midi messages
    // so we use this to compare them to the incomming_command,
    // which is just a uint8_t


void _processMessage(uint32_t i);
    // forward


//-------------------------------------
// immediate sends as device (cable0)
//-------------------------------------

void mySendDeviceProgramChange(uint8_t prog_num, uint8_t channel)
{
    usbMIDI.sendProgramChange(prog_num, channel);
    msgUnion msg(
        0x0C | PORT_MASK_OUTPUT,
        0xC0 | (channel-1),
        prog_num,
        0);
    enqueueProcess(msg.i);
}

void mySendDeviceControlChange(uint8_t cc_num, uint8_t value, uint8_t channel)
{
    usbMIDI.sendProgramChange(cc_num, value, channel);
    msgUnion msg(
        0x0B | PORT_MASK_OUTPUT,
        0xB0 | (channel-1),
        cc_num,
        value);
    enqueueProcess(msg.i);
}



//-------------------------------------
// outgoing Message Processing
//-------------------------------------

void _enqueueOutgoing(uint32_t msg)
{
    // __disable_irq();
    outgoing_queue[outgoing_head++] = msg;
    if (outgoing_head == MAX_OUTGOING_QUEUE)
        outgoing_head = 0;
    if (outgoing_head == outgoing_tail)
        my_error("FTP.cpp outGoingQueue overflow at %d",outgoing_head);
    // __enable_irq();
}


uint32_t _dequeueOutgoing()
{
    int msg = 0;
    if (outgoing_tail != outgoing_head)
    {
        msg = outgoing_queue[outgoing_tail++];
        if (outgoing_tail == MAX_OUTGOING_QUEUE)
            outgoing_tail = 0;
    }
    return msg;
}


void sendFTPCommandAndValue(uint8_t command, uint8_t value)
{
    if (!FTP_OUTPUT_PORT)
    {
        warning(0,"PREF_FTP_PORT is set to Off in sendFTPCommandAndValue(%02x,%02x)",command,value);
        return;
    }

    display(0,"sendFTPCommandAndValue(%02x,%02x)",command,value);

    msgUnion msg(
        0x1B,
        0xB7,
        FTP_COMMAND_OR_REPLY,    // 0x1f
        command);

    _enqueueOutgoing(msg.i);
    // midi_host.write_packed(msg.i);

    msg.b[2] = FTP_COMMAND_VALUE;       // 0x3f
    msg.b[3] = value;

    _enqueueOutgoing(msg.i);
    // midi_host.write_packed(msg.i);
    // midi_host.flush();
}



void sendPendingCommand()
{
    int ftp_output_port = FTP_OUTPUT_PORT;
    if (ftp_output_port)            // Host or Remote
    {
        if (ftp_output_port == 2)   // Remote
        {
            uint32_t cmd = (pending_command >> 24) & 0xFF;
            uint32_t val = (pending_command_value >> 24) & 0xFF;
            display(0,"SENDING cmd=%02x  val=%02x  TO TEENSYDUINO",cmd,val);
            usbMIDI.sendControlChange(
                0x1F,
                cmd,
                8,
                1);         // cable1 !
            usbMIDI.sendControlChange(
                0x3F,
                val,
                8,
                1);

            enqueueProcess(pending_command | PORT_MASK_OUTPUT);
            enqueueProcess(pending_command_value | PORT_MASK_OUTPUT);
        }
        else        // Host
        {
            midi_host.write_packed(pending_command);
            midi_host.write_packed(pending_command_value);
            enqueueProcess(pending_command | PORT_MASK_HOST | PORT_MASK_OUTPUT);
            enqueueProcess(pending_command_value | PORT_MASK_HOST | PORT_MASK_OUTPUT);
        }
        command_time = 0;
    }
    else
    {
        display(0,"PREF_FTP_PORT is NONE in sendPendingCommand(%d) command(%08x) value(%08x)",command_retry_count,pending_command,pending_command_value);
    }
    command_time = 0;
}



void _processOutgoing()
{
    // see if there's a command to dequue and send
    // dequeue them even if we don't send them

    if (!pending_command)
    {
        pending_command = _dequeueOutgoing();
        if (pending_command)
        {
            pending_command_value = _dequeueOutgoing();
            display(0,"--> sending(%d) command(%08x) value(%08x)",command_retry_count,pending_command,pending_command_value);
            command_retry_count = 0;
            sendPendingCommand();
        }
    }
    else if (command_retry_count > 10)
    {
        my_error("timed out sending command %08x %08x",pending_command,pending_command_value);
        command_retry_count = 0;
        pending_command = 0;
    }
    else if (command_time > 100)    // resend with timer
    {
        command_retry_count++;
        display(0,"--> retry(%d) command(%08x) value(%08x)",command_retry_count,pending_command,pending_command_value);
        sendPendingCommand();
    }
}




//-------------------------------------
// API and utilities
//-------------------------------------

const char *portName(int pindex)
{
    if (pindex==PORT_INDEX_DUINO_INPUT0 ) return "device_in_0 ";
    if (pindex==PORT_INDEX_DUINO_INPUT1 ) return "device_in_1 ";
    if (pindex==PORT_INDEX_DUINO_OUTPUT0) return "device_out_0";
    if (pindex==PORT_INDEX_DUINO_OUTPUT1) return "device_out_1";
    if (pindex==PORT_INDEX_HOST_INPUT0  ) return "host_in_0   ";
    if (pindex==PORT_INDEX_HOST_INPUT1  ) return "host_in_1   ";
    if (pindex==PORT_INDEX_HOST_OUTPUT0 ) return "host_out_0  ";
    if (pindex==PORT_INDEX_HOST_OUTPUT1 ) return "host_out_1  ";
    return "unknown";
}


void enqueueProcess(uint32_t msg)
{
    // display(0,"enqueueProcess(%d,%08x)",display_head,msg);

    __disable_irq();
    process_queue[process_head++] = msg;
    if (process_head == MAX_PROCESS_QUEUE)
        process_head = 0;
    if (process_head == process_tail)
        my_error("expSystem processQueue overflow at %d",process_head);
    __enable_irq();
}


void dequeueProcess()
{
    int msg = 0;
    if (process_tail != process_head)
    {
        msg = process_queue[process_tail++];
        if (process_tail == MAX_PROCESS_QUEUE)
            process_tail = 0;
    }
    if (msg)
        _processMessage(msg);

    _processOutgoing();
}


// patch display

uint8_t patch_sig[6] = {0xF0, 0x00, 0x01, 0x6E, 0x01, 0x21};

bool showPatch(int pindex, uint8_t *buf, uint32_t buflen)
{
    uint8_t *p = buf;
    patch_sig[5] = pindex ? 0x21 : 0x41;
    // if (!pindex) return false;            // looking for things from controller
    if (buflen != 142) return false;      // patches are 42
    for (int i=0; i<6; i++)
        if (*p++ != patch_sig[i])
        {
            warning(0,"sysex of 142 that does not match patch_sig!!",0);
            return true;
        }

    uint8_t bank_num = *p++;
    uint8_t patch_num = *p++;
    display(0,"    PATCH BANK(%d) PATCH(%d)",bank_num,patch_num);
    if (bank_num > 1) return false;

    p += 8;     // move to touch sensitivity
    uint8_t touch_sense = *p;

    p += 2;     // move to first split section
    p += 6;

    uint8_t dyn_sense = *p++;
    uint8_t dyn_off  = *p++;

    display(0,"        touch_sens=%d   dyn_sense=0x%02x  dyn_off=0x%02x",touch_sense,dyn_sense,dyn_off);

    return false;
}



// port classification for _processOMessageg;


bool isFtpPort(int idx)
{
    bool is_spoof = getPref8(PREF_SPOOF_FTP);
    if (is_spoof) return 1;
    uint8_t pref_ftp_port = getPref8(PREF_FTP_PORT);
    if (!pref_ftp_port) return 0;   // Off
    if (pref_ftp_port == 1)         // Host
        return INDEX_IS_HOST(idx);
    return !INDEX_IS_HOST(idx);     // Remote
}


bool isFtpController(int idx)
{
    if (INDEX_IS_OUTPUT(idx)) return false;
    bool is_spoof = getPref8(PREF_SPOOF_FTP);
    if (is_spoof) return INDEX_IS_HOST(idx);
    uint8_t pref_ftp_port = getPref8(PREF_FTP_PORT);
    if (!pref_ftp_port) return 0;   // Off
    if (pref_ftp_port == 1)         // Host
        return INDEX_IS_HOST(idx);
    return !INDEX_IS_HOST(idx);     // Remote
}



//===================================================================================
// _processMessage
//===================================================================================


void _processMessage(uint32_t i)
{
    msgUnion msg(i);
    uint8_t p0 = msg.b[1];
    uint8_t p1 = msg.b[2];
    uint8_t p2 = msg.b[3];

    // short ending on active sense filter

    if (msg.isActiveSense() && !getPref8(PREF_MONITOR_ACTIVESENSE))
        return;

    char buf2[100] = {0};
    const char *s = "unknown";
    int type = msg.getMsgType();
    int pindex = msg.portIndex();
    const char *port_name = portName(pindex);
    bool is_ftp_port = isFtpPort(pindex);
    bool is_ftp_controller = isFtpController(pindex);
    int monitor = getPref8(PREF_MIDI_MONITOR);   // off, DebugPort, USB, Serial   default(DebugPort)
    Stream *out_stream =
        monitor == 3 ? &Serial3 :
        monitor == 2 ? &Serial :
        monitor == 1 ? dbgSerial : 0;
    bool show_it = out_stream;

    // colors
    //    sysex and performance stuff comes out in ligh_grey
    //    note_on is light_red
    //    note_off is light_blue
    //    and yellow is used for a warning

    // that leaves cyan, magenta, green, and white

    int bg_color = ansi_color_bg_black;

    int color =
        msg.isOutput() ? ansi_color_white :
        msg.isHost() ? ansi_color_light_cyan : ansi_color_light_magenta;

    //--------------------------------
    // buffer SYSEX
    //--------------------------------
    // FTP seems to start all sysex's with 0x15
    // and end them with 0x15, 0x16, or 0x17.

    if (type >= 0x04 && type <= 0x07)
    {
        int len = 3;
        bool is_done = 0;
        int *buflen = &sysex_buflen[pindex];
        bool *ready = &sysex_buf_ready[pindex];

        if (type == 0x04)        // start midi message
        {
            if (*ready)
            {
                if (p0 != 0xf0)
                    warning(0,"sysex does not start with F0",0);
                *ready = 0;
                *buflen = 0;
            }
        }
        else                    // end midi message
        {
            is_done = 1;
            *ready = 1;
            len = type - 0x4;
        }

        uint8_t *ip = msg.b + 1;
        uint8_t *op = &sysex_buffer[pindex][*buflen];
        while (len--)
        {
            *op++ = *ip++;
            (*buflen)++;
        }

        if (is_done)
        {
            if (sysex_buffer[pindex][*buflen-1] != 0xf7)
                warning(0,"sysex does not end with F7",0);

            int show_sysex = getPref8(PREF_MONITOR_SYSEX);
            if (out_stream && show_sysex)
            {
                sprintf(buf2,"\033[%d;%dm %s(%d,--)      sysex len=%d",
                    color,
                    bg_color,
                    port_name,
                    INDEX_CABLE(pindex),
                    sysex_buflen[pindex]);
                out_stream->println(buf2);

                // if (showPatch(pindex,sysex_buffer[pindex],sysex_buflen[pindex]) ||
                if (show_sysex == 2)
                {
                    display_bytes_long(0,0,sysex_buffer[pindex],sysex_buflen[pindex],out_stream);
                }

            }   // show_sysex
        }   // is_done

        return;
    }

    // NON-SYSEX messages


	else
    {
        if (type == 0x08)
        {
            s = "Note Off";
            most_recent_note_val = msg.b[2];
            most_recent_note_vel = msg.b[3];
            color = ansi_color_light_blue;  // understood
        }
        else if (type == 0x09)
        {
            s = "Note On";
            most_recent_note_val = msg.b[2];
            most_recent_note_vel = msg.b[3];
            color = ansi_color_light_red;
        }
        else if (type == 0x0a)
        {
            s = "VelocityChange";   // after touch poly
            color = ansi_color_light_grey;  // understood
        }
        else if (type == 0x0c)
        {
            s = "ProgramChange";
            color = ansi_color_light_grey;  // understood
        }
        else if (type == 0x0d)
        {
            s = "AfterTouch";
            color = ansi_color_light_grey;  // understood
        }
        else if (type == 0x0E)
        {
            s = "Pitch Bend";
            int value = p1 + (p2 << 7);
            value -= 8192;
            if (show_it) sprintf(buf2,"value=%d",value);
            color = ansi_color_light_grey;  // understood
        }
        else if (msg.isActiveSense())
        {
            s = "ActiveSense";
            color = ansi_color_light_grey;  // understood
        }

        // CONTROL CHANGES

        else if (type == 0x0b)
        {
            s = "ControlChange";
            show_it = show_it && (getPref8(PREF_MONITOR_PERFORMANCE_CCS) || msg.getChannel() == 8);
            const char *cmd_or_reply = is_ftp_controller ?
                "reply" : "command";

            //-----------------------------------------
            // NoteInfo - create/delete my note_t
            //-----------------------------------------
            // based on most_recent_note_vel from previous NoteOn NoteOff message

            if (is_ftp_controller && p1 == FTP_NOTE_INFO)    // 0x1e
            {
                s = "NoteInfo";
                show_it = show_it && getPref8(PREF_MONITOR_FTP_NOTE_INFO);

                note_t *note = 0;
                uint8_t string = p2>>4;
                uint8_t vel = p2 & 0x0f;
                color = most_recent_note_vel ? ansi_color_light_red : ansi_color_light_blue;

                if (most_recent_note_vel)
                {
                    note = addNote(most_recent_note_val,most_recent_note_vel,string,vel);
                }
                else
                {
                    deleteNote(string);
                }

                // they've been used
                // set them to zero in case there's a noteInfo without a Note message

                most_recent_note_vel = 0;
                most_recent_note_val = 0;
                if (show_it)
                    sprintf(buf2,"string=%d fret=%d vel=%d",string,note?note->fret:0,vel);
            }

            //-----------------------
            // tuning
            //------------------------

            else if (is_ftp_controller && (p1 == FTP_SET_TUNING || p1 == FTP_TUNING))  // 0x1d || 0x3d
            {
                s = "Tuning";

                // hmm ...

                show_it = show_it && getPref8(PREF_MONITOR_FTP_TUNING_MSGS);

                if (p1 == FTP_SET_TUNING)   // 0x1D
                {
                    s = "SetTuning";
                    tuning_note = most_recent_note;
                    color = tuning_note ? ansi_color_light_red : ansi_color_light_blue;
                }
                else    // I am going to assume that if there is no tuning_note here, we inherit the most_recent_note, if any
                {
                    if (!tuning_note)
                        tuning_note = most_recent_note;
                    color = tuning_note ? ansi_color_light_grey : ansi_color_yellow;
                        // yellow indicates a tuning message without a corresponding tuning note
                        // or even a most_recent one to inherit from
                }

                // 0x00 = -40,  0x40 == 0, 0x80 == +40
                int tuning = ((int) p2) - 0x40;      // 40 == 0,  0==-
                if (tuning_note)
                    tuning_note->tuning = tuning;
                if (show_it)
                    sprintf(buf2,"tuning=%d",tuning);

            }

            //================================================================
            // General FTP command and reply messages
            //================================================================
            // These messags are of the format
            //
            //            B7 1F command/reply
            //            B7 3F value
            //
            // And are typically sent as "commands" from the Editor to the Controller
            // who then replies with a value.   Or some are sent from the controller
            // as you diddle it's buttons and knobs.  My main goal is to figure out how
            // the whole thing works outside of the context of the Editor software, but
            // to do that, I also typcially want to parse and look at the messages from
            // the Editor.
            //
            // So it is important to differentiate a command from a reply.  We are very
            // interested in the replies that model the state of the controller, like
            // the battery level, string sensitivity levels, and so on.
            //
            // There is a large desire to separate the parsing of messages, the state
            // machine(s), the serial "midi event monitor" and the UI of my app.  The
            // general notion was to be that the UI all happened from loop(), and any
            // "important" processing took place in the timer_handler() or
            // critical_timer_handler() methods,
            //
            // It means that I *should* be building a (fast) representation of what
            // I want to show in the serial monitor here, but not calling dbgSerial->print,
            // defering that to the updateUI() method (along with tft screen drawing),
            // but that the state machine should be very quick to update the model of
            // the controller.
            //
            // #define FTP_COMMAND_OR_REPLY    0x1F
            // #define FTP_COMMAND_VALUE       0x3F

            else if (is_ftp_port && p1 == FTP_COMMAND_OR_REPLY)
            {
                s = "ftpCmdOrReply";
                last_command[pindex] = p2;

                sprintf(buf2,"%s %s",cmd_or_reply,getFTPCommandName(p2));

                if (p2 == FTP_CMD_BATTERY_LEVEL)
                    show_it = show_it && getPref8(PREF_MONITOR_FTP_BATTERY);
                else if (p2 == FTP_CMD_VOLUME_LEVEL)
                    show_it = show_it && getPref8(PREF_MONITOR_FTP_VOLUME);
            }
            else if (is_ftp_port && p1 == FTP_COMMAND_VALUE)
            {
                s = "ftpCommandParam";
                uint8_t command = last_command[pindex];
                last_command[pindex] = 0;
                const char *command_name = getFTPCommandName(command);
                uint8_t pending_command_byte = GET_COMMAND_VALUE(pending_command);
                uint8_t pending_command_value_byte = GET_COMMAND_VALUE(pending_command_value);

                // get the 8bit "command" from the 32bit midi message

                if (command == FTP_CMD_POLY_MODE)
                {
                    if (is_ftp_controller)
                    {
                        if (show_it)
                            sprintf(buf2,"%s %s setting poly_mode=%02x",cmd_or_reply,command_name,p2);
                        ftp_poly_mode = p2;
                    }
                    else if (show_it)
                        sprintf(buf2,"%s %s ",cmd_or_reply,command_name);

                }
                else if (command == FTP_CMD_BATTERY_LEVEL) // we can parse this one because it doesn't require extra knowledge
                {
                    show_it = show_it && getPref8(PREF_MONITOR_FTP_BATTERY);
                    if (is_ftp_controller)
                    {
                        if (show_it)
                            sprintf(buf2,"%s %s setting battery_level=%02x",cmd_or_reply,command_name,p2);
                        ftp_battery_level = p2;
                    }
                    else if (show_it)
                        sprintf(buf2,"%s %s ",cmd_or_reply,command_name);
                }
                else if (command == FTP_CMD_GET_SENSITIVITY)
                {
                    // we only stuff the vaue if it matches what we're waiting for ...
                    // note that pending_command is a full 32 bits, but "command" is only 8

                    if (is_ftp_controller && pending_command_byte == FTP_CMD_GET_SENSITIVITY)
                    {
                        if (show_it)
                            sprintf(buf2,"%s %s setting string_sensitivity[%d]=%02x",cmd_or_reply,command_name,pending_command_value_byte,p2);
                        ftp_sensitivity[pending_command_value_byte] = p2;
                    }
                    else if (show_it)
                    {
                        // we can't parse this one because it requires extra knowledge
                        sprintf(buf2,"%s %s %s=%02x",cmd_or_reply,command_name,is_ftp_controller?"level":"string",p2);
                    }
                }
                else if (command == FTP_CMD_VOLUME_LEVEL)
                {
                    show_it = show_it && getPref8(PREF_MONITOR_FTP_VOLUME);
                }
                else if (command == FTP_CMD_SET_SENSITIVITY)  // we can parse this one because it doesn't require extra knowledge
                {
                    int string = p2 >> 4;
                    int level  = p2 & 0xf;
                    if (is_ftp_controller)
                    {
                        if (show_it)
                            sprintf(buf2,"%s %s setting string_sensitivity[%d]=%d",cmd_or_reply,command_name,string,level);
                        ftp_sensitivity[string] = level;
                    }
                    else if (show_it)
                    {
                        sprintf(buf2,"%s %s string[%d]=%d",cmd_or_reply,command_name,string,level);
                    }
                }

                // now that we have the 2nd 3F message, if we matched the 1F message,
                // clear the pending outgoing command

                if (is_ftp_controller && command == pending_command_byte)
                {
                    display(0,"Clearing pending command(%02x)",pending_command_byte);
                    pending_command = 0;
                }

            }   // is_ftp_port && it's an FTP command_value (0x1f)

            // any other CC's are considered "performance" CCs at this time ???

            else if (0)
            {
                color = ansi_color_light_grey;  // understood
            }

        }   // 0xB0 (controller) messages


        if (show_it && out_stream)
        {
            char buf[200];
            sprintf(buf,"\033[%d;%dm %s(%d,%2d)  %02X  %-16s  %02x  %02x  %s",
                color,
                bg_color,
                port_name,
                INDEX_CABLE(pindex),
                msg.getChannel(),
                p0,
                s,
                p1,
                p2,
                buf2);

            #if 1
                // putty fix for colors background colors wrapping
                out_stream->print(buf);
                out_stream->print("\033[37;40m");
                out_stream->println();
            #else
                out_stream->println(buf);
            #endif

        }   // show_it

    }   // Not Sysex

}   // processMsg()
