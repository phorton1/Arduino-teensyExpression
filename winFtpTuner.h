#ifndef _winFtpTuner_h_
#define _winFtpTuner_h_

#include "expSystem.h"



class winFtpTuner : public expWindow
{
    public:
        
        winFtpTuner();

    private:
        
        virtual const char *name()          { return "FTP Tuner"; }
        virtual const char *short_name()    { return "FTP Tuner"; }
        virtual void begin();
        virtual void updateUI();
        
        virtual void onButtonEvent(int row, int col, int event);
            // unused

        // implementation
        
        bool draw_needed;
        int last_battery_level;
        int last_string_pressed[6];
        int last_tuner_note;
        int last_tuner_value;        
        elapsedMillis battery_time;
        
        void init();
        void drawCircle(int string, int fret, bool pressed);
        void fretsToInts(int *ints);
        void drawTunerPointer(int tuner_x, int color);
};


#endif      // !_winFtpTuner_h_