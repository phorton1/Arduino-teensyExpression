#ifndef _dlgFtpSens_h_
#define _dlgFtpSens_h_

#include "expSystem.h"

#define NUM_ITEMS 8


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
        int last_vel[6];
        int last_velocity[6];
        int last_value[NUM_ITEMS];

        int selected_item;
        int last_selected_item;
        
        void init();
        void vel2ToInts(int *vel2, int *velocity);
        void drawBox(int string, int box32, int vel16);
        
        int ftp_dynamic_range;
        int ftp_dynamic_offset;
        
};


#endif      // !_dlgFtpSens_h_