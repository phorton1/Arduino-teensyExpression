#ifndef __prefs_h__
#define __prefs_h__

#include "defines.h"

#define PREF_NONE               -1

//--------------------------------------------------------------------------------
// SETTINGS (PREFERENCES) OPTIONS
//--------------------------------------------------------------------------------
// These define are also the locations in EEPROM of these items

#define PREF_BRIGHTNESS         0           // 1..100 - default(40)
#define PREF_PATCH_NUM          1           // 0..254 - default(1)
#define PREF_DEBUG_PORT         2           // off, USB, Serial - default(USB)
#define PREF_SPOOF_FTP          3           // off, on - default(off)
#define PREF_FTP_PORT           4           // off, Host, Remote, default(Host)

//-----------------------------
// default patch settings
//-----------------------------
// these are the default settings for patches that don't use patchSettings
// every patch change must at least calls setPatchSettings(0)
// POLY_MODE and SPLITS are specific to patches.  Since patch
// settings also include POLY_MODE and SPLITS, we define the offsets here.

#define OFFSET_PATCH_USE_DEFAULT 0          // 0,1 - default = 1 (on patchSettings)
#define OFFSET_FTP_POLY_MODE     1          // 0=mono, 1=poly, default=mono (on patchSettings)

#define OFFSET_FTP_TOUCH_SENS    2          // 0..9 - default(4)
#define OFFSET_FTP_DYN_RANGE     3          // 0..10 - default(10), we map to 0x0A..0x14 (10..20) in usage
#define OFFSET_FTP_DYN_OFFSET    4          // 0..20 - default(10)
#define OFFSET_PERF_FILTER       5          // off, on - default(off), filters all but notes and bends from channel 0
#define OFFSET_PERF_FILTER_BENDS 6          // off, on - default(off), filters bends too.
#define OFFSET_PERF_SPLITS       7          // off, 1+5, 2+3, 3+3, 4+2, 5+1  (number of strings in first split if any)

#define PREF_FTP_PATCH_SETTING   3          // fake skip offset 0 and 1
#define PREF_FTP_TOUCH_SENS      (PREF_FTP_PATCH_SETTING + OFFSET_FTP_TOUCH_SENS)
#define PREF_FTP_DYN_RANGE       (PREF_FTP_PATCH_SETTING + OFFSET_FTP_DYN_RANGE)
#define PREF_FTP_DYN_OFFSET      (PREF_FTP_PATCH_SETTING + OFFSET_FTP_DYN_OFFSET)
#define PREF_PERF_FILTER         (PREF_FTP_PATCH_SETTING + OFFSET_PERF_FILTER)
#define PREF_PERF_FILTER_BENDS   (PREF_FTP_PATCH_SETTING + OFFSET_PERF_FILTER_BENDS)


//-----------------------------
// midi monitor settings
//-----------------------------
// there are a bunch of default setting for the monitor
//     that each of the  ports can override and set individually
// different midi ports can be directed to different serial ports as well
// this section is initially implemented with whatever capabilities I had
//     when I changed this.  These should be reworked to a sensible set
//     for general use ...


#define BYTES_PER_PORT_MONITOR     10       // includes one unused byte

#define PREF_DEFAULT_MIDI_MONITOR  (PREF_FTP_PATCH_SETTING + OFFSET_PERF_SPLITS)        // 10

#define PREF_MONITOR_PORT0         (PREF_DEFAULT_MIDI_MONITOR + BYTES_PER_PORT_MONITOR) // 20

#define PREF_MONITOR_DEVICE_IN0    PREF_MONITOR_PORT0                                   // 20
#define PREF_MONITOR_DEVICE_IN1    (PREF_MONITOR_PORT0 + 1*BYTES_PER_PORT_MONITOR)      // 30
#define PREF_MONITOR_DEVICE_OUT0   (PREF_MONITOR_PORT0 + 2*BYTES_PER_PORT_MONITOR)      // 40
#define PREF_MONITOR_DEVICE_OUT1   (PREF_MONITOR_PORT0 + 3*BYTES_PER_PORT_MONITOR)      // 50
#define PREF_MONITOR_HOST_IN0      (PREF_MONITOR_PORT0 + 4*BYTES_PER_PORT_MONITOR)      // 60
#define PREF_MONITOR_HOST_IN1      (PREF_MONITOR_PORT0 + 5*BYTES_PER_PORT_MONITOR)      // 70
#define PREF_MONITOR_HOST_OUT0     (PREF_MONITOR_PORT0 + 6*BYTES_PER_PORT_MONITOR)      // 80
#define PREF_MONITOR_HOST_OUT1     (PREF_MONITOR_PORT0 + 7*BYTES_PER_PORT_MONITOR)      // 90

#define OFFSET_MIDI_MONITOR               0     // off, USB, Serial, 255=default    default(USB)
#define OFFSET_MONITOR_SHOW_FILTERED      1     // off, on                 default(off)
    // only applies to messages from the host port
