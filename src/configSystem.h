//--------------------------------
// configSystem.h
//--------------------------------

#pragma once

#include "expSystem.h"

#define BUTTON_EXIT_CANCEL      3
#define BUTTON_EXIT_DONE        4


class configOption;
class winConfigPedal;
    // forwards

class configSystem : public expWindow
{
    public:

        configSystem();

        void notifyTerminalModeEnd();
            // called by options with implemented terminal modes
            // at the end of their operation to return to the
            // configSystem editor

    private:

        friend class winConfigPedal;

        virtual const char *name()          { return "SYSTEM SETTINGS"; }
        virtual const char *short_name()    { return "Sys Settings"; }

        virtual void begin(bool warm);
        virtual void updateUI();
        virtual void onButtonEvent(int row, int col, int event);
        virtual void onEndModal(expWindow *win, uint32_t param);

        void checkDirty();
        void clearOptionStates(configOption *option);
        
        
        bool m_dirty;
        int m_scroll_top;
        configOption *m_last_display_option;
        int m_last_selected_rig;

};


extern configSystem config_system;

