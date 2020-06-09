#ifndef _systemConfig_h_
#define _systemConfig_h_

#include "expSystem.h"


class systemConfig : public expConfig
{
    public:
        
        systemConfig(expSystem *pSystem);
        virtual const char *name() { return "SYSTEM CONFIGURATION"; }
        
        virtual void begin();
        virtual void updateUI();
        virtual void onButtonEvent(int row, int col, int event);
        virtual void onRotaryEvent(int num, int val);        
        
    private:

        bool changed();
        
        int m_orig_brightness;
        int m_orig_config_num;
        
        int m_brightness;
        int m_config_num;

        void enableButtons();
        bool m_cancel_enabled;
        
        int m_last_changed;
        int m_last_brightness;
        int m_last_config_num;
        
};



#endif      // !_systemConfig_h_