#include <myDebug.h>
#include "prefs.h"
#include <EEPROM.h>

#define dbg_prefs   0


uint8_t prefs[NUM_EEPROM_USED];
uint8_t pref_cache[NUM_EEPROM_USED];
uint8_t last_prefs[NUM_EEPROM_USED];
int16_t pref_min[NUM_EEPROM_USED];
int16_t pref_max[NUM_EEPROM_USED];
const char **pref_strings[NUM_EEPROM_USED];


const char *off_on[]                  = {"Off","On"};
const char *off_usb_serial[]          = {"Off","USB","Serial"};
const char *default_off_usb_serial[]  = {"default","Off","USB","Serial"};
const char *off_host_remote[]         = {"Off","Host","Remote"};
const char *off_on_detail[]           = {"Off","On","Detail"};
const char *curve_types[]             = {"linear","asympt","scurve"};


//------------------------------
// defaults
//-------------------------------


static void _setDefaultPref8(int i, int min, int max, uint8_t def_val, const char *strings[]=0)
{
    if (pref_cache[i] == 255)
        prefs[i] = def_val;
    pref_min[i] = min;
    pref_max[i] = max;
    pref_strings[i] = strings;
}

static void _setDefaultPref16(int i, int min, int max, uint16_t def_val)
{
    if (pref_cache[i]==255 && pref_cache[i+1]==255)
    {
        uint16_t *p = (uint16_t *)&prefs[i];
        *p = def_val;
        pref_min[i] = min;
        pref_max[i] = max;
    }
}


void _setDefaultPrefs()
{
    _setDefaultPref8(PREF_BRIGHTNESS,        1,100, 30);    // 1..100 - default(30)
    _setDefaultPref8(PREF_PATCH_NUM,         0,254, 1);     // 0..254 - default(1)

    _setDefaultPref8(PREF_DEBUG_PORT,        0,2,   1,  off_usb_serial);           // off, USB, Serial - default(USB)
    _setDefaultPref8(PREF_SPOOF_FTP,         0,1,   0,  off_on);                   // off, on - default(off)
    _setDefaultPref8(PREF_FTP_PORT,          0,2,   1,  off_host_remote);          // off, Host, Remote, default(Host)

    _setDefaultPref8(PREF_FTP_TOUCH_SENS,    0,9,   4);                            // 0..9 - default(4)
    _setDefaultPref8(PREF_FTP_DYN_RANGE,     0,10,  10);                           // 0..10 - default(10) - 0x0A..0x14 (10..20) default(2) weird - we map to 0..10, default(10)
    _setDefaultPref8(PREF_FTP_DYN_OFFSET,    0,20,  10);                          // 0..20 - default(10)

    _setDefaultPref8(PREF_PERF_FILTER,       0,1,   0,  off_on);     // off, on - default(off), filters all but notes and bends from channel 0
    _setDefaultPref8(PREF_PERF_FILTER_BENDS, 0,1,   0,  off_on);     // off, on - default(off), filters bends too.

    _setDefaultPref8(DEFAULT_PREF_MONITOR,            0,2,  1,  off_usb_serial);  // off, USB, Serial, 255=default    default(USB)
    _setDefaultPref8(DEFAULT_PREF_SHOW_FILTERED,      0,1,  0,  off_on);          // off, on                 default(off)
    _setDefaultPref8(DEFAULT_PREF_SYSEX,              0,2,  2,  off_on_detail);   // off, on, Detail         default(1==Detail)
    _setDefaultPref8(DEFAULT_PREF_ACTIVESENSE,        0,1,  0,  off_on);          // off, on                 default(0==off)
    _setDefaultPref8(DEFAULT_PREF_PERFORMANCE_CCS,    0,1,  1,  off_on);          // off, on,                default(1=on)
    _setDefaultPref8(DEFAULT_PREF_FTP_TUNING_MSGS,    0,1,  1,  off_on);          // off, on                 default(1==on)
    _setDefaultPref8(DEFAULT_PREF_FTP_NOTE_INFO,      0,1,  1,  off_on);          // off, on                 default(1==on)
    _setDefaultPref8(DEFAULT_PREF_FTP_VOLUME,         0,1,  1,  off_on);          // off, on                 default(1==on)
    _setDefaultPref8(DEFAULT_PREF_FTP_BATTERY,        0,1,  1,  off_on);          // off, on                 default(1==on)

    for (int p=0; p<NUM_MIDI_PORTS; p++)
    {
        _setDefaultPref8(PORT_PREF_MONITOR(p),        -1,2, 1,  default_off_usb_serial);  // 255=default, off, USB, Serial, 255=default
        _setDefaultPref8(PORT_PREF_SHOW_FILTERED(p),   0,1, 0,  off_on);          // off, on                 default(off)
        _setDefaultPref8(PORT_PREF_SYSEX(p),           0,2, 2,  off_on_detail);   // off, on, Detail         default(1==Detail)
        _setDefaultPref8(PORT_PREF_ACTIVESENSE(p),     0,1, 0,  off_on);          // off, on                 default(0==off)
        _setDefaultPref8(PORT_PREF_PERFORMANCE_CCS(p), 0,1, 1,  off_on);          // off, on,                default(1=on)
        _setDefaultPref8(PORT_PREF_FTP_TUNING_MSGS(p), 0,1, 1,  off_on);          // off, on                 default(1==on)
        _setDefaultPref8(PORT_PREF_FTP_NOTE_INFO(p),   0,1, 1,  off_on);          // off, on                 default(1==on)
        _setDefaultPref8(PORT_PREF_FTP_VOLUME(p),      0,1, 1,  off_on);          // off, on                 default(1==on)
        _setDefaultPref8(PORT_PREF_FTP_BATTERY(p),     0,1, 1,  off_on);          // off, on                 default(1==on)
    }

    for (int i=0; i<NUM_PEDALS; i++)
    {
        _setDefaultPref16(PREF_PEDAL(i) + PREF_PEDAL_CALIB_MIN_OFFSET,    0,1023,   0);             // default 0
        _setDefaultPref16(PREF_PEDAL(i) + PREF_PEDAL_CALIB_MAX_OFFSET,    0,1023,   1023);          // default 1023
        _setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_VALUE_MIN_OFFSET,    0,127,    0);             // default 0
        _setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_VALUE_MAX_OFFSET,    0,127,    i==1?92:127);   // default 127 (except for pedal 1, loop, which is 92)
        _setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_CURVE_TYPE_OFFSET,   0,2,      0,curve_types); // 0=linear, 1=asymptotic, 2=scurve - default(0) == num_points

        // zero out the points, they will be set if the curve type changes

        _setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_POINTS_OFFSET + PEDAL_POINTS_OFFSET_X,     0,127,  0);
        _setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_POINTS_OFFSET + PEDAL_POINTS_OFFSET_Y,     0,127,  0);
        _setDefaultPref16(PREF_PEDAL(i) + PREF_PEDAL_POINTS_OFFSET + PEDAL_POINTS_OFFSET_WEIGHT,0,1023, 0);
    }
}


