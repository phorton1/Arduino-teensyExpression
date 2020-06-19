#ifndef _dlgFtpSens_h_
#define _dlgFtpSens_h_

#include "expSystem.h"



class dlgFtpSens : public expConfig
{
    public:
        
        dlgFtpSens();

    private:
        
        virtual const char *name()          { return "FTP String Sensitivity"; }
        virtual const char *short_name()    { return "FTP Sens"; }
        virtual void begin();
        virtual void updateUI();
        
        virtual void onButtonEvent(int row, int col, int event);

        // implementation
        
        bool draw_needed;
        int last_battery_level;
        int last_string_val[6];
        int last_string_sens[6];
        
        void init();
        void drawCircle(int string, int fret, bool pressed);
        void vel2ToInts(int *ints);
        void drawTunerPointer(int tuner_x, int color);
};


#endif      // !_dlgFtpSens_h_