#include "looper.h"
#include "myDebug.h"
#include "prefs.h"

Looper theLooper;


Looper::Looper()
{
    m_relative_mode = false;
    for (int i=0; i<NUM_LOOP_TRACKS; i++)
        m_relative_volume[i] = 127;
}

void Looper::init()        // init from prefs
{
    m_relative_volume[0] = 100;
    m_relative_volume[1] = 90;
    m_relative_volume[2] = 90;
    m_relative_volume[3] = 90;
}


void Looper::writePrefs()  // write all four to prefs
{
}


void Looper::setRelativeVolume(int track_num, int vol, bool write_pref /* =false */)
{
    m_relative_volume[track_num] = vol;
}
