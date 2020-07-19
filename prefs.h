#ifndef __prefs_h__
#define __prefs_h__

#include "defines.h"

#define PREF_NONE               -1

//  values for PREF_FTP_PORT

#define FTP_PORT_OFF            0
#define FTP_PORT_HOST           1
#define FTP_PORT_REMOTE         2

//--------------------------------------------------------------------------------
// SETTINGS (PREFERENCES) OPTIONS
//--------------------------------------------------------------------------------
// These define are also the locations in EEPROM of these items

#define PREF_BRIGHTNESS         0           // 1..100 - default(40)
#define PREF_PATCH_NUM          1           // 0..254 - default(1)
#define PREF_DEBUG_PORT         2           // off, USB, Serial - default(USB)
#define PREF_SPOOF_FTP          3           // off, on - default(off)
#define PREF_FTP_PORT           4           // off, Host, Remote, default(Host)

#define FTP_OUTPUT_PORT    (getPref8(PREF_SPOOF_FTP) ? 1 : getPref8(PREF_FTP_PORT))


//--------------------------------
// pedals
//--------------------------------

#define PREF_PEDAL0    (PREF_FTP_PORT + 2)      // skip a byte from header == 6 at this time

#define MAX_PEDAL_CURVES                3       // number of curves per pedal
#define MAX_CURVE_POINTS                4       // number of points per curve
#define MAX_PEDAL_CURVE_POINTS          (MAX_PEDAL_CURVES * MAX_CURVE_POINTS)

#define PREF_PEDAL_AUTO_OFFSET          0       // default 0, 1 for pedal #1 (loop)
#define PREF_PEDAL_CURVE_TYPE_OFFSET    1       // 0=linear, 1=asymptotic, 2=scurve - default(0) == num_points
#define PREF_PEDAL_CALIB_MIN_OFFSET     2       // default 0
#define PREF_PEDAL_CALIB_MAX_OFFSET     4       // default 1023

// each pedal maintains 3 distinct curves consisiting of upto 4 points
// where the MIN is always the 0th point, and the MAX is always the
// CURVE_TYPE + 1'th point. On those two "special points", the x values
// are fixed at 0 and 127, respecitvely, and the y position is used
// in my current pedal handling.

#define PREF_PEDAL_POINTS_OFFSET        6       // word boundry

#define PEDAL_POINTS_OFFSET_X           0
#define PEDAL_POINTS_OFFSET_Y           1
#define PEDAL_POINTS_OFFSET_WEIGHT      2
#define PEDAL_POINT_PREF_SIZE           4       // word boundry

#define PREF_BYTES_PER_CURVE   (MAX_CURVE_POINTS * PEDAL_POINT_PREF_SIZE)
#define CURVE_BYTES_PER_PEDAL  (MAX_PEDAL_CURVES * PREF_BYTES_PER_CURVE)
#define PREF_BYTES_PER_PEDAL   (PREF_PEDAL_POINTS_OFFSET + CURVE_BYTES_PER_PEDAL)
#define PREF_PEDAL(i)          (PREF_PEDAL0 + (i)*PREF_BYTES_PER_PEDAL)

#define PREF_PEDAL_AUTO(p)              (PREF_PEDAL(p) + PREF_PEDAL_AUTO_OFFSET)
#define PREF_PEDAL_CURVE_TYPE(p)        (PREF_PEDAL(p) + PREF_PEDAL_CURVE_TYPE_OFFSET)
#define PREF_PEDAL_CURVE(p,c)           (PREF_PEDAL(p) + PREF_PEDAL_POINTS_OFFSET + (c)*PREF_BYTES_PER_CURVE)
#define PREF_PEDAL_CURVE_POINT(p,c,i)   (PREF_PEDAL_CURVE(p,c) + (i)*PEDAL_POINT_PREF_SIZE)

#define getPrefPedalAuto(p)             (getPref8(PREF_PEDAL_AUTO(p)))
#define getPrefPedalCurve(p)            (getPref8(PREF_PEDAL_CURVE_TYPE(p)))
#define getPrefPedalMin(p)              (getPref8(PREF_PEDAL_CURVE_POINT(p,getPrefPedalCurve(p),0) + PEDAL_POINTS_OFFSET_Y))
#define getPrefPedalMax(p)              (getPref8(PREF_PEDAL_CURVE_POINT(p,getPrefPedalCurve(p),getPrefPedalCurve(p)+1) + PEDAL_POINTS_OFFSET_Y))
#define getPrefPedalCalibMin(p)         (getPref16(PREF_PEDAL(p) + PREF_PEDAL_CALIB_MIN_OFFSET))
#define getPrefPedalCalibMax(p)         (getPref16(PREF_PEDAL(p) + PREF_PEDAL_CALIB_MAX_OFFSET))

#define setPrefPedalAuto(p,i)           setPref8(PREF_PEDAL_AUTO(p),(i))
#define setPrefPedalCurve(p,i)          setPref8(PREF_PEDAL_CURVE_TYPE(p),(i))
#define setPrefPedalCalibMin(p,i)       setPref16(PREF_PEDAL(p) + PREF_PEDAL_CALIB_MIN_OFFSET, (i))
#define setPrefPedalCalibMax(p,i)       setPref16(PREF_PEDAL(p) + PREF_PEDAL_CALIB_MAX_OFFSET, (i))


//-----------------------------
// midi monitor settings
//-----------------------------

