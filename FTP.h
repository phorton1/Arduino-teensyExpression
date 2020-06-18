#ifndef __FTP_h__
#define __FTP_h__

// This module keeps track of the FTP state (as determined by the stuff in midiQueue.cpp)

#define NO_TUNING_YET    -1000


typedef struct noteStruct
{
    uint8_t val;
    uint8_t vel;
    uint8_t string;

    int fret;                   // note that this concievably could be less than zero with alternate tunings at this time
    int tuning;                 // starts as NO_TUNING_YET

    noteStruct *prev;
    noteStruct *next;
    
}   note_t;



extern note_t *first_note;
extern note_t *last_note;
extern note_t *most_recent_note;
extern note_t *tuning_note;

extern note_t *addNote(uint8_t val, uint8_t vel, uint8_t string);
extern note_t * findNote(uint8_t val, uint8_t string);
extern void deleteNote(uint8_t val, uint8_t string);

#endif  // !__FTP_h__
