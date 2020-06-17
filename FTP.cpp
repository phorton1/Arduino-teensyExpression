#include <myDebug.h>
#include "FTP.h"


note_t *first_note = 0;
note_t *last_note = 0;
note_t *most_recent_note = 0;
note_t *tuning_note = 0;


    
const int  string_base_notes[6] = {0x40, 0x3b, 0x37, 0x32, 0x2d, 0x28};

    

note_t *addNote(uint8_t val, uint8_t vel, uint8_t string)
{
    note_t *note = new note_t;
    note->val = val;
    note->vel = vel;
    note->fret = ((int) val) - string_base_notes[string];
    note->string = string;
    note->tuning = NO_TUNING_YET;
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
        
    if (note == most_recent_note)
        most_recent_note = 0;
        
    if (note == tuning_note)
        tuning_note = 0;    

    delete note;
}








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
