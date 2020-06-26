#ifndef __prefs_h__
#define __prefs_h__

#include "defines.h"

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

#endif
