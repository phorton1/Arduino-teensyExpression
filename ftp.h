#ifndef __ftp_h__
#define __ftp_h__

#include <Arduino.h>

// This module keeps track of the FTP state (as determined by the stuff in midiQueue.cpp
// The tuner, fretboard display, and sensitivy VU are driven off these values.

#define NUM_STRINGS 6


typedef struct noteStruct
{
    uint8_t val;                // zero is an invalid note
    uint8_t vel;                // full velocity from NoteOn event
    uint8_t string;
    uint8_t vel2;               // compressed velocity from NoteInfo event

    int fret;                   // this concievably could be less than zero with alternate tunings
                                // but generally, fret<0 is invalid for our purposes
    int tuning;                 // starts as NO_TUNING_YET

    noteStruct *prev;
    noteStruct *next;

}   note_t;



// maintained state

extern note_t *first_note;
extern note_t *last_note;
extern note_t *most_recent_note;
extern note_t *tuning_note;

extern int  ftp_battery_level;                       // -1 == not initialized
extern int  ftp_sensitivity[NUM_STRINGS];            // all must be !- -1 or we are not initialized
extern int  ftp_poly_mode;                           // default=true
extern int  ftp_bend_mode;                           // default=0, auto

extern note_t *addNote(uint8_t val, uint8_t vel, uint8_t string, uint8_t vel2);
extern void deleteNote(uint8_t string);

extern const char *noteName(uint8_t note);
extern const char *getFTPCommandName(uint8_t p2);



// in midiQueue.cpp

extern void sendFTPCommandAndValue(uint8_t command, uint8_t value);
    // send command and value, with with reply processing and
    // retries in dequeueProcess.

// used by midiQueue.cpp

extern bool showFtpPatch(
    Stream *out_stream,
    int color,
    int bg_color,
    bool is_ftp_controller,
    uint8_t *patch_buf,
    uint32_t buflen);



#endif  // !__ftp_h__
