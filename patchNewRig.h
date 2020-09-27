#ifndef __patchNewRig_h__
#define __patchNewRig_h__

#include "expSystem.h"
#include "oldRig_defs.h"

// prh - the loop pedal max should be 127 (not current default) for
// newRig ...

// 2020-08-13 initial re-implementation of New Rig
//
// - introduce two banks of 12 synth patches as per quick press of top right button
// - introduce iPad program change button via 2nd from top right button
// - introduce "quick mode" with press of 3rd button from top right button
//
// "Quick Mode" is a mode that times out after approximately 3 seconds if not
// utilized.  When in Quick mode the loop buttons remain mapped the same, so
// that you can stop a loop, and the the volume pedals continue to work (since
// that behaivor is in global pedals.cpp).  A second press of the "Quick Mode"
// button also returns to the default modal behavior.
//
// At this time "Quick Mode" presents four purple/green/red (save/down/up)
// triplets of buttons on the first top three rows of the keyboard.
// The green and red buttons allow you to adjust the relative volume
// for each channel.   The purple button allows you to save it as the
// default in EEPROM.


#define NUM_SYNTH_BANKS   2
#define NUM_SYNTH_PATCHES 12


class patchNewRig : public expWindow
{
    public:

        patchNewRig();

        virtual bool onRotaryEvent(int num, int val);

    private:

        virtual const char *name()          { return "New Rig"; }
        virtual const char *short_name()    { return "New Rig"; }

        virtual void end();
        virtual void begin(bool warm);
        virtual void onButtonEvent(int row, int col, int event);
        virtual void updateUI();

        void startQuickMode();
        void endQuickMode();

        bool m_quick_mode;                  // are we in "quick mode"
        bool m_last_quick_mode;             // for redrawing
        int  m_last_relative_vol[4];

        int m_last_set_poly_mode;
        int m_last_displayed_poly_mode;
            // keeps track of mono mode (opposite of ftp_poly_mode)
            // changes to modo/poly mode as needed for given patch definition
            // oldRig should check separately if it needs to send out the polymode

        elapsedMillis m_quick_mode_time;    // for how long?

        int m_cur_bank_num;     // synthesizer "bank" number (modal)
        int m_cur_patch_num;    // 0..11  (12 patches per bank, as defined by constants)

        int m_last_bank_num;
        int m_last_patch_num;
        bool m_full_redraw;

        int m_event_state[NUM_BUTTON_ROWS * NUM_BUTTON_COLS];
            // it is a bad idea to store state in the buttons
            // even if it means denormalizing and duplicating the values

        bool m_dub_mode;
        bool m_last_dub_mode;
        int m_selected_track_num;

        int m_track_state[4];
        int m_last_track_state[4];
        bool m_track_flash;


        elapsedMillis m_track_flash_time;


        void clearLooper();
        virtual void onSerialMidiEvent(int cc_num, int value);

        // static definitions, though currently same between old and new rigs

        static synthPatch_t synth_patch[NUM_SYNTH_BANKS * NUM_SYNTH_PATCHES];
        static int guitar_effect_ccs[NUM_BUTTON_COLS];
        static int loop_ccs[NUM_BUTTON_COLS];

        static int patch_to_button(int patch_num);
        static int bank_button_to_patch(int bank, int button_num);
};


#endif // !__patchNewRig_h__