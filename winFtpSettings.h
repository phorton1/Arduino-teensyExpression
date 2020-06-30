#ifndef _winFtpSettings_h_
#define _winFtpSettings_h_

#include "expSystem.h"

#define PERF_LAYER_TYPE_NONE  0
#define PERF_LAYER_TYPE_1_5   1
#define PERF_LAYER_TYPE_2_4   2
#define PERF_LAYER_TYPE_3_3   3
#define PERF_LAYER_TYPE_4_2   4
#define PERF_LAYER_TYPE_5_1   5
#define PERF_NUM_LAYER_TYPES  6

#define FTP_SETTING_POLY_MODE             0
#define FTP_SETTING_BEND_MODE             1
#define FTP_SETTING_PERF_LAYER_TYPE       2
#define FTP_NUM_SETTINGS                  3

class winFtpSettings : public expWindow
{
    public:

        winFtpSettings();

        static int getSetting(int i)   { return ftp_settings[i];}


    private:


        virtual const char *name()          { return "FTP Settings"; }
        virtual const char *short_name()    { return "FTP Settings"; }

        virtual void updateUI();
        virtual void begin(bool warm);
        virtual void onButtonEvent(int row, int col, int event);

        // implementation

        bool draw_needed;
        int selected_item;
        int last_selected_item;

        static int ftp_settings[FTP_NUM_SETTINGS];
        int last_value[FTP_NUM_SETTINGS];

};


#endif      // !_winFtpSettings_h_