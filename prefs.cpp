#include <myDebug.h>
#include "prefs.h"
#include <EEPROM.h>


uint8_t prefs[NUM_EEPROM_USED];
uint8_t pref_cache[NUM_EEPROM_USED];


static void setDefaultPref8(int i, uint8_t def_val)
{
    if (pref_cache[i] == 255)
        prefs[i] = def_val;
}

static void setDefaultPref16(int i, uint16_t def_val)
{
    if (pref_cache[i]==255 && pref_cache[i+1]==255)
    {
        uint16_t *p = (uint16_t *)&prefs[i];
        *p = def_val;
    }
}



// extern
extern void init_global_prefs()
{
    for (int i=0; i<NUM_EEPROM_USED; i++)
    {
        prefs[i] = pref_cache[i] = EEPROM.read(i);
    }

    setDefaultPref8(PREF_BRIGHTNESS,        30);    // 1..100 - default(30)
    setDefaultPref8(PREF_PATCH_NUM,         1);     // 0..254 - default(1)

    setDefaultPref8(PREF_DEBUG_PORT,        1);     // off, USB, Serial - default(USB)
    setDefaultPref8(PREF_SPOOF_FTP,         0);     // off, on - default(off)
    setDefaultPref8(PREF_FTP_PORT,          1);     // off, Host, Remote, default(Host)

    setDefaultPref8(PREF_FTP_TOUCH_SENS,    4);     // 0..9 - default(4)
    setDefaultPref8(PREF_FTP_DYN_RANGE,     10);    // 0x0A..0x14 (10..20) weird - default(20), we map to 0..10, default(10)
    setDefaultPref8(PREF_FTP_DYN_OFFSET,    10);    // 0..20 - default(10)

    setDefaultPref8(PREF_PERF_FILTER,       0);     // off, on - default(off), filters all but notes and bends from channel 0
    setDefaultPref8(PREF_PERF_FILTER_BENDS, 0);     // off, on - default(off), filters bends too.

    setDefaultPref8(DEFAULT_PREF_MONITOR,            1);    // off, USB, Serial, 255=default    default(USB)
    setDefaultPref8(DEFAULT_PREF_SHOW_FILTERED,      0);    // off, on                 default(off)
    setDefaultPref8(DEFAULT_PREF_SYSEX,              2);    // off, on, Detail         default(1==Detail)
    setDefaultPref8(DEFAULT_PREF_ACTIVESENSE,        0);    // off, on                 default(0==off)
    setDefaultPref8(DEFAULT_PREF_PERFORMANCE_CCS,    1);    // off, on,                default(1=on)
    setDefaultPref8(DEFAULT_PREF_FTP_TUNING_MSGS,    1);    // off, on                 default(1==on)
    setDefaultPref8(DEFAULT_PREF_FTP_NOTE_INFO,      1);    // off, on                 default(1==on)
    setDefaultPref8(DEFAULT_PREF_FTP_VOLUME,         1);    // off, on                 default(1==on)
    setDefaultPref8(DEFAULT_PREF_FTP_BATTERY,        1);    // off, on                 default(1==on)

    for (int p=0; p<NUM_MIDI_PORTS; p++)
    {
        setDefaultPref8(PORT_PREF_MONITOR(p),         255);  // off, USB, Serial, 255=default
        setDefaultPref8(PORT_PREF_SHOW_FILTERED(p),   0);    // off, on                 default(off)
        setDefaultPref8(PORT_PREF_SYSEX(p),           2);    // off, on, Detail         default(1==Detail)
        setDefaultPref8(PORT_PREF_ACTIVESENSE(p),     0);    // off, on                 default(0==off)
        setDefaultPref8(PORT_PREF_PERFORMANCE_CCS(p), 1);    // off, on,                default(1=on)
        setDefaultPref8(PORT_PREF_FTP_TUNING_MSGS(p), 1);    // off, on                 default(1==on)
        setDefaultPref8(PORT_PREF_FTP_NOTE_INFO(p),   1);    // off, on                 default(1==on)
        setDefaultPref8(PORT_PREF_FTP_VOLUME(p),      1);    // off, on                 default(1==on)
        setDefaultPref8(PORT_PREF_FTP_BATTERY(p),     1);    // off, on                 default(1==on)
    }

    for (int i=0; i<NUM_PEDALS; i++)
    {
        setDefaultPref16(PREF_PEDAL(i) + PREF_PEDAL_CALIB_MIN_OFFSET,    0);             // default 0
        setDefaultPref16(PREF_PEDAL(i) + PREF_PEDAL_CALIB_MAX_OFFSET,    1023);          // default 1023
        setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_VALUE_MIN_OFFSET,    0);             // default 0
        setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_VALUE_MAX_OFFSET,    i==1?92:127);   // default 127 (except for pedal 1, loop, which is 92)
        setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_CURVE_TYPE_OFFSET,   0);             // 0=linear, 1=asymptotic, 2=scurve - default(0) == num_points

        // zero out the points, they will be set if the curve type changes

        setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_POINTS_OFFSET + PEDAL_POINTS_OFFSET_X, 0);
        setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_POINTS_OFFSET + PEDAL_POINTS_OFFSET_Y, 0);
        setDefaultPref16(PREF_PEDAL(i) + PREF_PEDAL_POINTS_OFFSET + PEDAL_POINTS_OFFSET_WEIGHT, 0);
    }
}


// extern
void save_global_prefs()
{
    for (int i=0; i<NUM_EEPROM_USED; i++)
    {
        EEPROM.write(i,pref_cache[i]);
    }
}


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
