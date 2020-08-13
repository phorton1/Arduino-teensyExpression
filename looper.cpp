#include <Arduino.h>
#include <EEPROM.h>
#include "looper.h"
#include "myDebug.h"
#include "prefs.h"


#define LOOPER_PREF_BASE    512


Looper theLooper;


Looper::Looper()
{
    m_relative_mode = false;
    for (int i=0; i<NUM_LOOP_TRACKS; i++)
        m_relative_volume[i] = 127;
}


int Looper::getPrefRelativeVol(int track_num)
{
    int val = EEPROM.read(LOOPER_PREF_BASE + track_num);
    if (val == 255)
        val = track_num == 0 ? 100 : 90;
    return val;
}

void Looper::init()        // init from prefs
{
    display(0,"sizeof(prefs)=%d  LOOPER_PREF_BASE=%d",NUM_EEPROM_USED,LOOPER_PREF_BASE);

    m_relative_volume[0] = getPrefRelativeVol(0);
    m_relative_volume[1] = getPrefRelativeVol(1);
    m_relative_volume[2] = getPrefRelativeVol(2);
    m_relative_volume[3] = getPrefRelativeVol(3);
}


void Looper::setRelativeVolume(int track_num, int vol, bool write_pref /* =false */)
{
    display(0,"relative_volume(%d)=%d  write_pref=%d",track_num,vol,write_pref);
    if (vol < 0) vol = 0;
    if (vol > 127) vol = 127;
    m_relative_volume[track_num] = vol;

    if (write_pref)
    {
        // there's a bug in the teensy 3.6 running at 180Mhz versus EEPROM.write().
        // at 180Mhz, this fucks with the button memory somehow.
        // at 120Mhz it does not seem to.
        // These delays seem to fix it, sheesh.

        delay(20);
        EEPROM.write(LOOPER_PREF_BASE + track_num,vol);
        delay(20);
    }
}
