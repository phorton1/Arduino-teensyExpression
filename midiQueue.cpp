#include "myDebug.h"
#include "midiQueue.h"
#include "FTP.h"
#include "ftp_defs.h"

// This file contains routines that process and display midi messages,
// and sets the state of the FTP controller in FTP.cpp ...
//
// This is NOT a general purpose MIDI parser, though it is close.
// It contains many assumptiosn about messages that are specific
// to the FTP system.
//
// There should be a separate doc about the various messages the
// FTP controller puts out, and how it works with the FTP editor.
// At this time the information is sporadically included in this
// and a few other files.

#define MAX_DISPLAY_QUEUE  8192
#define MAX_PROCESS_QUEUE  8192
#define MAX_SYSEX_BUFFER   1024

// NOTE INFO MESSAGES
//
//      Following most NoteOn and NoteOff messages is a B7 1E xy
//      "NoteInfo" message, where x is the string number and y is a
//      compressed 0..15 velocity value (for the VU meter).
//
//      Although these appear to be primarly used to drive the FTP editor
//      sensitivity VU meter, they also appear to be neded to drive the FTP
//      'fretboard' display, as there is no other apparent way for the editor
//      to know which string was meant in a NOTE_ON message.
//
//      In fact, in my system, the lifetime of the "Note" (note_t object) is
//      bracketed by the 1E messages, and NOT by the note on and note off messages.
//      I cache the NOTE_ON and NOTE_OFF values, as a pair of uint8_s.
//
//      Thus I assume that these 1E messages refer to the most recent
//      NoteOn or NoteOff message.   There is a notion of the "most_recent"
//      note object, which is the most recently created one, which
//      comes into play in the Tuning messages.
//
// TUNING MESSAGES:
//
//      Following the NoteOn/1E message is usually a single B7 1D yy
//      and a series of B7 3D yy messages.  These are generally tuning
//      messages, where yy is the 0x40 biased signed number (from -0x40
//      to 0x40).
//      
//      I call the 1D message a "SetTuning" message as it appears to
//      set the tunning relative to the most recent NoteOn/Off message,
//      whereas the 3D messages appear to be continuations, which I just
//      call "Tuning" messages.   It is not invariant that a NoteOn or
//      NoteOff is followed by a 1D ... the tuner will keep working on
//      a given string if multiple strings are picked.
//
//      Therefore I grab the "most recent" note upon a 1D and asusme that
//      is the note we are tuning.  If it goes away, the tuner is turned
//      off until the next 1D in the context of a new NoteOn/1E message



int  showSysex = 1; 
bool showActiveSense = 0;
bool showTuningMessages = 1;
bool showNoteInfoMessages = 1;
    // Has a hard time running from the timer_handler()
    // with all of the serial output, FTP barfs often.
    // Especially with showSysex == 2

int display_head = 0;
int display_tail = 0;
int process_head = 0;
int process_tail = 0;

int sysex_buflen[2]     = {0,0};
bool sysex_buf_ready[2] = {0,0};
uint8_t pending_command[2]  = {0,0};
    // the most recent B7 1F "command or reply" value (i.e. 07==FTP_BATTERY_LEVEL)
    // it will be reset on the next 1F or following 3F message ...
                                 

uint32_t display_queue[MAX_DISPLAY_QUEUE];
uint32_t process_queue[MAX_PROCESS_QUEUE];
uint8_t sysex_buffer[2][MAX_SYSEX_BUFFER];

uint8_t most_recent_note_val = 0;
uint8_t most_recent_note_vel = 0;
    // these values are cached from the most recent NoteOn/NoteOff
    // messages and used t create (or delete) my note_t's upon 1E
    // NoteInfo messages.



#define ansi_color_black 	        30
#define ansi_color_red 	     	    31
#define ansi_color_green 	        32
#define ansi_color_brown 	 	    33
#define ansi_color_blue 	        34
#define ansi_color_magenta 	 	    35
#define ansi_color_cyan 	        36
#define ansi_color_grey 	        37
#define ansi_color_light_gray  	    90
#define ansi_color_light_red 	    91
#define ansi_color_light_green 	    92
#define ansi_color_yellow 		    93
#define ansi_color_light_blue  	    94
#define ansi_color_light_magenta    95
#define ansi_color_light_cyan 	    96
#define ansi_color_white  		    97



void showRawMessage(uint32_t i)
{
    msgUnion msg(i);
    
    if (showActiveSense || !msg.isActiveSense())
    {
        char buf[100];
        sprintf(buf,"\033[%dm %s  %02x %02x %02x\n",
            msg.isHost() ? ansi_color_light_cyan : ansi_color_light_magenta,
            msg.isHost() ? "host  " : "device",
            msg.type(),msg.param1(),msg.param2());
        Serial.print(buf);
    }
}