#define PREF_MIDI_MONITOR    PREF_PEDAL(NUM_PEDALS + 1)   // off, DebugPort, USB, Serial   default(DebugPort)

#define PREF_MONITOR_PORT0          (PREF_MIDI_MONITOR+1)      // 0=DUINO_INPUT0    default(off)

#define PREF_MONITOR_DUINO_INPUT0   (PREF_MONITOR_PORT0+0)     // 0=DUINO_INPUT0    default(off)
#define PREF_MONITOR_DUINO_INPUT1   (PREF_MONITOR_PORT0+1)     // 1=DUINO_INPUT1    default(on)
#define PREF_MONITOR_DUINO_OUTPUT0  (PREF_MONITOR_PORT0+2)     // 2=DUINO_OUTPUT0   default(on)
#define PREF_MONITOR_DUINO_OUTPUT1  (PREF_MONITOR_PORT0+3)     // 3=DUINO_OUTPUT1   default(on)
#define PREF_MONITOR_HOST_INPUT0    (PREF_MONITOR_PORT0+4)     // 4=HOST_INPUT0     default(off)
#define PREF_MONITOR_HOST_INPUT1    (PREF_MONITOR_PORT0+5)     // 5=HOST_INPUT1     default(on)
#define PREF_MONITOR_HOST_OUTPUT0   (PREF_MONITOR_PORT0+6)     // 6=HOST_OUTPUT0    default(off)
#define PREF_MONITOR_HOST_OUTPUT1   (PREF_MONITOR_PORT0+7)     // 7=HOST_OUTPUT1    default(on)
    // The above prefs determine whether or not messages will be enqueued for
    // ports besides the FTP Controller port.  So it must be rechecked in the
    // monitor display method as well in case we don't want to SEE the FTP
    // controller messages.

#define PREF_MONITOR_CHANNEL1       (PREF_MONITOR_PORT0+8)      // monitor specific midi channels
    // default:  all channels are monitored

#define PREF_MONITOR_SYSEX              (PREF_MONITOR_CHANNEL1+16)     // off, on, Detail         default(2=Detail)
#define PREF_MONITOR_ACTIVESENSE        (PREF_MONITOR_SYSEX+1)         // off, on                 default(0==off)
#define PREF_MONITOR_NOTE_ON            (PREF_MONITOR_SYSEX+2)
#define PREF_MONITOR_NOTE_OFF           (PREF_MONITOR_SYSEX+3)
#define PREF_MONITOR_VELOCITY           (PREF_MONITOR_SYSEX+4)      // "after touch poly"
#define PREF_MONITOR_PROGRAM_CHG        (PREF_MONITOR_SYSEX+5)
#define PREF_MONITOR_AFTERTOUCH         (PREF_MONITOR_SYSEX+6)
#define PREF_MONITOR_PITCHBEND          (PREF_MONITOR_SYSEX+7)

// ftp specifics
// one or both of the MONITOR_FTP_COMMANDS or VALUES must be ON
// to see specific FTP commadns and replies

#define PREF_MONITOR_FTP                (PREF_MONITOR_SYSEX+8)
#define PREF_MONITOR_PARSE_FTP_PATCHES  (PREF_MONITOR_FTP+0)
#define PREF_MONITOR_FTP_NOTE_INFO      (PREF_MONITOR_FTP+1)
#define PREF_MONITOR_FTP_TUNING_MSGS    (PREF_MONITOR_FTP+2)
#define PREF_MONITOR_FTP_COMMANDS       (PREF_MONITOR_FTP+3)
#define PREF_MONITOR_FTP_VALUES         (PREF_MONITOR_FTP+4)
#define PREF_MONITOR_FTP_POLY_MODE      (PREF_MONITOR_FTP+5)
#define PREF_MONITOR_FTP_BEND_MODE      (PREF_MONITOR_FTP+6)
#define PREF_MONITOR_FTP_VOLUME         (PREF_MONITOR_FTP+7)
#define PREF_MONITOR_FTP_BATTERY        (PREF_MONITOR_FTP+8)
#define PREF_MONITOR_FTP_SENSITIVITY    (PREF_MONITOR_FTP+9)
#define PREF_MONITOR_KNOWN_FTP_COMMANDS     (PREF_MONITOR_FTP+10)
#define PREF_MONITOR_UNKNOWN_FTP_COMMANDS   (PREF_MONITOR_FTP+11)

#define PREF_MONITOR_CCS                (PREF_MONITOR_FTP+12)
    // all other CC's (which should be displayed with standard midi names if known)
#define PREF_MONITOR_EVERYTHING_ELSE    (PREF_MONITOR_CCS+1)
    // general catch all


//-----------------------
// performance filter
//-----------------------

#define PREF_PERF_FILTER                (PREF_MONITOR_EVERYTHING_ELSE + 1)
#define PREF_PERF_FILTER_BENDS          (PREF_PERF_FILTER + 1)
#define PREF_MONITOR_PERFORMANCE        (PREF_PERF_FILTER + 2)


//-----------------------
// total
//-----------------------

#define NUM_EEPROM_USED       (PREF_MONITOR_PERFORMANCE + 1)



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
extern bool pref_changed8(int pref);
extern bool pref_changed16(int pref);

extern void restore_prefs();
    // restore them to last saved state
extern void restore_pref8(int pref);
extern void restore_pref16(int pref);
extern void setDefaultPrefs();
    // should be called after multiple calls to restore_pref
    // is safe - does not re-read or alter write thru cache


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
