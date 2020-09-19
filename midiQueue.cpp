#include "myDebug.h"
#include "midiQueue.h"
#include "defines.h"
#include "prefs.h"
#include "ftp.h"
#include "ftp_defs.h"
#include "myMidiHost.h"
#include "expSystem.h"


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

void mySendMidiMessage(uint8_t msg_type, uint8_t channel, uint8_t p1, uint8_t p2)
{
    msgUnion msg(
        msg_type,   //  | PORT_MASK_OUTPUT,
        (msg_type<<4) | (channel-1),
        p1,
        p2);

    usb_midi_write_packed(msg.i);
    usb_midi_flush_output();
    theSystem.midiActivity(INDEX_MASK_OUTPUT);
    enqueueProcess(msg.i | PORT_MASK_OUTPUT);
}


void mySendDeviceProgramChange(uint8_t prog_num, uint8_t channel)
{
    #if 1
        mySendMidiMessage(0x0C, channel, prog_num, 0);
    #else
        usbMIDI.sendProgramChange(prog_num, channel);
        msgUnion msg(
            0x0C | PORT_MASK_OUTPUT,
            0xC0 | (channel-1),
            prog_num,
            0);
        theSystem.midiActivity(INDEX_MASK_OUTPUT);   // it IS port #2
        enqueueProcess(msg.i);
    #endif
}

void mySendDeviceControlChange(uint8_t cc_num, uint8_t value, uint8_t channel)
{
    #if 1
        mySendMidiMessage(0x0B, channel, cc_num, value);
    #else
        usbMIDI.sendControlChange(cc_num, value, channel);
        msgUnion msg(
            0x0B | PORT_MASK_OUTPUT,
            0xB0 | (channel-1),
            cc_num,
            value);
        theSystem.midiActivity(INDEX_MASK_OUTPUT);   // it IS port #2
        enqueueProcess(msg.i);
    #endif
}



void sendSerialControlChange(uint8_t cc_num, uint8_t value, const char *debug_msg)
{
    #if 0
        display(0,"sendSerialControlChange(0x%02x, 0x%02x) from %s",cc_num,value,debug_msg);
    #endif

    unsigned char midi_buf[4];
    midi_buf[0] = 0xB;				// controller message
    midi_buf[1] = 0xB0;				// controller message on channel one
    midi_buf[2] = cc_num;           // the cc_number
    midi_buf[3] = value;			// the value
    Serial3.write(midi_buf,4);
}



