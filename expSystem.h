#ifndef __exp_system_h__
#define __exp_system_h__

#include "defines.h"
#include <Arduino.h>        // for intevalTimer

#define MAX_EXP_RIGS     10
#define MAX_MODAL_STACK     10


class expSystem;

#define WIN_FLAG_DELETE_ON_END      0x00010000
    // window will be deleted after call to endModal
#define WIN_FLAG_OWNER_TITLE        0x00001000
    // window calls theSystem.setTitle() itself
#define WIN_FLAG_SHOW_PEDALS        0x00002000
    // window calls theSystem.setTitle() itself

extern int_rect tft_rect;
extern int_rect title_rect;
extern int_rect full_client_rect;
extern int_rect pedal_rect;
extern int_rect client_rect;


class expWindow
    // base class for rigs, modal windows, and the configSystem
{
    public:

        expWindow()                 {m_flags = 0;}
        expWindow(uint32_t flags)   {m_flags = flags;}
        virtual ~expWindow()        {}

        virtual const char *name() = 0;
            // used for titles
        virtual const char *short_name() { return ""; };
            // only used for rigs in the config window
        virtual uint32_t getId()    { return 0; }


    protected:

        friend class expSystem;

        virtual void begin(bool warm);
            // warm means that we are coming down the modal window stack
            // as opposed to being invoked as a new window

            // derived classes should call base class method FIRST
            // base class clears all button registrations.
        virtual void end()  {}
            // called when the window is taken out of focus, they
            // don't generally need to worry about buttons and LEDs,
            // but may want to unregister midi event handlers, etc

        virtual bool onRotaryEvent(int num, int val)  { return false; }
        // virtual bool onPedalEvent(int num, int val)   { return false; }
        virtual void onButtonEvent(int row, int col, int event) {}
        virtual void onSerialMidiEvent(int cc_num, int value) {}


        virtual void updateUI() {}
        virtual void timer_handler()  {}

        virtual void onEndModal(expWindow *win, uint32_t param) {}
            // called by expSystem after modal windows close themselves
            // with calls to endModal();

        virtual void endModal(uint32_t param);
            // called by modal windows when they end themselves

        uint32_t m_flags;

};



#define MIDI_ACTIVITY_INLINE  1

class expSystem
{
    public:

        expSystem();
        ~expSystem()  {}

        void begin();
        void updateUI();

        void activateRig(int i);

        int getNumRigs()         { return m_num_rigs; }
        int getCurRigNum()       { return m_cur_rig_num; }
        int getPrevRigNum()      { return m_prev_rig_num; }
        expWindow *getCurRig()   { return m_rigs[m_cur_rig_num]; }

        // void pedalEvent(int num, int val);
        void rotaryEvent(int num, int val);
        void buttonEvent(int row, int col, int event);

        void setTitle(const char *title);

        void startModal(expWindow *win);
        void swapModal(expWindow *win, uint32_t param);
        void endModal(expWindow *win, uint32_t param);
        expWindow *getTopModalWindow();

        #if MIDI_ACTIVITY_INLINE
            inline void midiActivity(int port_num) { midi_activity[port_num]=millis(); }
        #else
            void midiActivity(int port_num);
        #endif

        int getTempo()      { return m_tempo; }

    private:

        int m_num_rigs;
        int m_cur_rig_num;
        int m_prev_rig_num;

        void addRig(expWindow *pRig);

        int m_num_modals;
        expWindow *m_modal_stack[MAX_MODAL_STACK];

        expWindow *m_rigs[MAX_EXP_RIGS + 1];
            // 1 extra for rig #0 which is overloaded
            // as the configSystem window.

        IntervalTimer m_timer;
        IntervalTimer m_critical_timer;

        static void timer_handler();
        static void critical_timer_handler();

        int last_battery_level;

        const char *m_title;
        bool draw_pedals;
        bool draw_title;
            // state for redraw

        unsigned midi_activity[NUM_PORTS];
        bool last_midi_activity[NUM_PORTS];

        int m_tempo;

        void handleSerialData();

};


extern expSystem theSystem;
    // in teensyExpression.ino


#endif // !__exp_system_h__