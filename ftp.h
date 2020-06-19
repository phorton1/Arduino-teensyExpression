#ifndef __ftp_h__
#define __ftp_h__

// This module keeps track of the FTP state (as determined by the stuff in midiQueue.cpp
// The tuner, fretboard display, and sensitivy VU are driven off these values.

#define NUM_STRINGS 6


typedef struct noteStruct
{
    uint8_t val;
    uint8_t vel;                // full velocity from NoteOn event
    uint8_t string;
    uint8_t vel2;               // compressed velocity from NoteInfo event
    
    int fret;                   // note that this concievably could be less than zero with alternate tunings at this time
    int tuning;                 // starts as NO_TUNING_YET

    noteStruct *prev;
    noteStruct *next;
    
}   note_t;




extern note_t *first_note;
extern note_t *last_note;
extern note_t *most_recent_note;
extern note_t *tuning_note;

extern note_t *addNote(uint8_t val, uint8_t vel, uint8_t string, uint8_t vel2);
extern note_t * findNote(uint8_t val, uint8_t string);
extern void deleteNote(uint8_t val, uint8_t string);

extern int ftp_battery_level;                       // -1 == not initialized
extern int ftp_sensitivity[NUM_STRINGS];            // all must be !- -1 or we are not initialized
extern uint8_t ftp_get_sensitivy_command_string_number;    
    // YOU must set this if you send the FTP_GET_SENSITIVITY command
    // yourself ... it will be set automatically if the parser see
    // the command come from the FTP editor, but it is needed to
    // figure out what the 

// methods

extern void initFTPifNeeded(bool force=0);
    // set ftp_battery level to -1 and call this for keep-alive probe.
    // will get sensitivities for any strings that don't have them

extern const char *getFTPCommandName(uint8_t p2);
extern void sendGetFTPSensitivityCommand(uint8_t string);
    // will send the command and set the following value for you

extern const char *noteName(uint8_t note);




#endif  // !__ftp_h__
