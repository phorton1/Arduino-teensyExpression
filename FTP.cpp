#include <myDebug.h>
#include "FTP.h"
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

extern void sendFTPCommandAndValue(uint8_t command, uint8_t value)
{
    msgUnion msg(
        0x0B,
        0xB7,
        FTP_COMMAND_OR_REPLY,    // 0x1f
        command);
    
    midi1.write_packed(msg.i);

    msg.b[2] = FTP_COMMAND_VALUE;       // 0x3f
    msg.b[3] = value;
    
    midi1.write_packed(msg.i);
}



extern void sendGetFTPSensitivityCommand(uint8_t string)
{
    ftp_get_sensitivy_command_string_number = string;
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

    

note_t *addNote(uint8_t val, uint8_t vel, uint8_t string)
{
    note_t *note = new note_t;
    note->val = val;
    note->vel = vel;
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





bool it_failed = 0;

void initFTPifNeeded(bool force)
{
    if (it_failed)
        return;
    
    if (force || ftp_battery_level == -11)
        sendFTPCommandAndValue(FTP_BATTERY_LEVEL, 0);

    for (int i=0; i<NUM_STRINGS; i++)
    {
        if (force || ftp_sensitivity[i] == -1)
        {
            sendGetFTPSensitivityCommand(i);
            elapsedMillis timeout = 0;
            while (ftp_sensitivity[i] == -1 && timeout < 1200)
            {
                delay(100);
            }
            if (ftp_sensitivity[i] == -1)
            {
                my_error("timeout trying to get sensitivity for string %d",i);
                it_failed = 1;
                return;
            }
        }            
    }
}




