#ifndef __exp_system_h__
#define __exp_system_h__

#include "defines.h"
#include <Arduino.h>        // for intevalTimer

#define MAX_EXP_CONFIGS     10

class expSystem;


class expConfig
    // base class for configurations
{
    public:
        
        expConfig();
        ~expConfig() {}
        
        virtual const char *name() = 0;
        virtual const char *short_name() = 0;
        
    protected:

        friend class expSystem;
        
        virtual void begin();
            // derived classes should call base class method FIRST
            // base class clears all button registrations.
        virtual void end()  {}
            // called when the config is taken out of focus, they
            // don't generally need to worry about buttons and LEDs,
            // but may want to unregister midi event handlers, etc
            
        virtual bool onRotaryEvent(int num, int val)  { return false; }
        virtual bool onPedalEvent(int num, int val)   { return false; }
            // derived classes return true if they handled the event
            // otherwise default base class behavior takes place
        virtual void onButtonEvent(int row, int col, int event) {}
        
        virtual void updateUI() {}
        virtual void timer_handler()  {}
       
};



class expSystem
{
    public:
        
        expSystem();
        ~expSystem()  {}
        
        void begin();
        void updateUI();
        
        void activateConfig(int i);

        int getNumConfigs()         { return m_num_configs; }
        int getCurConfigNum()       { return m_cur_config_num; }
        int getPrevConfigNum()      { return m_prev_config_num; }
        expConfig *getCurConfig()   { return m_pConfigs[m_cur_config_num]; }
        expConfig *getConfig(int i) { return m_pConfigs[i]; }
        
        void addConfig(expConfig *pConfig);
        
        void pedalEvent(int num, int val);
        void rotaryEvent(int num, int val);
        void buttonEvent(int row, int col, int event);
        
    private:

        int m_num_configs;
        int m_cur_config_num;
        int m_prev_config_num;
        expConfig *m_pConfigs[MAX_EXP_CONFIGS];
            
        IntervalTimer m_timer;
        IntervalTimer m_critical_timer;
        
        static void timer_handler();
        static void critical_timer_handler();
        
};


extern expSystem theSystem;
    // in teensyExpression.ino



#endif // !__exp_system_h__