// extern
int16_t getPrefMin(int pref)
{
    return pref_min[pref];
}
// extern
int16_t getPrefMax(int pref)
{
    return pref_max[pref];
}
// extern
void setPrefMax(int pref, int16_t max)
{
    pref_max[pref] = max;
}
// extern
const char **getPrefStrings(int pref)
{
    return pref_strings[pref];
}
// extern
void setPrefStrings(int pref, const char *strings[])
{
    pref_strings[pref] = strings;
}



//------------------------------
// init and save
//------------------------------

// extern
extern void init_global_prefs()
{
    for (int i=0; i<NUM_EEPROM_USED; i++)
    {
        prefs[i] = last_prefs[i] = pref_cache[i] = EEPROM.read(i);
    }
    _setDefaultPrefs();
}



// extern
void save_global_prefs()
{
    display(dbg_prefs,"save_global_prefs",0);
    for (int i=0; i<NUM_EEPROM_USED; i++)
    {
        EEPROM.write(i,pref_cache[i]);
        last_prefs[i] = pref_cache[i];
    }
}



//------------------------------
// changes
//------------------------------


// extern
bool pref_changed8(int pref)
{
    return last_prefs[pref] != pref_cache[pref];
}

// extern
bool pref_changed16(int pref)
{
    return last_prefs[pref] != pref_cache[pref] ||
           last_prefs[pref+1] != pref_cache[pref+1];
}


// extern
bool prefs_changed()
{
    for (int i=0; i<NUM_EEPROM_USED; i++)
        if (last_prefs[i] != pref_cache[i])
            return true;
    return false;
}


// extern
void restore_prefs()
{
    display(dbg_prefs,"restore_prefs()",0);
    for (int i=0; i<NUM_EEPROM_USED; i++)
        pref_cache[i] = last_prefs[i];
    _setDefaultPrefs();
}




//------------------------------
// setters
//------------------------------

// extern
void setPref8(int pref, uint8_t val)
{
    pref_cache[pref] = prefs[pref] = val;
}


// extern
void setPrefBool(int pref, bool val)
{
    pref_cache[pref] = prefs[pref] = val;
}


// extern
void setPref16(int pref, uint16_t val)
{
    uint16_t *p0 = (uint16_t *)&pref_cache[pref];
    uint16_t *p1 = (uint16_t *)&prefs[pref];
    *p0 = *p1 = val;
}


//extern
uint8_t portMonitorPref(int p, int off)
    // returns the preference setting for a port
    // defering to the default settings if the port
    // has it's main pref set to "default"
{
    uint8_t val = PORT_PREF_MONITOR(p);
    if (val == 255)
        return getPref8(DEFAULT_PREF_MONITOR + off);
    return getPref8(PORT_PREF_MONITOR(p) + off);
}
