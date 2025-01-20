//--------------------------------
// expSystem.h
//--------------------------------

#pragma once

#include "defines.h"
#include <Arduino.h>        // for intevalTimer

#define MIDI_ACTIVITY_INLINE        1

#define MAX_MODAL_STACK             10

#define WIN_FLAG_DELETE_ON_END      0x00010000
    // window will be deleted after call to endModal
#define WIN_FLAG_OWNER_TITLE        0x00001000
    // window calls theSystem.setTitle() itself
#define WIN_FLAG_SHOW_PEDALS        0x00002000
    // window calls theSystem.setTitle() itself


// globally defined screen regions

extern int_rect tft_rect;                   // the full screen
extern int_rect title_rect;                 // title area not incuding line
extern int_rect full_client_rect;           // from under line to end of screen - available to windows
extern int_rect pedal_rect;                 // the area containing the pedals - shown in rigs
extern int_rect client_rect;                // area available to rigs - under title to top of pedals

// screen regions for "standard" rigs (derived from rigBase)
// that also work with the songMachine ...

extern int_rect synth_rect;                 // top part of the client area
extern int_rect song_title_rect;            // shows the current song title
extern int_rect song_state_rect;            // shows the current songmachine state
extern int_rect song_msg_rect[2];           // the two regions for user defined DISPLAY messages


class expSystem;
    // forward

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
            // only used for enumerated rigs in the config window
        virtual uint32_t getId()    { return 0; }

        virtual void onSerialMidiEvent(int cc_num, int value) {}
            // made public for common handleSerial() method

    protected:

        friend class expSystem;

        virtual void begin(bool warm)  {}
            // warm means that we are coming down the modal window stack
            // as opposed to being invoked as a new window.
        virtual void end()  {}
            // called when the window is taken out of focus, they
            // don't generally need to worry about buttons and LEDs,
            // but may want to unregister midi event handlers, etc

        virtual bool onRotaryEvent(int num, int val)  { return false; }
        // virtual bool onPedalEvent(int num, int val)   { return false; }
        virtual void onButtonEvent(int row, int col, int event) {}


        virtual void updateUI() {}
        virtual void timer_handler()  {}

        virtual void onEndModal(expWindow *win, uint32_t param) {}
            // called by expSystem after modal windows close themselves
            // with calls to endModal();

        virtual void endModal(uint32_t param);
            // called by modal windows when they end themselves

        uint32_t m_flags;
};



class expSystem
{
    public:

        expSystem();
        ~expSystem()  {}

        void begin();
        void updateUI();

        void activateRig(expWindow *the_rig);

        expWindow   *m_cur_rig;

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

    private:
        
        void startWindow(expWindow *win, bool warm);
        static void timer_handler();
        static void critical_timer_handler();

        IntervalTimer m_timer;
        IntervalTimer m_critical_timer;

        volatile int m_num_modals;
        expWindow *m_modal_stack[MAX_MODAL_STACK];

        bool draw_title;
        bool draw_pedals;
        const char *m_title;
        int last_battery_level;

        unsigned midi_activity[NUM_PORTS];
        bool last_midi_activity[NUM_PORTS];
};


extern expSystem theSystem;
    // in expSystem.cpp

