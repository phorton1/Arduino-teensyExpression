#ifndef _winFtpSensitivity_h_
#define _winFtpSensitivity_h_

#include "expSystem.h"

#define NUM_SENSITIVITY_ITEMS 10


class winFtpSensitivity : public expWindow
{
    public:

        winFtpSensitivity();

    private:

        virtual const char *name()          { return "FTP String Sensitivity"; }
        virtual const char *short_name()    { return "FTP Sens"; }

        virtual void begin(bool warm);
        virtual void updateUI();
        virtual void onButtonEvent(int row, int col, int event);

        // implementation

        bool draw_needed;
        int last_vel[6];
        int last_velocity[6];
        int last_value[NUM_SENSITIVITY_ITEMS];

        int selected_item;
        int last_selected_item;

        void init();
        void vel2ToInts(int *vel2, int *velocity);
        void drawBox(int string, int box32, int vel16);

        int ftp_dynamic_range;
        int ftp_dynamic_offset;
        int ftp_touch_sensitivity;
        int ftp_poly_mode;
};


#endif      // !_winFtpSensitivity_h_