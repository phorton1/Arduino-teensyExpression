#include "myDebug.h"
#include "midiQueue.h"
#include "defines.h"
#include "ftp.h"
#include "ftp_defs.h"
#include "myMidiHost.h"


#define MAX_PROCESS_QUEUE   8192
#define MAX_SYSEX_BUFFER    1024
#define MAX_OUTGOING_QUEUE  1024

// display filters


int  showSysex = 2;
bool showActiveSense = 0;
bool showTuningMessages = 1;
bool showNoteInfoMessages = 1;
    // Has a hard time running from the timer_handler()
    // with all of the serial output, FTP barfs often.
    // Especially with showSysex == 2
bool showVolumeLevel = 1;
bool showBatteryLevel = 1;
    // useful to turn these off while trying to debug other messages
bool showPerformanceCCs = 1;

    
bool showPatch(int hindex, uint8_t *buf, uint32_t buflen);
    // forward



// basic queues

int process_head = 0;
int process_tail = 0;
int outgoing_head = 0;
int outgoing_tail = 0;

uint32_t outgoing_queue[MAX_OUTGOING_QUEUE];
uint32_t process_queue[MAX_PROCESS_QUEUE];


// sysex buffer

int sysex_buflen[2]     = {0,0};
bool sysex_buf_ready[2] = {0,0};
uint8_t sysex_buffer[2][MAX_SYSEX_BUFFER];


// note processing

uint8_t most_recent_note_val = 0;
uint8_t most_recent_note_vel = 0;
    // these values are cached from the most recent NoteOn/NoteOff
    // messages and used t create (or delete) my note_t's upon 1E
    // NoteInfo messages.


// basic ftp processing

uint8_t incoming_command[2]  = {0,0};
    // as we are processing incoming messages, we keep track of the
    // most recent B7 1F "command or reply" (i.e. 07==FTP_CMD_BATTERY_LEVEL),
    // to be able to hook it up to th following "command_or_reply" value
    // message (B7 1F "value") for processing and display purposes.
    //
    // If coming from the host, this is used to set the state of certain
    // FTP variables (battery level, sensitivy etc) as well as to clear
    // any pending outGoing commands to the host.
    
    
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



//-------------------------------------
// outGoingMessage Processing
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
    display(0,"sendFTPCommandAndValue(%02x,%02x)",command,value);
    
    msgUnion msg(
        0x1B,
        0xB7,
        FTP_COMMAND_OR_REPLY,    // 0x1f
        command);
    
    _enqueueOutgoing(msg.i);
    // midi1.write_packed(msg.i);

    msg.b[2] = FTP_COMMAND_VALUE;       // 0x3f
    msg.b[3] = value;
    
    _enqueueOutgoing(msg.i);
    // midi1.write_packed(msg.i);
    // midi1.flush();
}


void _processOutgoing()
{
    // see if there's a command to dequue and send
    
    if (!pending_command)
    {
        pending_command = _dequeueOutgoing();
        if (pending_command)
        {
            pending_command_value = _dequeueOutgoing();
            command_retry_count = 0;
            
            display(0,"--> sending(%d) command(%08x) value(%08x)",command_retry_count,pending_command,pending_command_value);
            midi1.write_packed(pending_command);
            midi1.write_packed(pending_command_value);
            command_time = 0;
        }
    }
    else if (command_retry_count > 10)
    {
        my_error("timed out sending command %08x %08x",pending_command,pending_command_value);
        command_retry_count = 0;
        pending_command = 0;
    }
    else if (command_time > 50)    // resend with timer
    {
        command_retry_count++;
        display(0,"--> sending(%d) command(%08x) value(%08x)",command_retry_count,pending_command,pending_command_value);
        midi1.write_packed(pending_command);
        midi1.write_packed(pending_command_value);
        command_time = 0;
    }
}

    




//-------------------------------------
// _processMessage
//-------------------------------------

