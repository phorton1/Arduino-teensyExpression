#include "myDebug.h"
#include "midiQueue.h"

#define MAX_DISPLAY_QUEUE  8192
#define MAX_PROCESS_QUEUE  8192
#define MAX_SYSEX_BUFFER   1024

// TUNING MESSAGES:
//
// Following the NoteOn/1E message is usually a single B7 1D yy
// and a series of B7 3D yy messages.  These are generally tuning
// messages, where yy is the 0x40 biased signed number (from -0x40
// to 0x40).
//
// I call the 1D message a "SetTuning" message as it appears to
// set the tunning relative to the most recent NoteOn/Off message,
// whereas the 3D messages appear to be continuations, which I just
// call "Tuning" messages.   It is not invariant that a NoteOn or
// NoteOff is followed by a 1D ... the tuner will keep working on
// a given string if multiple strings are picked.
//
// Therefore the "tuning" message "keep" a pointer to the note they
// refer to.  Since my Notes are deleted at NoteOff messages, a DD
// from a NoteOff could end up pointing at a deleted message, so we
// check for that, and just assume they are "off" and paint them in red.


int display_head = 0;
int display_tail = 0;
int process_head = 0;
int process_tail = 0;

int sysex_buflen[2] = {0,0};
bool sysex_buf_ready[2] = {0,0};
int expected[2] = {0,0};


uint32_t display_queue[MAX_DISPLAY_QUEUE];
uint32_t process_queue[MAX_PROCESS_QUEUE];
uint8_t sysex_buffer[2][MAX_SYSEX_BUFFER];

int  showSysex = 1; 
bool showActiveSense = 0;
bool showTuningMessages = 1;
bool showNoteInfoMessages = 1;



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




typedef struct noteStruct
{
    uint8_t val;
    uint8_t vel;
    uint8_t string;
    
    int     tuning;
    
    noteStruct *prev;
    noteStruct *next;
    
}   note_t;



note_t *first_note = 0;
note_t *last_note = 0;
note_t *most_recent_note = 0;
note_t *tuning_note = 0;

uint8_t most_recent_note_val;
uint8_t most_recent_note_vel;
    // note kept for one following message
bool    prev_was_1E = 0;
    // B7 1E xy always seems to be followed by a B7 1D tt message
    // This asserts it.
    

note_t *addNote(uint8_t val, uint8_t vel, uint8_t string)
{
    note_t *note = new note_t;
    note->val = val;
    note->vel = vel;
    note->string = string;
    note->tuning = 0;
    note->next = 0;
    
    if (!first_note)
        first_note = note;
    if (last_note)
    {
        last_note->next = note;
        note->prev = last_note;
    }
    else
        note->prev = 0;
    last_note = note;
    most_recent_note = note;
    return note;
}


note_t * findNote(uint8_t val, uint8_t string)
{
    note_t *note = first_note;
    while (note)
    {
        if (note->val == val && note->string == string)
            return note;
        note = note->next;
    }
    return 0;
}

void deleteNote(uint8_t val, uint8_t string)
{
    note_t *note = findNote(val,string);
    if (!note)
    {
        warning(0,"could not find note(%d,%d) to delete",val,string);
        return;
    }
    if (note->prev)
        note->prev->next = note->next;
    if (note->next)
        note->next->prev = note->prev;
    if (note == first_note)
        first_note = note->next;
    if (note == last_note)
        last_note = note->prev;
        
    most_recent_note = 0;
    tuning_note = 0;
    delete note;
}


bool myAssert(bool condition, const char *msg)
{
    if (!condition)
        warning(0,msg,0);
    return condition;
}


#define WITH_1E_VELOCITY_MAPPING   0

#if WITH_1E_VELOCITY_MAPPING
    // This code derives the mapping between the "B7 1E xy" "NoteInfo" CC messages
    // that come from the FTP controller to the NoteOn or Off messages that alwyas
    // immediately precede them.  Every NoteOn or NoteOff message is immediately
    // followed by one of these where "x" is the string number, and "y" is the
    // level to be shown on the "Sensitivity VU meter" for that string.
    //
    // Each of the 15 possible y's, map directly to the previous NoteOn/Off velocity
    // in a nearly invariant manner.  In several hours of testing I only saw one time
    // they did not consistently map, but it did happen that one of the "breakpoints"
    // changed (the breakpoint for level 8 change from velocity 92 to 93 once).
    //
    // Nonetheless, for posterities sake, here is the mapping derived by the following code.
    // If the velocity (bottom row) is above the breakpoint for the "y" value, it is in the bucket.
    // Note that 0 is only returned after "NoteOff" which has a velocity of zero.
    //
    //           Y:   0    1    2    3    4    5    6    7    8    9    10    11    12    13    14    15
    //  NoteOn Vel:   0    1   22   43   58   69   78   86   93   99   104   108   113   117   120   124
    //    or maybe    ^       (23 was not exactly            92
    //                |        determined ... never 
    //      NoteOff   +        got a '22' in testing) changed ^
    //
    // The curve is not linear, here are the deltas
    //
    //                         21  21   15    11    9    8    7    6     5    4     5     4     3     4
    //
    // I think the main purport of this message is that it is used to drive the sensitivity VU meter,
    // which has 32 incrments on the display, but only "lights" up in increments of 2.  Also note that
    // the FTP editor VU meter may at first seem "active" but really it lights up to a single value
    // between the NoteOn and NoteOff messages and does not move while the note is decaying on the guitar.
    

