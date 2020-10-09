#ifndef __rigNew_h__
#define __rigNew_h__

#include "expSystem.h"
#include "oldRig_defs.h"
#include "commonDefines.h"
    // LOOPER_NUM_TRACKS and LAYERS, TRACK_STATES, LOOP_COMMANDS, and common CC's
    // denormalized H file common to circle-Looper and Arduino-teensyExpression projects

#define NUM_SYNTH_BANKS   2
#define NUM_SYNTH_PATCHES 12
#define NUM_GUITAR_EFFECTS 4


#define TRACKS_TIMES_CLIPS    (LOOPER_NUM_TRACKS * LOOPER_NUM_LAYERS)


class rigNew : public expWindow
{
    public:

        rigNew();

        virtual bool onRotaryEvent(int num, int val);

        // support for songMachine
        // call with display_only=false from songMachine

        static int findPatchByName(const char *patch_name);
        void setPatchNumber(int patch_number);

        void clearGuitarEffects(bool display_only);
        void setGuitarEffect(int effect_num, bool on);

        void clearLooper(bool display_only);
            // other looper commands can be sent directly as
            // LOOP_COMMANDs and the UI will pick them up

    private:

        virtual const char *name()          { return "New Rig"; }
        virtual const char *short_name()    { return "New Rig"; }

        virtual void begin(bool warm);
        virtual void onButtonEvent(int row, int col, int event);
        virtual void updateUI();

        void startQuickMode();
        void endQuickMode();

        // STATE
        // No longer maintaining state in buttons
        // or relying on Button grouping mechanism

        int  m_cur_bank_num;                     // synthesizer "bank" number (modal)
        int  m_cur_patch_num;                    // 0..23  (12 patches per bank, as defined by constants)
        int  m_last_set_poly_mode;
        int  m_guitar_state[NUM_GUITAR_EFFECTS];

        int  m_dub_mode;
        int  m_stop_button_cmd;

        int  m_selected_track_num;
        int  m_track_state[LOOPER_NUM_TRACKS];   // track state
        bool m_track_flash;
        elapsedMillis m_track_flash_time;

        int  m_quick_mode;
        int  m_clip_mute[TRACKS_TIMES_CLIPS];
        int  m_clip_vol[TRACKS_TIMES_CLIPS];


        // REDISPLAY STATE

        bool m_full_redraw;

        int m_last_bank_num;
        int m_last_patch_num;
        int m_last_displayed_poly_mode;
            // opposite of ftp_poly_mode

        int  m_last_guitar_state[NUM_GUITAR_EFFECTS];

        int m_last_dub_mode;
        int m_last_stop_button_cmd;
        int m_last_track_state[LOOPER_NUM_TRACKS];

        int m_last_quick_mode;
        int m_last_clip_mute[TRACKS_TIMES_CLIPS];
        int m_last_clip_vol[TRACKS_TIMES_CLIPS];
        int m_last_erase_state[LOOPER_NUM_TRACKS];

        void resetDisplay();
        virtual void onSerialMidiEvent(int cc_num, int value);

        // static definitions

        static synthPatch_t synth_patch[NUM_SYNTH_BANKS * NUM_SYNTH_PATCHES];
        static int guitar_effect_ccs[NUM_GUITAR_EFFECTS];

        static int patch_to_button(int patch_num);
        static int bank_button_to_patch(int bank, int button_num);

        bool song_machine_running;

};


extern int_rect song_rect;
extern rigNew *theNewRig;
    // static globals for use by songMachine

#endif // !__rigNew_h__