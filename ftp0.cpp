#include <myDebug.h>
#include "ftp.h"
#include "ftp_defs.h"
#include "myMidiHost.h"
#include "midiQueue.h"


note_t *first_note = 0;
note_t *last_note = 0;
note_t *most_recent_note = 0;
note_t *tuning_note = 0;

int ftp_battery_level = -1;
int ftp_sensitivity[NUM_STRINGS] = {-1,-1,-1,-1,-1,-1};   
uint8_t ftp_get_sensitivy_command_string_number = 0;

const int  string_base_notes[6] = {0x40, 0x3b, 0x37, 0x32, 0x2d, 0x28};




void sendGetFTPSensitivityCommand(uint8_t string)
{
    display(0,"sendGetFTPSensitivityCommand(%d)",string);
    sendFTPCommandAndValue(FTP_GET_SENSITIVITY, string);
}



const char *getFTPCommandName(uint8_t p2)
{
    if (p2==FTP_SLIDER_POSITION ) return "SLIDER_POSITION";    
    if (p2==FTP_BATTERY_LEVEL   ) return "BATTERY_LEVEL";    
    if (p2==FTP_VOLUME_LEVEL    ) return "VOLUME_LEVEL";    
    if (p2==FTP_GET_SENSITIVITY ) return "GET_SENSITIVITY";
    if (p2==FTP_SET_SENSITIVITY ) return "SET_SENSITIVITY";
    return "unknownFTPCommand";
}



const char *note_names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    

const char *noteName(uint8_t note)
{
    return note_names[note % 12];
}


note_t *addNote(uint8_t val, uint8_t vel, uint8_t string, uint8_t vel2)
{
    note_t *note = new note_t;
    note->val = val;
    note->vel = vel;
    note->vel2 = vel2;
    note->fret = ((int) val) - string_base_notes[string];
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
        
    if (note == most_recent_note)
        most_recent_note = 0;
        
    if (note == tuning_note)
        tuning_note = 0;    

    delete note;
}






void initFTPifNeeded(bool force)
{
    if (force || ftp_battery_level == -1)
    {
        sendFTPCommandAndValue(FTP_BATTERY_LEVEL, 0);
    }

    for (int i=0; i<NUM_STRINGS; i++)
    {
        if (force || ftp_sensitivity[i] == -1)
        {
            sendGetFTPSensitivityCommand(i);
        }
    }
}




