#ifndef _systemConfig_h_
#define _systemConfig_h_

#include "expSystem.h"

#define BUTTON_MOVE_UP          12
#define BUTTON_MOVE_LEFT        16
#define BUTTON_MOVE_RIGHT       18
#define BUTTON_MOVE_DOWN        22
#define BUTTON_SELECT           17


class systemConfig : public expConfig
{
    public:
        
        systemConfig();

        void notifyTerminalModeEnd();
            // called by options with implemented terminal modes
            // at the end of their operation to return to the
            // configuration editor;

    private:
        
        virtual const char *name()          { return "SYSTEM CONFIGURATION"; }
        virtual const char *short_name()    { return "Sys Config"; }
        
        virtual void begin();
        virtual void updateUI();
        virtual void timer_handler();
        virtual void onButtonEvent(int row, int col, int event);
        virtual void onRotaryEvent(int num, int val);        

        void draw();
        void enableCancel();
        void onNavPad(int num);
        
};


#endif      // !_systemConfig_h_