void _processMessage(uint32_t i)
{
    msgUnion msg(i);
    
    // active sensing messed up the 1E asserts
    
    if (msg.isActiveSense() && !showActiveSense)
        return;
    
    bool show_it = 1;
    char buf2[100] = {0};
    const char *s = "unknown";
    int type = msg.getMsgType();
    int hindex = msg.hostIndex();
    const char *who = hindex ? "host" : "dev ";
    int color = msg.isHost() ? ansi_color_light_cyan : ansi_color_light_magenta;
    
    uint8_t p0 = msg.b[1];
    uint8_t p1 = msg.b[2];
    uint8_t p2 = msg.b[3];
    
    //--------------------------------
    // buffer SYSEX
    //--------------------------------
    // FTP seems to start all sysex's with 0x15
    // and end them with 0x15, 0x16, or 0x17.
    
    if (type >= 0x04 && type <= 0x07)
    {
        int len = 3;
        bool is_done = 0;
        int *buflen = &sysex_buflen[hindex];
        bool *ready = &sysex_buf_ready[hindex];
            
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
        uint8_t *op = &sysex_buffer[hindex][*buflen];
        while (len--)
        {
            *op++ = *ip++;
            (*buflen)++;
        }

        if (is_done  && sysex_buffer[hindex][*buflen-1] != 0xf7)
            warning(0,"sysex does not end with F7",0);
        
        if (is_done && showSysex)
        {
            sprintf(buf2,"\033[%dm %s(%d,--)      sysex len=%d",
                color,
                who,
                msg.getCable(),
                sysex_buflen[hindex]);
            Serial.println(buf2);
            if (showPatch(hindex,sysex_buffer[hindex],sysex_buflen[hindex]) ||
                showSysex == 2)
                display_bytes_long(0,0,sysex_buffer[hindex],sysex_buflen[hindex]);
                
            ;
        }
    }
    
    // NON-SYSEX messages
    
    
	else 
    {
        if (type == 0x08)
        {
            s = "Note Off";
            color = ansi_color_light_blue;
            most_recent_note_val = msg.b[2];
            most_recent_note_vel = msg.b[3];
        }
        else if (type == 0x09)
        {
            s = "Note On";
            color = ansi_color_light_red;
            most_recent_note_val = msg.b[2];
            most_recent_note_vel = msg.b[3];
        }
        else if (type == 0x0a)
        {
            s = "VelocityChange";   // after touch poly
        }
        else if (type == 0x0c)
        {
            s = "ProgramChange";
        }
        else if (type == 0x0d)
        {
            s = "AfterTouch";
        }
        else if (type == 0x0E)
        {
            s = "Pitch Bend";
            int value = p1 + (p2 << 7);
            value -= 8192;
            sprintf(buf2,"value=%d",value);
            color = ansi_color_light_gray;  // understood
        }
        else if (msg.isActiveSense())
        {
            s = "ActiveSense";
            color = ansi_color_light_gray;  // understood
        }
        
        
        
        else if (type == 0x0b)
        {
            s = "ControlChange";
            show_it = showPerformanceCCs || msg.getChannel() == 8;
            
            //===================================================
            // Messages from Controller ONLY
            //===================================================
            // These messages are never sent by the FTP editor.
            // They are sent by the controller and necessary to effect my UI.
            
            //-----------------------------------------
            // NoteInfo - create/delete my note_t
            //-----------------------------------------
            // based on most_recent_note_vel from previous NoteOn NoteOff message
            
            if (p1 == FTP_NOTE_INFO)    // 0x1e
            {
                s = "NoteInfo";
                show_it = showNoteInfoMessages;

                note_t *note = 0;
                uint8_t string = p2>>4;
                uint8_t vel = p2 & 0x0f;
                color = most_recent_note_vel ? ansi_color_light_red : ansi_color_light_blue;

                #if 0
                    if (!most_recent_note_val)
                        warning(0,"NoteInfo (B7 1E xy) not following a note!!",0);
                    if (((bool)vel) != ((bool)most_recent_note_vel))
                        warning(0,"expected NoteInfo (B7 1E %02x) vel(%02x) to correspond to most_recent_note_vel(%02x)",
                            p2,vel,most_recent_note_vel);
                #endif
                
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
                sprintf(buf2,"string=%d fret=%d vel=%d",string,note?note->fret:0,vel);
            }

            //-----------------------
            // tuning
            //------------------------
            
            else if (p1 == FTP_SET_TUNING || p1 == FTP_TUNING)  // 0x1d || 0x3d
            {
                s = "Tuning";
                
                // hmm ... 
                    
                show_it = showTuningMessages;
                
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
                    color = tuning_note ? ansi_color_light_gray : ansi_color_yellow;
                        // yellow indicates a tuning message without a corresponding tuning note
                        // or even a most_recent one to inherit from
                }
                
                // 0x00 = -40,  0x40 == 0, 0x80 == +40
                int tuning = ((int) p2) - 0x40;      // 40 == 0,  0==-
                sprintf(buf2,"tuning=%d",tuning);
                if (tuning_note)
                    tuning_note->tuning = tuning;
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
            // I want to show in the serial monitor here, but not calling Serial.print,
            // defering that to the updateUI() method (along with tft screen drawing),
            // but that the state machine should be very quick to update the model of
            // the controller.
            //
            // #define FTP_COMMAND_OR_REPLY    0x1F
            // #define FTP_COMMAND_VALUE       0x3F
        
            else if (p1 == FTP_COMMAND_OR_REPLY)
            {
                s = "ftpCmdOrReply";
                incoming_command[hindex] = p2;
                sprintf(buf2,"%s %s",hindex?"reply":"command",getFTPCommandName(p2));
                
                if (p2 == FTP_CMD_BATTERY_LEVEL)
                    show_it = showBatteryLevel;
                else if (p2 == FTP_CMD_VOLUME_LEVEL)
                    show_it = showVolumeLevel;
            }
            else if (p1 == FTP_COMMAND_VALUE)
            {
                s = "ftpCommandParam";
                uint8_t command = incoming_command[hindex]; 
                incoming_command[hindex] = 0;
                const char *command_name = getFTPCommandName(command);
                const char *what_name = hindex ? "reply" : "command";
                uint8_t pending_command_byte = GET_COMMAND_VALUE(pending_command);
                uint8_t pending_command_value_byte = GET_COMMAND_VALUE(pending_command_value);
                
                    // get the 8bit "command" from the 32bit midi message
                
                // we used to sniff out the commands from the FTP Editor
                // and make sense of certain replies from the host based
                // on the commands the Editor was sending.
                
                // Now we ONLY take responses from the for certain things
                // (i.e. getSensitivity message) if we have explicitly asked,
                // and are waiting for them.
                
                if (command == FTP_CMD_BATTERY_LEVEL) // we can parse this one because it doesn't require extra knowledge
                {
                    show_it = showBatteryLevel;
                    if (hindex)     
                    {
                        sprintf(buf2,"%s %s setting battery_level=%02x",what_name,command_name,p2);
                        ftp_battery_level = p2;
                    }
                    else
                        sprintf(buf2,"%s %s ",what_name,command_name);
                    
                }
                else if (command == FTP_CMD_GET_SENSITIVITY)  
                {
                    // we only stuff the vaue if it matches what we're waiting for ...
                    // note that pending_command is a full 32 bits, but "command" is only 8
                    
                    if (hindex && pending_command_byte == FTP_CMD_GET_SENSITIVITY)
                    {
                        sprintf(buf2,"%s %s setting string_sensitivity[%d]=%02x",what_name,command_name,pending_command_value_byte,p2);
                        ftp_sensitivity[pending_command_value_byte] = p2;
                    }
                    else
                    {
                        // we can't parse this one because it requires extra knowledge
                        sprintf(buf2,"%s %s %s=%02x",what_name,command_name,hindex?"level":"string",p2);
                    }
                }
                else if (command == FTP_CMD_VOLUME_LEVEL)
                {
                    show_it = showVolumeLevel;
                }
                else if (command == FTP_CMD_SET_SENSITIVITY)  // we can parse this one because it doesn't require extra knowledge
                {
                    int string = p2 >> 4;
                    int level  = p2 & 0xf;
                    sprintf(buf2,"%s %s setting string_sensitivity[%d]=%d",what_name,command_name,string,level);
                    if (hindex)
                    {
                        sprintf(buf2,"%s %s setting string_sensitivity[%d]=%d",what_name,command_name,string,level);
                        ftp_sensitivity[string] = level;
                    }
                    else
                    {
                        sprintf(buf2,"%s %s string[%d]=%d",what_name,command_name,string,level);
                    }
                }
                
                // now that we have the 2nd 3F message, if we matched the 1F message,
                // clear the pending outgoing command
                
                if (hindex && command == pending_command_byte)
                {
                    display(0,"Clearing pending command(%02x)",pending_command_byte);
                    pending_command = 0;
                }
            }
        }
        

        if (show_it)
        {
            char buf[200];
            sprintf(buf,"\033[%dm %s(%d,%2d)  %02X  %-16s  %02x  %02x  %s",
                color,
                who,
                msg.getCable(),
                msg.getChannel(),
                p0,
                s,
                p1,
                p2,
                buf2);
            Serial.println(buf);
            
        }   // show_it
    }   // Not Sysext
    
}   // processMsg()

    


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



//------------------------------------------------
// patches
//-------------------------------------------------

uint8_t patch_sig[6] = {0xF0, 0x00, 0x01, 0x6E, 0x01, 0x21};

bool showPatch(int hindex, uint8_t *buf, uint32_t buflen)
{
    uint8_t *p = buf;
    patch_sig[5] = hindex ? 0x21 : 0x41;
    // if (!hindex) return false;            // looking for things from controller
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

