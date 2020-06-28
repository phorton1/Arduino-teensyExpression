#ifndef _configSystem_h_
#define _configSystem_h_

#include "expSystem.h"

class configOption;

class configSystem : public expWindow
{
    public:

        configSystem();

        void notifyTerminalModeEnd();
            // called by options with implemented terminal modes
            // at the end of their operation to return to the
            // configSystem editor

    private:

        virtual const char *name()          { return "SYSTEM SETTINGS"; }
        virtual const char *short_name()    { return "Sys Settings"; }

        virtual void begin(bool warm);
        virtual void updateUI();
        virtual void onButtonEvent(int row, int col, int event);
        virtual void onEndModal(expWindow *win, uint32_t param);

        int m_scroll_top;
        configOption *m_last_display_option;

};


#endif      // !_configSystem_h_