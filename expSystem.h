#ifndef __exp_system_h__
#define __exp_system_h__

#include "defines.h"
#include <Arduino.h>        // for intevalTimer

#define MAX_EXP_PATCHES     5

class expSystem;


class expWindow
    // base class for patches, modal windows, and the configSystem
{
    public:
        
        expWindow();
        ~expWindow() {}
        
        virtual const char *name() = 0;
        virtual const char *short_name() = 0;
        
    protected:

        friend class expSystem;
        
        virtual void begin();
            // derived classes should call base class method FIRST
            // base class clears all button registrations.
        virtual void end()  {}
            // called when the window is taken out of focus, they
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
        
        void activatePatch(int i);

        int getNumPatches()         { return m_num_patches; }
        int getCurPatchNum()        { return m_cur_patch_num; }
        int getPrevConfigNum()      { return m_prev_patch_num; }
        expWindow *getCurPatch()    { return m_patches[m_cur_patch_num]; }
        expWindow *getPatch(int i) { return m_patches[i]; }
        
        void pedalEvent(int num, int val);
        void rotaryEvent(int num, int val);
        void buttonEvent(int row, int col, int event);
        
        void setTitle(const char *title);
        
        
    private:

        int m_num_patches;
        int m_cur_patch_num;
        int m_prev_patch_num;


        void addPatch(expWindow *pConfig);
        
        expWindow *m_patches[MAX_EXP_PATCHES + 1];
            // 1 extra for patch #0 which is overloaded
            // as the configSystem window.
            
        IntervalTimer m_timer;
        IntervalTimer m_critical_timer;
        
        static void timer_handler();
        static void critical_timer_handler();
        
        
        int last_battery_level;
        elapsedMillis battery_time;
        bool draw_needed;
        
        const char *m_title;
        
        
};


extern expSystem theSystem;
    // in teensyExpression.ino



#endif // !__exp_system_h__