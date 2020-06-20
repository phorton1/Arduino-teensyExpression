#ifndef _midiHostConfig_h_
#define _midiHostConfig_h_

#include "expSystem.h"


class midiHostConfig : public expConfig
{
    public:
        
        midiHostConfig();

    private:
        
        virtual const char *name()          { return "FTP Test Config"; }
        virtual const char *short_name()    { return "FTP Test"; }

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
        
        uint8_t dbg_bank_num;
        uint8_t dbg_patch_num;
        uint8_t dbg_command;	
        uint8_t dbg_param;
            
        int sens_button_repeat;
        unsigned sens_button_repeat_time;
        
        int my_led_color[25];
        void mySetLED(int i, int color);
        void myRestoreLED(int i);
        void myIncDec(int inc, uint8_t *val);

        
};


#endif      // !_midiHostConfig_h_