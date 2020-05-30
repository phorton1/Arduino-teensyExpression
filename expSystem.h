#ifndef __exp_system_h__
#define __exp_system_h__

#include "rawButtonArray.h"

#define USE_SERIAL_TO_RPI   0


class expSystem;
class expConfig;
    // forwards

class expSection
{
    public:
        
        expSection(expSystem *pSystem, expConfig *pConfig);

        virtual bool buttonEventHandler(int row, int col, int event)
            // returns true if the section handled the event
            { return false; }
        
    protected:
        friend class expConfig;
        
        expSection *m_pNextSection;
        expSection *m_pPrevSection;
        
        expSystem *m_pSystem;
        expConfig *m_pConfig;
            // pointers to the parent system and configuration

};



class expConfig
    // base class for configurations
{
    public:
        
        expConfig(expSystem *pSystem);
        ~expConfig() {}
        
        void addSection(expSection *pSection);
        
        virtual void begin();
            // derived classes should call base class method FIRST
            // base class clears all button registrations.
            
        virtual void buttonEventHandler(int row, int col, int event);
            // dispatches the event to the appropriate section
        
    protected:
        friend class expSystem;
        
        expSystem *m_pSystem;
            // pointer to the parent system
            
        expSection *m_pFirstSection;
        expSection *m_pLastSection;
            // a linked list of sections

};


#define MAX_EXP_CONFIGS   10


class expSystem
{
    public:
        
        expSystem();
        ~expSystem()  {}
        
        void begin();
        void task();
        
        void activateConfig(int i);

        int getNumConfigs()     { return m_num_configs; }
        int getCurConfigNum()   { return m_cur_config_num; }
        int getPrevConfigNum()  { return m_prev_config_num; }
        expConfig *getCurConfig() { return m_pConfigs[m_cur_config_num]; }
        void addConfig(expConfig *pConfig);
        
        rawButtonArray *getRawButtonArray()  { return m_pRawButtonArray; }
            
        
        
        
    private:

        int m_num_configs;
        int m_cur_config_num;
        int m_prev_config_num;
        expConfig *m_pConfigs[MAX_EXP_CONFIGS];
            
            
        rawButtonArray *m_pRawButtonArray;
        static void staticButtonEventHandler(void *obj, int row, int col, int event);
        void buttonEventHandler(int row, int col, int event);
            // the button array and its event handlers.
            // button events are then dispatched to the current configuration.
        
        IntervalTimer m_timer;
        static void timer_handler();
            // the timer and handler that calls rawButtonArray::task()
            // and expConvertors::task(), which then calls back to the
            // registered button event handler.

};


#endif // !__exp_system_h__