void mySendFtpSysex(int length, uint8_t *buf)
    // called by me: midi_host.sendSysEx(sizeof(ftpRequestPatch),ftpRequestPatch,true);
    // Pauls API: void sendSysEx(uint32_t length, const uint8_t *data, bool hasTerm=false, uint8_t cable=0)
{
    int ftp_output_port = FTP_OUTPUT_PORT;
    if (ftp_output_port)            // Host or Remote
    {
        int pindex = INDEX_MASK_OUTPUT | INDEX_MASK_CABLE |
            (ftp_output_port == 1 ? INDEX_MASK_HOST : 0);

        msgUnion msg(0);
        int len = length;
        uint8_t *p = buf;
        bool started = false;

        msg.b[0] = 0x14;
            // we are always writing to cable 1 (0x10)
            // the 4 is the message type

        bool flush_usb_midi = false;

        while (len)
        {
            // create the 32 bit packet

            int take = 3;
            if (started && len <= 3)
            {
                take = len;
                msg.b[0] = 0x15 + len-1;
            }
            for (int i=0; i<3; i++)
            {
                msg.b[i+1] = (i<take) ? *p++ : 0;
            }
            len -= take;
            started = 1;

            if (ftp_output_port == 2)   // Remote
            {
                flush_usb_midi = true;
                usb_midi_write_packed(msg.i);
                enqueueProcess(msg.i | PORT_MASK_OUTPUT);
            }
            else
            {
                midi_host.write_packed(msg.i);
                enqueueProcess(msg.i | PORT_MASK_OUTPUT | PORT_MASK_HOST);
            }

            theSystem.midiActivity(pindex);
        }

        if (flush_usb_midi)
            usb_midi_flush_output();

    }
    else
    {
        warning(0,"PREF_FTP_PORT is NONE in mySendFtpSysex(%d)",length);
    }
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

            theSystem.midiActivity(INDEX_MASK_OUTPUT | INDEX_MASK_CABLE);   // it IS port #3
            enqueueProcess(pending_command | PORT_MASK_OUTPUT);
            enqueueProcess(pending_command_value | PORT_MASK_OUTPUT);
        }
        else        // Host
        {
            midi_host.write_packed(pending_command);
            midi_host.write_packed(pending_command_value);
            theSystem.midiActivity(INDEX_MASK_HOST | INDEX_MASK_OUTPUT | INDEX_MASK_CABLE);   // it IS port #7
            enqueueProcess(pending_command | PORT_MASK_HOST | PORT_MASK_OUTPUT);
            enqueueProcess(pending_command_value | PORT_MASK_HOST | PORT_MASK_OUTPUT);
        }
        command_time = 0;
    }
    else
    {
        warning(0,"PREF_FTP_PORT is NONE in sendPendingCommand(%d) command(%08x) value(%08x)",command_retry_count,pending_command,pending_command_value);
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




// port classification for _processOMessageg;


bool isFtpPort(int idx)
    // if messages from the port should be parsed and
    // displayed as ftp specific
{
    bool is_spoof = getPref8(PREF_SPOOF_FTP);
    if (is_spoof) return 1;                             // all ports if spoofing
    uint8_t pref_ftp_port = getPref8(PREF_FTP_PORT);    // otherwise
    if (!pref_ftp_port) return 0;   // Off              // only if it matches
    if (pref_ftp_port == FTP_PORT_HOST)   // Host       // the specified ftp port
        return INDEX_IS_HOST(idx);
    return !INDEX_IS_HOST(idx);     // Remote
}


bool isFtpController(int idx)
    // returns true if message should be considered as
    // coming from the controller for updating our internal
    // ftp state variables
{
    if (!INDEX_CABLE(idx)) return false;                // only on cable 1
    if (INDEX_IS_OUTPUT(idx)) return false;             // only on input
    bool is_spoof = getPref8(PREF_SPOOF_FTP);
    if (is_spoof) return INDEX_IS_HOST(idx);            // if spoofing and on host
    uint8_t pref_ftp_port = getPref8(PREF_FTP_PORT);    // otherwise
    if (!pref_ftp_port) return 0;   // Off              // only if ftp port specified
    if (pref_ftp_port == FTP_PORT_HOST)  // Host        // and it matches the specified ftp port
        return INDEX_IS_HOST(idx);
    return !INDEX_IS_HOST(idx);     // Remote
}


//-------------------------------
// standard controller names
//-------------------------------

const char *getStandardCCName(int i)
   // from http://www.nortonmusic.com/midi_cc.html
{
    if (i==0  ) return "Bank Select (MSB)";
    if (i==1  ) return "Modulation Wheel";
    if (i==2  ) return "Breath controller";
    if (i==4  ) return "Foot Pedal (MSB)";
    if (i==5  ) return "Portamento Time (MSB)";
    if (i==6  ) return "Data Entry (MSB)";
    if (i==7  ) return "Volume (MSB)";
    if (i==8  ) return "Balance (MSB)";
    if (i==10 ) return "Pan position (MSB)";
    if (i==11 ) return "Expression (MSB)";
    if (i==12 ) return "Effect Control 1 (MSB)";
    if (i==13 ) return "Effect Control 2 (MSB)";
    if (i==16 ) return "General Purpose Slider 1";
    if (i==17 ) return "Knob 1 or General Purpose Slider 2";
    if (i==18 ) return "General Purpose Slider 3";
    if (i==19 ) return "Knob 2 General Purpose Slider 4";
    if (i==20 ) return "Knob 3";
    if (i==21 ) return "Knob 4";
    if (i==32 ) return "Bank Select (LSB) (see cc0)";
    if (i==33 ) return "Modulation Wheel (LSB)";
    if (i==34 ) return "Breath controller (LSB)";
    if (i==36 ) return "Foot Pedal (LSB)";
    if (i==37 ) return "Portamento Time (LSB)";
    if (i==38 ) return "Data Entry (LSB)";
    if (i==39 ) return "Volume (LSB)";
    if (i==40 ) return "Balance (LSB)";
    if (i==42 ) return "Pan position (LSB)";
    if (i==43 ) return "Expression (LSB)";
    if (i==44 ) return "Effect Control 1 (LSB)";
    if (i==45 ) return "Effect Control 2 (LSB)";
    if (i==64 ) return "Hold Pedal (on/off)";
    if (i==65 ) return "Portamento (on/off)";
    if (i==66 ) return "Sustenuto Pedal (on/off)";
    if (i==67 ) return "Soft Pedal (on/off)";
    if (i==68 ) return "Legato Pedal (on/off)";
    if (i==69 ) return "Hold 2 Pedal (on/off)";
    if (i==70 ) return "Sound Variation";
    if (i==71 ) return "Resonance (aka Timbre)";
    if (i==72 ) return "Sound Release Time";
    if (i==73 ) return "Sound Attack Time";
    if (i==74 ) return "Frequency Cutoff (aka Brightness)";
    if (i==75 ) return "Sound Control 6";
    if (i==76 ) return "Sound Control 7";
    if (i==77 ) return "Sound Control 8";
    if (i==78 ) return "Sound Control 9";
    if (i==79 ) return "Sound Control 10";
    if (i==80 ) return "Decay or General Purpose Button 1";
    if (i==81 ) return "Hi Pass Filter Frequency or General Purpose Button 2";
    if (i==82 ) return "General Purpose Button 3";
    if (i==83 ) return "General Purpose Button 4";
    if (i==91 ) return "Reverb Level";
    if (i==92 ) return "Tremolo Level";
    if (i==93 ) return "Chorus Level";
    if (i==94 ) return "Celeste Level or Detune";
    if (i==95 ) return "Phaser Level";
    if (i==96 ) return "Data Button increment";
    if (i==97 ) return "Data Button decrement";
    if (i==98 ) return "Non-registered Parameter (LSB)";
    if (i==99 ) return "Non-registered Parameter (MSB)";
    if (i==100) return "Registered Parameter (LSB)";
    if (i==101) return "Registered Parameter (MSB)";
    if (i==120) return "All Sound Off";
    if (i==121) return "All Controllers Off";
    if (i==122) return "Local Keyboard (on/off)";
    if (i==123) return "All Notes Off";
    if (i==124) return "Omni Mode Off";
    if (i==125) return "Omni Mode On";
    if (i==126) return "Mono Operation";
    if (i==127) return "Poly Operation";
    return "undefined";
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
    const char *s = "unknown msg!!";
    int type = msg.getMsgType();
    int pindex = msg.portIndex();
    int channel = msg.getChannel();
    const char *port_name = portName(pindex);
    bool is_ftp_port = isFtpPort(pindex);
    bool is_ftp_controller = isFtpController(pindex);
    int monitor = getPref8(PREF_MIDI_MONITOR);   // off, DebugPort, USB, Serial   default(DebugPort)

    Stream *out_stream =
        monitor == 3 ? &Serial3 :
        monitor == 2 ? &Serial :
        monitor == 1 ? dbgSerial : 0;

    bool show_it =
        out_stream &&
        getPref8(PREF_MONITOR_PORT0 + pindex);

    // display(0,"show_it=%d pindex=%02x type=%d",show_it,pindex,type,channel);


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
    // FTP seems to start all sysex's with 0x14
    // and end them with 0x15, 0x16, or 0x17.

    if (type >= 0x04 && type <= 0x07)
    {
        int len = 3;
        bool is_done = 0;
        uint8_t *buf = sysex_buffer[pindex];
        int buf_len = sysex_buflen[pindex];

        if (type == 0x04)        // start midi message
        {
            if (!buf_len && p0 != 0xf0)
                warning(0,"sysex does not start with F0 got(%02x)",p0);
        }
        else                    // end midi message
        {
            is_done = 1;
            len = type - 0x4;
        }

        uint8_t *ip = msg.b + 1;
        uint8_t *op = &buf[buf_len];
        while (len--)
        {
            *op++ = *ip++;
            buf_len++;
        }
        sysex_buflen[pindex] = buf_len;

        if (is_done)
        {
            sysex_buflen[pindex] = 0;
            if (buf[buf_len-1] != 0xf7)
                warning(0,"sysex does not end with F7",0);

            if (out_stream &&
                getPref8(PREF_MONITOR_PORT0 + pindex))
            {
                int show_sysex = getPref8(PREF_MONITOR_SYSEX);
                if (show_sysex)
                {
                    sprintf(buf2,"\033[%d;%dm %s(%d,--)      sysex len=%d",
                        color,
                        bg_color,
                        port_name,
                        INDEX_CABLE(pindex),
                        buf_len);
                    out_stream->println(buf2);
                }

                // if (is_ftp_port &&
                if (getPref8(PREF_MONITOR_PARSE_FTP_PATCHES))
                    showFtpPatch(out_stream,color,bg_color,is_ftp_controller,buf,buf_len);

                if (show_sysex == 2)
                    display_bytes_long(0,0,buf,buf_len,out_stream);

            }   // show_sysex
        }   // is_done

        return;
    }

    // NON-SYSEX messages


	else
    {
        show_it = show_it &&
            (getPref8(PREF_MONITOR_CHANNEL1 + channel - 1));

        if (type == 0x08)
        {
            s = "Note Off";
            most_recent_note_val = msg.b[2];
            most_recent_note_vel = msg.b[3];
            if (i & PORT_MASK_PERFORM)
            {
                bg_color = ansi_color_bg_blue;
                color = ansi_color_white;
            }
            else
                color = ansi_color_light_blue;  // understood
            show_it = show_it && getPref8(PREF_MONITOR_NOTE_OFF);
        }
        else if (type == 0x09)
        {
            s = "Note On";
            most_recent_note_val = msg.b[2];
            most_recent_note_vel = msg.b[3];
            if (i & PORT_MASK_PERFORM)
            {
                bg_color = ansi_color_bg_red;
                color = ansi_color_white;
            }
            else
                color = ansi_color_light_red;
            show_it = show_it && getPref8(PREF_MONITOR_NOTE_ON);
        }
        else if (type == 0x0a)
        {
            s = "VelocityChange";   // after touch poly
            color = ansi_color_light_grey;  // understood
            show_it = show_it && getPref8(PREF_MONITOR_VELOCITY);
        }
        else if (type == 0x0c)
        {
            s = "ProgramChange";
            color = ansi_color_light_grey;  // understood
            show_it = show_it && getPref8(PREF_MONITOR_PROGRAM_CHG);
        }
        else if (type == 0x0d)
        {
            s = "AfterTouch";
            color = ansi_color_light_grey;  // understood
            show_it = show_it && getPref8(PREF_MONITOR_AFTERTOUCH);
        }
        else if (type == 0x0E)
        {
            s = "Pitch Bend";
            int value = p1 + (p2 << 7);
            value -= 8192;
            show_it = show_it && getPref8(PREF_MONITOR_PITCHBEND);
            if (show_it) sprintf(buf2,"value=%d",value);
            if (i & PORT_MASK_PERFORM)
            {
                bg_color = ansi_color_bg_grey;
                color = ansi_color_white;
            }
            else
                color = ansi_color_light_grey;  // understood
        }
        else if (msg.isActiveSense())
        {
            s = "ActiveSense";
            color = ansi_color_light_grey;  // understood
        }

        else if (p0 == 0xF8)
        {
            show_it = false;
                // don't show the midi clock
        }
        else if (p0 == 0xFA)
        {
            s = "MIDI CLOCK START";
        }
        else if (p0 == 0xFA)
        {
            s = "MIDI CLOCK CONTINUE";
        }
        else if (p0 == 0xFA)
        {
            s = "MIDI CLOCK STOP";
        }

        // CONTROL CHANGES

        else if (type == 0x0b)
        {
            s = "ControlChange";
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
                show_it = show_it && getPref8(PREF_MONITOR_FTP_COMMANDS);

                if (show_it)
                {
                    bool known = true;
                    const char *command_name = getFTPCommandName(p2);
                    if (!command_name)
                    {
                        command_name = "unknown";
                        known = false;
                    }

                    if (!known)
                        show_it = getPref8(PREF_MONITOR_UNKNOWN_FTP_COMMANDS);
                    else if (p2 == FTP_CMD_POLY_MODE)
                        show_it = getPref8(PREF_MONITOR_FTP_POLY_MODE);
                    else if (p2 == FTP_CMD_PITCHBEND_MODE)
                        show_it = getPref8(PREF_MONITOR_FTP_BEND_MODE);
                    else if (p2 == FTP_CMD_VOLUME_LEVEL)
                        show_it = getPref8(PREF_MONITOR_FTP_VOLUME);
                    else if (p2 == FTP_CMD_BATTERY_LEVEL)
                        show_it = getPref8(PREF_MONITOR_FTP_BATTERY);
                    else if (p2 == FTP_CMD_GET_SENSITIVITY ||
                             p2 == FTP_CMD_SET_SENSITIVITY)
                        show_it = getPref8(PREF_MONITOR_FTP_SENSITIVITY);
                    else
                        show_it = getPref8(PREF_MONITOR_KNOWN_FTP_COMMANDS);

                    if (show_it)
                        sprintf(buf2,"%s %s",cmd_or_reply,command_name);
                }
            }
            else if (is_ftp_port && p1 == FTP_COMMAND_VALUE)
            {
                s = "ftpCommandParam";
                uint8_t command = last_command[pindex];
                last_command[pindex] = 0;
                uint8_t pending_command_byte = GET_COMMAND_VALUE(pending_command);
                uint8_t pending_command_value_byte = GET_COMMAND_VALUE(pending_command_value);

                bool known = true;
                const char *command_name = getFTPCommandName(pending_command_byte);
                if (!command_name)
                {
                    command_name = "unknown";
                    known = false;
                }

                if (!known)
                {
                    show_it = show_it && getPref8(PREF_MONITOR_UNKNOWN_FTP_COMMANDS);
                }
                else if (command == FTP_CMD_POLY_MODE)
                {
                    show_it = show_it && getPref8(PREF_MONITOR_FTP_POLY_MODE);

                    if (is_ftp_controller)
                    {
                        if (show_it)
                            sprintf(buf2,"%s %s setting poly_mode=%02x",cmd_or_reply,command_name,p2);
                        ftp_poly_mode = p2;
                    }
                    else if (show_it)
                        sprintf(buf2,"%s %s ",cmd_or_reply,command_name);
                }

                else if (command == FTP_CMD_PITCHBEND_MODE)
                {
                    show_it = show_it && getPref8(PREF_MONITOR_FTP_BEND_MODE);

                    if (is_ftp_controller)
                    {
                        if (show_it)
                            sprintf(buf2,"%s %s setting bend_mode=%02x",cmd_or_reply,command_name,p2);
                        ftp_bend_mode = p2;
                    }
                    else if (show_it)
                        sprintf(buf2,"%s %s ",cmd_or_reply,command_name);

                }
                else if (command == FTP_CMD_VOLUME_LEVEL)
                {
                    show_it = show_it && getPref8(PREF_MONITOR_FTP_VOLUME);
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

                // prh - added 2020-08-06

                else if (command == FTP_CMD_DYNAMICS_SENSITIVITY)
                {
                    int value = p2;
                    if (is_ftp_controller)
                    {
                        if (show_it)
                            sprintf(buf2,"%s %s setting dynamic_sensitivity=%d",cmd_or_reply,command_name,value);
                        ftp_dynamic_range = value;
                    }
                    else if (show_it)
                    {
                        sprintf(buf2,"%s %s dynamic_sensitivity=%d",cmd_or_reply,command_name,value);
                    }
                }
                else if (command == FTP_CMD_DYNAMICS_OFFSET)
                {
                    int value = p2;
                    if (is_ftp_controller)
                    {
                        if (show_it)
                            sprintf(buf2,"%s %s setting dynamic_offset=%d",cmd_or_reply,command_name,value);
                        ftp_dynamic_offset = value;
                    }
                    else if (show_it)
                    {
                        sprintf(buf2,"%s %s dynamic_offset=%d",cmd_or_reply,command_name,value);
                    }
                }
                else if (command == FTP_CMD_TOUCH_SENSITIVITY)
                {
                    int value = p2;
                    if (is_ftp_controller)
                    {
                        if (show_it)
                            sprintf(buf2,"%s %s setting touch_sensitivity=%d",cmd_or_reply,command_name,value);
                        ftp_touch_sensitivity = value;
                    }
                    else if (show_it)
                    {
                        sprintf(buf2,"%s %s touch_sensitivity=%d",cmd_or_reply,command_name,value);
                    }
                }





                else
                {
                    show_it = show_it && getPref8(PREF_MONITOR_KNOWN_FTP_COMMANDS);
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
            // we at least try to get the "known" midi CC names

            else
            {
                s = "CC";
                sprintf(buf2,"%-3d - %s",p1,getStandardCCName(p1));
            }

        }   // 0xB0 (controller) messages
        else
        {
            show_it = show_it && getPref8(PREF_MONITOR_EVERYTHING_ELSE);
        }

        if (show_it && out_stream)
        {
            char buf[200];
            sprintf(buf,"\033[%d;%dm %s(%d,%2d)  %02X  %-16s  %02x  %02x  %s",
                color,
                bg_color,
                port_name,
                INDEX_CABLE(pindex),
                channel,
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
