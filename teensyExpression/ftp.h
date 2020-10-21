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

// the battery level is read only
//
// a value of other than -1 indicates that the ftp controller has been "found",
// which triggers other things (like an initial readback of the sensitivity
// settings), and THEN an initialization of the FTP settings from EEPROM

extern int  ftp_battery_level;                       // -1 == not initialized

// stored on controller

extern int  ftp_sensitivity[NUM_STRINGS];            // all must be !- -1 or we are not initialized

// stored in EEPROM (separate from preferences)
// In this code I treat these as global settings.
//
// They have default values based on the default settings in the controller
// factory patch 0-0 (Polymode Patch 1).
//
// If the values in the EEPROM are different than those defaults, AND
// an FTP controller is located remotely or in the host port (as per prefs),
// the initial EEPROM settings will be sent to the controller.
//
// The public values here are READONLY and ONLY gotten from readbacks
// (replies from the controller). They are "gotten" by midiQueue.cpp,
// and stored here (in ftp.cpp) so that ftp.cpp is the central repository
// of FTP state.

extern int ftp_poly_mode;                           // default=true
extern int ftp_bend_mode;                           // default=0, auto
extern int ftp_dynamic_range;                       // value 10..20, default 20
extern int ftp_dynamic_offset;                      // value 0..20, default 10
extern int ftp_touch_sensitivity;                   // value 0..9, default 4

// methods used by midiQueue to understand and keep track of tuning state
// and for display purposes

extern note_t *addNote(uint8_t val, uint8_t vel, uint8_t string, uint8_t vel2);
extern void deleteNote(uint8_t string);
extern const char *noteName(uint8_t note);
extern const char *getFTPCommandName(uint8_t p2);


// in midiQueue.cpp

extern void sendFTPCommandAndValue(uint8_t command, uint8_t value);
    // send command and value, with with reply processing and
    // retries in dequeueProcess.

// Only used by midiQueue.cpp

extern bool showFtpPatch(
    Stream *out_stream,
    int color,
    int bg_color,
    bool is_ftp_controller,
    uint8_t *patch_buf,
    uint32_t buflen);

// called frequently from expSystem.cpp

void initQueryFTP();
    // IFF there is an FTP port specified as per preferences ...
    //
    // sends out the battery level request every 5 seconds initially
    // for 30 seconds after boot, then every 15 seconds until an FTP
    // controller is found.
    //
    // If the controller is found (on the expected port), the battery
    // indicator should light up, and then this method will perform a
    // one time "send" of the above EEPROM based settings to the FTP
    // controller, which *should* result in readback replies for each
    // of the EEPROM based settings.
    //
    // After that this method does nothing.


#endif  // !__ftp_h__
