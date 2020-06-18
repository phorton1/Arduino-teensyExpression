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