#define OFFSET_MONITOR_SYSEX              2     // off, on, Detail         default(2=Detail)
#define OFFSET_MONITOR_ACTIVESENSE        3     // off, on                 default(0==off)
#define OFFSET_MONITOR_PERFORMANCE_CCS    4     // off, on,                default(1=on)
// ftp input and output port only
#define OFFSET_MONITOR_FTP_TUNING_MSGS    5     // off, on                 default(1==on)
#define OFFSET_MONITOR_FTP_NOTE_INFO      6     // off, on                 default(1==on)
#define OFFSET_MONITOR_FTP_VOLUME         7     // off, on                 default(1==on)
#define OFFSET_MONITOR_FTP_BATTERY        8     // off, on                 default(1==on)


#define DEFAULT_PREF_MONITOR            (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MIDI_MONITOR)
#define DEFAULT_PREF_SHOW_FILTERED      (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_SHOW_FILTERED)
#define DEFAULT_PREF_SYSEX              (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_SYSEX)
#define DEFAULT_PREF_ACTIVESENSE        (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_ACTIVESENSE)
#define DEFAULT_PREF_PERFORMANCE_CCS    (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_PERFORMANCE_CCS)
#define DEFAULT_PREF_FTP_TUNING_MSGS    (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_FTP_TUNING_MSGS)
#define DEFAULT_PREF_FTP_NOTE_INFO      (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_FTP_NOTE_INFO)
#define DEFAULT_PREF_FTP_VOLUME         (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_FTP_VOLUME)
#define DEFAULT_PREF_FTP_BATTERY        (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_FTP_BATTERY)


#define PORT_PREF_MONITOR(p)            (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MIDI_MONITOR)
#define PORT_PREF_SHOW_FILTERED(p)      (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_SHOW_FILTERED)
#define PORT_PREF_SYSEX(p)              (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_SYSEX)
#define PORT_PREF_ACTIVESENSE(p)        (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_ACTIVESENSE)
#define PORT_PREF_PERFORMANCE_CCS(p)    (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_PERFORMANCE_CCS)
#define PORT_PREF_FTP_TUNING_MSGS(p)    (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_FTP_TUNING_MSGS)
#define PORT_PREF_FTP_NOTE_INFO(p)      (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + DEFAULT_PREF_FTP_NOTE_INFO)
#define PORT_PREF_FTP_VOLUME(p)         (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_FTP_VOLUME)
#define PORT_PREF_FTP_BATTERY(p)        (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_FTP_BATTERY)


//--------------------------------
// pedals
//--------------------------------

#define PREF_PEDAL_CALIB_MIN_OFFSET     0       // default 0
#define PREF_PEDAL_CALIB_MAX_OFFSET     2       // default 1023
#define PREF_PEDAL_VALUE_MIN_OFFSET     4       // default 0
#define PREF_PEDAL_VALUE_MAX_OFFSET     5       // default 127 (except for pedal 1, loop, which is 92)
#define PREF_PEDAL_CURVE_TYPE_OFFSET    6       // 0=linear, 1=asymptotic, 2=scurve - default(0) == num_points

#define PREF_PEDAL_POINTS_OFFSET        8       // = 8, and there are 2 of what follows

#define PEDAL_POINTS_OFFSET_X           0
#define PEDAL_POINTS_OFFSET_Y           2
#define PEDAL_POINTS_OFFSET_WEIGHT      4
#define PEDAL_POINT_PREF_SIZE           6

#define PREF_BYTES_PER_PEDAL   (PREF_PEDAL_POINTS_OFFSET + MAX_PEDAL_CURVE_POINTS * PEDAL_POINT_PREF_SIZE)   // 20

#define PREF_PEDAL0            (PREF_MONITOR_PORT0 + 8*BYTES_PER_PORT_MONITOR)      // 100
#define PREF_PEDAL(i)          (PREF_PEDAL0 + (i)*PREF_BYTES_PER_PEDAL)             // 100, 120, 140, 160

#define NUM_EEPROM_USED        (PREF_PEDAL0 + NUM_PEDALS*PREF_BYTES_PER_PEDAL)     // 180


//--------------------------------------------------------------------------------
// API
//--------------------------------------------------------------------------------

extern uint8_t prefs[NUM_EEPROM_USED];
    // for inline speed - NOT for use by clients


inline uint8_t  getPref8(int pref)      { return prefs[pref]; }
inline bool     getPrefBool(int pref)   { return (bool) prefs[pref]; }
inline uint16_t getPref16(int pref)     { uint16_t *p=(uint16_t *) &prefs[pref];  return *p; }


extern void init_global_prefs();
extern void save_global_prefs();

extern void setPref8(int pref, uint8_t val);
extern void setPrefBool(int pref, bool val);
extern void setPref16(int pref, uint16_t val);

extern bool prefs_changed();
    // are they changed since last save?
extern void restore_prefs();
    // restore them to last saved state
extern bool pref_changed8(int pref);
extern bool pref_changed16(int pref);


extern uint8_t portMonitorPref(int p, int off);
    // returns the preference setting for a port
    // defering to the default settings if the port
    // has it's main pref set to "default"

extern int16_t getPrefMin(int pref);
extern int16_t getPrefMax(int pref);
extern void    setPrefMax(int pref, int16_t max);

extern const char **getPrefStrings(int pref);
extern void setPrefStrings(int pref, const char *strings[]=0);


#endif
