#ifndef _dlgFtpTuner_h_
#define _dlgFtpTuner_h_

#include "expSystem.h"



class dlgFtpTuner : public expConfig
{
    public:
        
        dlgFtpTuner();

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
        
        void init();
        void drawCircle(int string, int fret, bool pressed);
        void fretsToInts(int *ints);
        void drawTunerPointer(int tuner_x, int color);
};


#endif      // !_dlgFtpTuner_h_