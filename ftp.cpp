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
    sendFTPCommandAndValue(FTP_CMD_GET_SENSITIVITY, string);
}



const char *getFTPCommandName(uint8_t p2)
{
    if (p2 == FTP_CMD_EDITOR_MODE           ) return "EDITOR_MODE";                 // 0x04
    if (p2 == FTP_CMD_SLIDER_POSITION       ) return "SLIDER_POSITION";             // 0x05
    if (p2 == FTP_CMD_BATTERY_LEVEL         ) return "BATTERY_LEVEL";               // 0x07
    if (p2 == FTP_CMD_VOLUME_LEVEL          ) return "VOLUME_LEVEL";                // 0x08
    if (p2 == FTP_CMD_DYNAMICS_SENSITIVITY  ) return "DYNAMICS_SENSITIVITY";        // 0x2F
    if (p2 == FTP_CMD_SET_PGM_NUMBER        ) return "SET_PGM_NUMBER";              // 0x28
    if (p2 == FTP_CMD_SET_BANK_LSB          ) return "SET_BANK_LSB";                // 0x29
    if (p2 == FTP_CMD_SET_BANK_MSB          ) return "SET_BANK_MSB";                // 0x2A
    if (p2 == FTP_CMD_PITCHBEND_MODE        ) return "PITCHBEND_MODE";              // 0x2B
    if (p2 == FTP_CMD_TRANSPOSE             ) return "TRANSPOSE";                   // 0x2C
    if (p2 == FTP_CMD_MIDI_VOLUME           ) return "MIDI_VOLUME";                 // 0x2D
    if (p2 == FTP_CMD_DYNAMICS_OFFSET       ) return "DYNAMICS_OFFSET";             // 0x30
    if (p2 == FTP_CMD_MIDI_REVERB           ) return "MIDI_REVERB";                 // 0x31
    if (p2 == FTP_CMD_GET_SENSITIVITY       ) return "GET_SENSITIVITY";             // 0x3C
    if (p2 == FTP_CMD_POLYMODE              ) return "POLYMODE";                    // 0x3f
    if (p2 == FTP_CMD_SET_SENSITIVITY       ) return "SET_SENSITIVITY";             // 0x42
    if (p2 == FTP_CMD_BLOCK_MIDI_NOTES      ) return "BLOCK_MIDI_NOTES";            // 0x46
    if (p2 == FTP_CMD_TOUCH_SENSITIVITY     ) return "TOUCH_SENSITIVITY";           // 0x4f
    if (p2 == FTP_CMD_SPLIT_NUMBER          ) return "SPLIT_NUMBER";                // 0x52
    
    return "unknownFTPCommand";
}



const char *note_names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
    

const char *noteName(uint8_t note)
{
    return note_names[note % 12];
}


note_t *addNote(uint8_t val, uint8_t vel, uint8_t string, uint8_t vel2)
    // -1 is an invalid fret
    // zero is an invalid note
{
    note_t *note = new note_t;
    note->val = val;                
    note->vel = vel;
    note->vel2 = vel2;
    note->fret = val ? ((int) val) - string_base_notes[string] : -1;
    note->string = string;
    note->tuning = 0;
    note->next = 0;
    
    __disable_irq();
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
    __enable_irq();

    return note;
}




void deleteNote(uint8_t string)
{
    __disable_irq();
    note_t *note = first_note;
    while (note && note->string != string)
    {
        note = note->next;
    }
    if (note)
    {
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
    }
    else
    {
        warning(0,"could not find note on string(%d) to delete",string);
    }
    __enable_irq();
    delete note;
}





bool ftp_initialized = 0;

void initFTPifNeeded(bool force)
{
    if (force || !ftp_initialized)
    {
        ftp_initialized = 0;
        sendFTPCommandAndValue(FTP_CMD_EDITOR_MODE, 0x02);
    }
    if (force || ftp_battery_level == -1)
    {
        sendFTPCommandAndValue(FTP_CMD_BATTERY_LEVEL, 0);
    }

    for (int i=0; i<NUM_STRINGS; i++)
    {
        if (force || ftp_sensitivity[i] == -1)
        {
            sendGetFTPSensitivityCommand(i);
        }
    }
}




