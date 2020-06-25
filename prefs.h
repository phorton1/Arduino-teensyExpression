#ifndef __prefs_h__
#define __prefs_h__

#include "defines.h"


extern uint8_t prefs[NUM_EEPROM_USED];

extern void init_global_prefs();
extern void save_global_prefs();

inline uint8_t  getPref8(int pref)      { return prefs[pref]; }
inline bool     getPrefBool(int pref)   { return (bool) prefs[pref]; }
inline uint16_t getPref16(int pref)     { uint16_t *p=(uint16_t *) &prefs[pref];  return *p; }

extern void setPref8(int pref, uint8_t val);
extern void setPrefBool(int pref, bool val);
extern void setPref16(int pref, uint16_t val);


inline uint8_t portMonitorPref(int p, int off)
    // returns the preference setting for a port
    // defering to the default settings if the port
    // has it's main pref set to "default"
{
    uint8_t val = PORT_PREF_MONITOR(p);
    if (val == 255)
        return getPref8(DEFAULT_PREF_MONITOR + off);
    return getPref8(PORT_PREF_MONITOR(p) + off);
}


#endif