uint8_t velmap[128];
bool velmap_inited = false;
elapsedMillis velmap_idle = 0;
bool velmap_changed = 0;


void addVel(uint8_t note_vel, uint8_t vel)
{
    if (!myAssert(note_vel < 128,"bad addVel"))
        return;
    if (!velmap_inited)
    {
        velmap_inited = 1;
        for (int i=0; i<128; i++) velmap[i] = 0;
    }
    if (velmap[note_vel] && velmap[note_vel] != vel)
    {
        display(0,"velmap(%d) changing from %d to %d",note_vel,velmap[note_vel],vel);
    }
    velmap[note_vel] = vel;
    velmap_idle = 0;
    velmap_changed = 1;
}


void showVelmap()
{
    if (velmap_changed && velmap_idle > 10000)
    {
        velmap_changed = 0;
        int counter = 0;
        for (int i=0; i<127; i++)
        {
            if (velmap[i])
            {
                if (counter % 10 == 0) Serial.println();
                counter++;
                Serial.print(i);
                Serial.print(":");
                Serial.print(velmap[i]);
                Serial.print("  ");
            }
        }
        Serial.println();
    }
}

#endif      // WITH_1E_VELOCITY_MAPPING


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
    
    uint8_t note_val = 0;   // note on or off parameters during this call
    uint8_t note_vel = 0;
    bool    is_1E = 0;      // if the current msg is a 1E we set it in the global for assert
    
    #if WITH_1E_VELOCITY_MAPPING
        showVelmap();
    #endif

    
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
                 myAssert(p0 == 0xf0,"sysex does not start with F0");
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

        if (is_done)
        {
            myAssert(sysex_buffer[hindex][*buflen-1] == 0xf7,"sysex does not end with F7");
        }
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
    
    // NON-REALTIME CHANNEL MESSAGES
    
    
	else 
    {
        if (type == 0x08)
        {
            s = "Note Off";
            color = ansi_color_light_red;
            note_val = msg.b[2];
            note_vel = msg.b[3];
        }
        else if (type == 0x09)
        {
            s = "Note On";
            color = ansi_color_light_blue;
            note_val = msg.b[2];
            note_vel = msg.b[3];
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
            sprintf(buf2,"        %d",value);
            color = ansi_color_light_gray;  // understood
        }
        else if (msg.isActiveSense())
        {
            s = "ActiveSense";
            color = ansi_color_light_gray;  // understood
            // keep all globals the same
            
            note_val = most_recent_note_val;
            note_vel = most_recent_note_vel;
            is_1E = prev_was_1E;
        }
        else if (type == 0x0b)
        {
            s = "ControlChange";
            
            if (p1 == 0x1e)     // NoteInfo referring to previous note
            {
                is_1E = 1;
                s = "NoteInfo";
                show_it = showNoteInfoMessages;

                uint8_t string = p2>>4;
                uint8_t vel = p2 & 0x0f;
                color = most_recent_note_vel ? ansi_color_light_blue : ansi_color_light_red;
                sprintf(buf2,"%02x  %02x  string=%d vel=%d",p1,p2,string,vel);
                if (myAssert(most_recent_note_val,"B7 1E not following a note!!") &&
                    myAssert(most_recent_note_vel || !vel,"expected 1E to have zero vel following note off"))
                {
                    if (most_recent_note_vel)
                    {
                        addNote(most_recent_note_val,most_recent_note_vel,string);
                        #if WITH_1E_VELOCITY_MAPPING
                            addVel(most_recent_note_vel, vel);
                        #endif
                    }
                    else
                    {
                        deleteNote(most_recent_note_val,string);
                    }
                }
            }
            else if (p1 == 0x1D || p1 == 0x3D)    // tuning message
            {
                s = "Tuning";
                color = ansi_color_light_gray;
                show_it = showTuningMessages;
                
                if (p1 == 0x1D)
                {
                    s = "SetTuning";
                    tuning_note = most_recent_note;
                    color = tuning_note ? ansi_color_light_blue : ansi_color_light_red;
                }
                
                // 0x00 = -40,  0x40 == 0, 0x80 == +40
                int tuning = ((int) p2) - 0x40;      // 40 == 0,  0==-
                sprintf(buf2,"%02x  %02x  tuning=%d",p1,p2,tuning);
            }
            
        }
        

        if (show_it)
        {
            char buf[100];
            if (buf2[0])
            {
                sprintf(buf,"\033[%dm  %s(%2d)  %02X  %-16s  %s",
                    color,
                    who,
                    msg.getChannel(),
                    p0,
                    s,
                    buf2);
            }
            else
            {
                sprintf(buf,"\033[%dm  %s(%2d)  %02X  %-16s  %02x  %02x",
                    color,
                    who,
                    msg.getChannel(),
                    p0,
                    s,
                    p1,
                    p2);
            }
            Serial.println(buf);
        }
    }
    
    most_recent_note_val = note_val;
    most_recent_note_vel = note_vel;

}

    

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
    return msg;
}


