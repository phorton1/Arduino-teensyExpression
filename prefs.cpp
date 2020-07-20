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
const char *off_debug_usb_serial[]    = {"Off","Debug Port","USB","Serial"};
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


extern
void setDefaultPrefs()
{
    _setDefaultPref8(PREF_BRIGHTNESS,        1,100, 30);    // 1..100 - default(30)

    // use the existing max and strings for PREF_PATCH_NUM
    // which gets setup in expSystem::begin()

    _setDefaultPref8(PREF_PATCH_NUM,         0,pref_max[PREF_PATCH_NUM], 1, pref_strings[PREF_PATCH_NUM]);
        // 0..0 - default(1) .. better be some patches!
    _setDefaultPref8(PREF_DEBUG_PORT,        0,2,   1,  off_usb_serial);            // off, USB, Serial - default(USB)
    _setDefaultPref8(PREF_SPOOF_FTP,         0,1,   0,  off_on);                    // off, on - default(off)
    _setDefaultPref8(PREF_FTP_PORT,          0,2,   2,  off_host_remote);           // off, Host, Remote, default(Remote)

    //---------------
    // pedals
    //---------------
    // the prefs store the regular pedal calibrations
    // the auto pedal calibrations are always 0,127

    #define PEDAL_CALIB_WITH_1K_OUTPUT_RESISTOR    770
        // every time it seems to get lower.
        // this is about the value I get with the rig plugged into the old grey box ...
        // at a voltage of 4.60-4.98V ... not sure why adding 1K dropped 1/5th of the
        // range ... might have been a bit much.

    for (int i=0; i<NUM_PEDALS; i++)
    {
        bool is_auto = i==PEDAL_SYNTH ? 1 : 0;
        _setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_AUTO_OFFSET,         0,1,      is_auto,off_on);    // 0=linear, 1=asymptotic, 2=scurve - default(0) == num_points
        _setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_CURVE_TYPE_OFFSET,   0,2,      0,curve_types);     // 0=linear, 1=asymptotic, 2=scurve - default(0) == num_points
        _setDefaultPref16(PREF_PEDAL(i) + PREF_PEDAL_CALIB_MIN_OFFSET,    0,1023,   0);                 // default 0
        _setDefaultPref16(PREF_PEDAL(i) + PREF_PEDAL_CALIB_MAX_OFFSET,    0,1023,   PEDAL_CALIB_WITH_1K_OUTPUT_RESISTOR);    // default 890 (1023)

        int out_byte = PREF_PEDAL(i) + PREF_PEDAL_POINTS_OFFSET;

        for (int j=0; j<MAX_PEDAL_CURVES; j++)
        {
            int max = 127;

            _setDefaultPref8(out_byte++, 0, 127, 0);     // X    first point is always min, at 0,0
            _setDefaultPref8(out_byte++, 0, 127, 0);     // Y
            _setDefaultPref8(out_byte++, 0, 127, 0);     // W
            _setDefaultPref8(out_byte++, 0, 0,   0);     // W

            int x = 127/(j+1);
            int use_max = i==1 && j==0 ? 92 : 127;               // set loop max volume to 92 on linear
            int y = use_max/(j+1);

            _setDefaultPref8(out_byte++, 0, 127, x);     // X    second point is max, mid, or left with slope 1
            _setDefaultPref8(out_byte++, 0, 127, y);     // Y
            _setDefaultPref8(out_byte++, 0, 127, 0);     // W
            _setDefaultPref8(out_byte++, 0, 0,   0);     // W

            x = 0;
            max = 0;
            if (j)
            {
                max = 127;
                x = (2 * 127)/(j+1);
            }

            _setDefaultPref8(out_byte++, 0, max, x);     // X    third point is null, max, or right
            _setDefaultPref8(out_byte++, 0, max, x);     // Y
            _setDefaultPref8(out_byte++, 0, max, 0);     // W
            _setDefaultPref8(out_byte++, 0, 0,   0);     // W

            x = 0;
            max = 0;
            if (j==2)
            {
                max = 127;
                x = 127;
            }

            _setDefaultPref8(out_byte++, 0, max, x);     // X    fourth point is null or max
            _setDefaultPref8(out_byte++, 0, max, x);     // Y
            _setDefaultPref8(out_byte++, 0, max, 0);     // W
            _setDefaultPref8(out_byte++, 0, 0,   0);     // W

        }
    }

    //----------------------
    // midi monitor prefs
    //----------------------

    _setDefaultPref8(PREF_MIDI_MONITOR,            0,3,  1,  off_debug_usb_serial); // off, USB, Serial, 255=default    default(USB)

    _setDefaultPref8(PREF_MONITOR_DUINO_INPUT0,   0,1,  0, off_on);    // default(off)
    _setDefaultPref8(PREF_MONITOR_DUINO_INPUT1,   0,1,  1, off_on);    // default(on)
    _setDefaultPref8(PREF_MONITOR_DUINO_OUTPUT0,  0,1,  1, off_on);    // default(on)
    _setDefaultPref8(PREF_MONITOR_DUINO_OUTPUT1,  0,1,  1, off_on);    // default(on)
    _setDefaultPref8(PREF_MONITOR_HOST_INPUT0,    0,1,  0, off_on);    // default(off)
    _setDefaultPref8(PREF_MONITOR_HOST_INPUT1,    0,1,  1, off_on);    // default(on)
    _setDefaultPref8(PREF_MONITOR_HOST_OUTPUT0,   0,1,  0, off_on);    // default(off)
    _setDefaultPref8(PREF_MONITOR_HOST_OUTPUT1,   0,1,  1, off_on);    // default(on)

    for (int i=0; i<16; i++)
        _setDefaultPref8(PREF_MONITOR_CHANNEL1+i, 0,1,  1,off_on);

    _setDefaultPref8(PREF_MONITOR_SYSEX,           0,2,  2,  off_on_detail);        // off, on, Detail         default(1==Detail)
    _setDefaultPref8(PREF_MONITOR_ACTIVESENSE,     0,1,  0,  off_on);               // off, on                 default(0==off)

    for (int i=PREF_MONITOR_NOTE_ON; i<=PREF_MONITOR_EVERYTHING_ELSE; i++)
        _setDefaultPref8(i,0,1,1,off_on);

    //----------------------
    // performance filter
    //----------------------

    _setDefaultPref8(PREF_PERF_FILTER,          0,1,  1, off_on);    // default(on)
    _setDefaultPref8(PREF_PERF_FILTER_BENDS,    0,1,  0, off_on);    // default(off)
    _setDefaultPref8(PREF_MONITOR_PERFORMANCE,  0,1,  1, off_on);    // default(on)
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
    setDefaultPrefs();
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
bool prefs_changed()
{
    for (int i=0; i<NUM_EEPROM_USED; i++)
        if (last_prefs[i] != pref_cache[i])
            return true;
    return false;
}


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
void restore_prefs()
{
    display(dbg_prefs,"restore_prefs()",0);
    for (int i=0; i<NUM_EEPROM_USED; i++)
        pref_cache[i] = last_prefs[i];
    setDefaultPrefs();
}

// extern
void restore_pref8(int i)
{
    pref_cache[i] = last_prefs[i];
}

void restore_pref16(int i)
{
    pref_cache[i] = last_prefs[i];
    pref_cache[i+1] = last_prefs[i+1];
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
