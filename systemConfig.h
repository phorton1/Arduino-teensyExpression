#ifndef _systemConfig_h_
#define _systemConfig_h_

#include "expSystem.h"


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
        virtual void onButtonEvent(int row, int col, int event);

};


#endif      // !_systemConfig_h_