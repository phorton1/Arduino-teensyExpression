//--------------------------------
// ftp.h
//--------------------------------
// This module keeps track of the FTP state (as determined by the stuff in midiQueue.cpp
// The tuner, fretboard display, and sensitivy VU are driven off these values.

#pragma once

#include <Arduino.h>

#define NUM_STRINGS 6


//-----------------------------
// notes
//-----------------------------

typedef struct noteStruct
{
    uint8_t val;                // zero is an invalid note
    uint8_t vel;                // full velocity from NoteOn event
    uint8_t string;
    uint8_t vel2;               // compressed velocity from NoteInfo event
    int fret;                   // could be less than zero with alternate tunings
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

// called from midiQueue

extern note_t *addNote(uint8_t val, uint8_t vel, uint8_t string, uint8_t vel2);
extern void deleteNote(uint8_t string);

// called from Tuner

extern const char *noteName(uint8_t note);


//-----------------------------------------------------
// ftp state
//-----------------------------------------------------

extern int  ftp_battery_level;                       // -1 == not initialized
    // The battery level is readonly from 0x40 (empty) to 0x6f (full).
    // A value of other than -1 indicates that the ftp controller has been "found",
    // which triggers initQueryFTP() to do an initialization sequence.

extern int  ftp_sensitivity[NUM_STRINGS];
    // Stored on the controller, with values 0..15, and initialized by initQueryFTP().
    // The public values here are READONLY and ONLY gotten from readbacks
    // (replies from the controller). They are "gotten" by midiQueue.cpp,
    // and stored here (in ftp.cpp) so that ftp.cpp is the central repository
    // of FTP state.

extern int ftp_poly_mode;                           // default=true
extern int ftp_bend_mode;                           // default=0, auto

extern int ftp_dynamic_range;                       // value 10..20, default 20
extern int ftp_dynamic_offset;                      // value 0..20, default 10
extern int ftp_touch_sensitivity;                   // value 0..9, default 4
    // These three parameters are set to the above defaults by initQueryFTP(),
    // although they may be modified for experimentation, per teeensyExpression
    // boot, by the winFTPSensitivity dialog()


// called frequently from expSystem.cpp

void initQueryFTP();
    // IFF there is an FTP port specified as per preferences,
    // sends out the battery level request every 5 seconds initially
    // for 30 seconds after boot, then every 15 seconds until an FTP
    // controller is found.
    //
    // If the controller is found (on the expected port), the battery
    // indicator should light up, and then this method will perform a
    // one time "initialization" of the ftp to read the senstivity settings
    // and set a limited set of parameters ("send" of the above EEPROM based settings to the FTP
    // controller, which *should* result in readback replies for each
    // of the EEPROM based settings.
    //
    // After that this method does nothing.



// Only used by midiQueue.cpp

extern const char *getFTPCommandName(uint8_t p2);
    // called by midiQueue's midiMonitor

extern bool showFtpPatch(
    Stream *out_stream,
    int color,
    int bg_color,
    bool is_ftp_controller,
    uint8_t *patch_buf,
    uint32_t buflen);

//---------------------------------------------
// for use by ftp.cpp, in midiQueue.cpp
//----------------------------------------------

extern int pendingFTPCount();
    // returns number of pending FTPCommandAndValues that have ben
    // enqueued, but not yet resolved. Used to throttle the init sequence
extern void sendFTPCommandAndValue(uint8_t command, uint8_t value);
    // send command and value, with with reply processing and
    // retries in dequeueProcess.


