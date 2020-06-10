#ifndef __exp_system_h__
#define __exp_system_h__

#include "defines.h"
#include "rawButtonArray.h"

#if WITH_CHEAP_TFT
    #include <LCDWIKI_GUI.h>    // my modified Core graphics library
    #include <LCDWIKI_KBV.h>    // my modified Hardware-specific library
    #include <font_Arial.h>
    #include <font_ArialBold.h>
    
    extern LCDWIKI_KBV mylcd;
#endif



class expSystem;
    // forwards


class expConfig
    // base class for configurations
{
    public:
        
        expConfig(expSystem *pSystem);
        ~expConfig() {}
        
        virtual const char *name() = 0;
        virtual const char *short_name() = 0;
        
        
        virtual void begin();
            // derived classes should call base class method FIRST
            // base class clears all button registrations.
            
        virtual void onButtonEvent(int row, int col, int event);
        virtual void onRotaryEvent(int num, int val);
        virtual void onPedalEvent(int num, int val);
        virtual void updateUI() {}
        virtual void timer_handler()  {}
        
    protected:
        friend class expSystem;
        
        expSystem *m_pSystem;
            // pointer to the parent system
};




#define MAX_EXP_CONFIGS   10


class expSystem
{
    public:
        
        expSystem();
        ~expSystem()  {}
        
        void begin();
        void updateUI();
        
        void activateConfig(int i);

        int getNumConfigs()     { return m_num_configs; }
        int getCurConfigNum()   { return m_cur_config_num; }
        int getPrevConfigNum()  { return m_prev_config_num; }
        expConfig *getCurConfig()   { return m_pConfigs[m_cur_config_num]; }
        expConfig *getConfig(int i) { return m_pConfigs[i]; }
        
        void addConfig(expConfig *pConfig);
        
        rawButtonArray *getRawButtonArray()  { return m_pRawButtonArray; }
            
    private:

        int m_num_configs;
        int m_cur_config_num;
        int m_prev_config_num;
        expConfig *m_pConfigs[MAX_EXP_CONFIGS];
            
            
        rawButtonArray *m_pRawButtonArray;
        static void staticOnButtonEvent(void *obj, int row, int col, int event);
        void onButtonEvent(int row, int col, int event);
            // the button array and its event handlers.
            // button events are then dispatched to the current configuration.
        
        IntervalTimer m_timer;
        static void timer_handler();
            // the timer and handler that calls rawButtonArray::task()
            // and expConvertors::task(), which then calls back to the
            // registered button event handler.

};

extern expSystem *s_pTheSystem;



#endif // !__exp_system_h__