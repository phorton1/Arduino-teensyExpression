#ifndef __looper_h__
#define __looper_h__

// similar to ftp.h/cpp, this "module" abstracts some global behavior
// releated to the looper, and is, at this time Quantiloop specific,
// and/or will be the place to put my (in development) rPi looper specfic
// behaviors

#define NUM_LOOP_TRACKS    4

class Looper
{
    public:

        Looper();

        void init();        // (re)-init from prefs

        bool getRelativeVolumeMode()
            { return m_relative_mode; }
        void setRelativeVolumeMode(bool relative_mode)
            { m_relative_mode = true; };

        int getRelativeVolume(int track_num)
            { return m_relative_volume[track_num]; }

        void setRelativeVolume(int track_num, int vol, bool write_pref=false);

        int getPrefRelativeVol(int track_num);

    private:

        bool m_relative_mode;
        int m_relative_volume[NUM_LOOP_TRACKS];
};


extern Looper theLooper;



#endif  // !__looper_h__
