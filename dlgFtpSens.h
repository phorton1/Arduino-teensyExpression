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

        virtual void end();
        virtual void begin();
        virtual void updateUI();
        virtual void timer_handler();
        virtual void onButtonEvent(int row, int col, int event);
        void navPad(int num);

        // implementation
        
        bool draw_needed;
        int last_vel[6];
        int last_velocity[6];
        int last_sens[6];
        
        void init();
        void vel2ToInts(int *vel2, int *velocity);
        void drawBox(int string, int box32, int vel16);
};


#endif      // !_dlgFtpSens_h_