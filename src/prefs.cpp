#include <myDebug.h>
#include "prefs.h"
#include <EEPROM.h>

#define dbg_prefs   0


uint8_t pref[NUM_EEPROM_USED];
uint8_t pref_save[NUM_EEPROM_USED];
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
const char *pedal_modes[]             = {"normal","serial"};
const char *file_system_ports[]       = {"Main USB", "Alt Serial"};


//------------------------------
// defaults
//-------------------------------


static void _setDefaultPref8(int i, int min, int max, uint8_t def_val, const char *strings[]=0)
{
    if (pref[i] == 255)
    {
        pref[i] = def_val;
        pref_save[i] = def_val;
    }
    pref_min[i] = min;
    pref_max[i] = max;
    pref_strings[i] = strings;
}

static void _setDefaultPref16(int i, int min, int max, uint16_t def_val)
{
    if (pref_save[i]==255 && pref_save[i+1]==255)
    {
        uint16_t *p0 = (uint16_t *) &pref[i];
        uint16_t *p1 = (uint16_t *) &pref_save[i];
        *p0 = def_val;
        *p1 = def_val;
    }
    pref_min[i] = min;
    pref_max[i] = max;
}


extern
void setDefaultPrefs()
{
    _setDefaultPref8(PREF_BRIGHTNESS,        1,100, 30);    // 1..100 - default(30)

    _setDefaultPref8(PREF_DEBUG_PORT,        0,2,   1,  off_usb_serial);            // off, USB, Serial - default(USB)
    _setDefaultPref8(PREF_FILE_SYSTEM_PORT,  0,1,   0,  file_system_ports);         // MainUSB or AlternateSerial port

    _setDefaultPref8(PREF_SPOOF_FTP,         0,1,   0,  off_on);                    // off, on - default(off)
    _setDefaultPref8(PREF_FTP_PORT,          0,2,   0,  off_host_remote);   // 2025-01-14 was: 2,  off_host_remote);           // off, Host, Remote, default(Remote)

    //---------------
    // pedals
    //---------------

    #define PEDAL_CALIB_DEFAULT_HIGH    770


    for (int i=0; i<NUM_PEDALS; i++)
    {
        int use_mode = (i == 1) ? PEDAL_MODE_SERIAL : PEDAL_MODE_NORMAL;
            // the loop pedal IS serial

        _setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_MODE_OFFSET,         0,1,      use_mode, pedal_modes);     // 0=normal, 1=serial
        _setDefaultPref8 (PREF_PEDAL(i) + PREF_PEDAL_CURVE_TYPE_OFFSET,   0,2,      0,curve_types);             // 0=linear, 1=asymptotic, 2=scurve - default(0) == num_points
        _setDefaultPref16(PREF_PEDAL(i) + PREF_PEDAL_CALIB_MIN_OFFSET,    0,1023,   0);                         // default 0
        _setDefaultPref16(PREF_PEDAL(i) + PREF_PEDAL_CALIB_MAX_OFFSET,    0,1023,   PEDAL_CALIB_DEFAULT_HIGH);

        int out_byte = PREF_PEDAL(i) + PREF_PEDAL_POINTS_OFFSET;

        for (int j=0; j<MAX_PEDAL_CURVES; j++)
        {
            int max = 127;

            _setDefaultPref8(out_byte++, 0, 127, 0);     // X    first point is always min, at 0,0
            _setDefaultPref8(out_byte++, 0, 127, 0);     // Y
            _setDefaultPref8(out_byte++, 0, 127, 0);     // W
            _setDefaultPref8(out_byte++, 0, 0,   0);     // W

            int x = 127/(j+1);

            // prh - 2020-08-13 - to use old rig you must manually set the loop pedal to 92 max
            //  int use_max = i==PEDAL_LOOP && j==0 ? 92 : 127;               // set loop max volume to 92 on linear

            int use_max = 127;
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

    _setDefaultPref8(PREF_MIDI_MONITOR,           0,3,  1,  off_debug_usb_serial); // off, follow, USB, Serial, default(follow debuug)

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
int16_t getPrefMin(int i)
{
    return pref_min[i];
}
// extern
int16_t getPrefMax(int i)
{
    return pref_max[i];
}
// extern
const char **getPrefStrings(int i)
{
    return pref_strings[i];
}



//------------------------------
// init and save
//------------------------------

// extern
void clear_prefs()
{
    EEPROM.write(0,TEENSY_EXPRESSION1_PREF_VERSION);
    for (int i=1; i<NUM_EEPROM_USED; i++)
        EEPROM.write(i,255);
}


// extern
bool init_global_prefs()
{
    bool retval = 0;
    if (EEPROM.read(0) != TEENSY_EXPRESSION1_PREF_VERSION)
    {
        retval = 1;
        clear_prefs();
    }
    for (int i=1; i<NUM_EEPROM_USED; i++)
    {
        pref[i] = pref_save[i] = EEPROM.read(i);
    }
    setDefaultPrefs();
    return retval;
}



// extern
void save_global_prefs()
{
    display(dbg_prefs,"save_global_prefs",0);
    for (int i=1; i<NUM_EEPROM_USED; i++)
    {
        EEPROM.write(i,pref[i]);
        pref_save[i] = pref[i];
    }
}



//------------------------------
// changes
//------------------------------

// extern
bool prefs_changed()
{
    for (int i=0; i<NUM_EEPROM_USED; i++)
        if (pref[i] != pref_save[i])
            return true;
    return false;
}


// extern
bool pref_changed8(int i)
{
    return pref[i] != pref_save[i];
}

// extern
bool pref_changed16(int i)
{
    return pref[i] != pref_save[i] ||
           pref[i+1] != pref_save[i+1];
}


// extern
void restore_prefs()
{
    display(dbg_prefs,"restore_prefs()",0);
    for (int i=0; i<NUM_EEPROM_USED; i++)
        pref[i] = pref_save[i];
}

// extern
void restore_pref8(int i)
{
    pref[i] = pref_save[i];
}

void restore_pref16(int i)
{
    pref[i] = pref_save[i];
    pref[i+1] = pref_save[i+1];
}




//------------------------------
// setters
//------------------------------

// extern
void setPref8(int i, uint8_t val)
{
    pref[i] = val;
}


// extern
void setPrefBool(int i, bool val)
{
    pref[i] = val;
}


// extern
void setPref16(int i, uint16_t val)
{
    uint16_t *p0 = (uint16_t *)&pref[i];
    *p0 = val;
}