void processMsg(uint32_t i)
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
            if (showSysex == 2)
                display_bytes(0,who,sysex_buffer[hindex],sysex_buflen[hindex]);
            else
            {
                sprintf(buf2,"\033[%dm  %s          sysex len=%d",color,who,sysex_buflen[hindex]);
                Serial.println(buf2);
            }
        }
    }
    
    // NON-SYSEX messages
    
    
	else 
    {
        if (type == 0x08)
        {
            s = "Note Off";
            color = ansi_color_light_red;
            most_recent_note_val = msg.b[2];
            most_recent_note_vel = msg.b[3];
        }
        else if (type == 0x09)
        {
            s = "Note On";
            color = ansi_color_light_blue;
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
                color = ansi_color_light_red;

                note_t *note = 0;
                uint8_t string = p2>>4;
                uint8_t vel = p2 & 0x0f;
                color = most_recent_note_vel ? ansi_color_light_blue : ansi_color_light_red;

                if (!most_recent_note_val)
                    warning(0,"NoteInfo (B7 1E xy) not following a note!!",0);
                if (((bool)vel) != ((bool)most_recent_note_vel))
                    warning(0,"expected NoteInfo (B7 1E %02x) vel(%02x) to correspond to most_recent_note_vel(%02x)",
						p2,vel,most_recent_note_vel);
                    
                if (most_recent_note_vel)
                {
                    color = ansi_color_light_blue;
                    note = addNote(most_recent_note_val,most_recent_note_vel,string);
                }
                else
                {
                    deleteNote(most_recent_note_val,string);
                }
                sprintf(buf2,"string=%d fret=%d vel=%d",string,note?note->fret:0,vel);
            }

            else if (p1 == FTP_SET_TUNING || p1 == FTP_TUNING)  // 0x1d || 0x3d
            {
                s = "Tuning";
                
                // hmm ... 
                    
                show_it = showTuningMessages;
                
                if (p1 == FTP_SET_TUNING)   // 0x1D
                {
                    s = "SetTuning";
                    tuning_note = most_recent_note;
                    color = tuning_note ? ansi_color_light_blue : ansi_color_light_red;
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
            // 
            // // specific commands
            // 
            // #define FTP_SLIDER_POSITION     0x05
            // #define FTP_BATTERY_LEVEL       0x07
            // #define FTP_VOLUME_LEVEL        0x08
            // #define FTP_GET_SENSITIVITY     0x3C
            // #define FTP_SET_SENSITIVITY     0x42
        
            else if (p1 == FTP_COMMAND_OR_REPLY)
            {
                s = "ftpCmdOrReply";
                pending_command[hindex] = p2;
                sprintf(buf2,"%s %s",hindex?"reply":"command",getFTPCommandName(p2));
            }
            else if (p1 == FTP_COMMAND_VALUE)
            {
                s = "ftpCommandParam";
                uint8_t pending = pending_command[hindex];
                pending_command[hindex] = 0;
                const char *pending_name = getFTPCommandName(pending);
                const char *what_name = hindex ? "reply" : "command";
                if (pending == FTP_BATTERY_LEVEL)
                {
                    if (hindex)
                    {
                        sprintf(buf2,"%s %s level=%02x",what_name,pending_name,p2);
                        ftp_battery_level = p2;
                    }
                    else
                        sprintf(buf2,"%s %s",what_name,pending_name);
                    
                }
                else if (pending == FTP_GET_SENSITIVITY)
                {
                    sprintf(buf2,"%s %s %s=%02x",what_name,pending_name,hindex?"level":"string",p2);
                    if (hindex)
                        ftp_sensitivity[ftp_get_sensitivy_command_string_number] = p2;
                    else
                        ftp_get_sensitivy_command_string_number = p2;
                }
                else if (pending == FTP_SET_SENSITIVITY)
                {
                    int string = p2 >> 4;
                    int level  = p2 & 0xf;
                    sprintf(buf2,"%s %s string=%d level=%d",what_name,pending_name,string,level);
                    if (hindex)
                        ftp_sensitivity[string] = level;
                }
            }
        }
        

        if (show_it)
        {
            char buf[200];
            sprintf(buf,"\033[%dm  %s(%2d)  %02X  %-16s  %02x  %02x  %s",
                color,
                who,
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

    

void showDisplayQueue()
{
    if (display_tail != display_head)
    {
        uint32_t msg = display_queue[display_tail++];
        if (display_tail == MAX_DISPLAY_QUEUE) 
            display_tail = 0;
        processMsg(msg);
    }
}


void enqueueDisplay(uint32_t msg)
{
    // display(0,"enqueueDisplay(%d,%08x)",display_head,msg);

    __disable_irq();
    display_queue[display_head++] = msg;
    if (display_head == MAX_DISPLAY_QUEUE)
        display_head = 0;
    if (display_head == display_tail)
        my_error("expSystem displayQueue overflow at %d",display_head);
    __enable_irq();
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

uint32_t dequeueProcess()
{
    int msg = 0;
    if (process_tail != process_head)
    {
        msg = process_queue[process_tail++];
        if (process_tail == MAX_DISPLAY_QUEUE)
            process_tail = 0;
    }
    if (msg)
        processMsg(msg);
    return msg;
}


