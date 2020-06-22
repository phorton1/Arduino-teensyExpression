#ifndef _configSystem_h_
#define _configSystem_h_

#include "expSystem.h"


class configSystem : public expWindow
{
    public:
        
        configSystem();

        void notifyTerminalModeEnd();
            // called by options with implemented terminal modes
            // at the end of their operation to return to the
            // configSystem editor

    private:
        
        virtual const char *name()          { return "SYSTEM CONFIGURATION"; }
        virtual const char *short_name()    { return "Sys Config"; }
        
        virtual void begin();
        virtual void updateUI();
        virtual void onButtonEvent(int row, int col, int event);

};


#endif      // !_configSystem_h_