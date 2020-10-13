#ifndef __rigLooper_h__
#define __rigLooper_h__

#include "expSystem.h"
#include "rigBase.h"
#include "commonDefines.h"
    // LOOPER_NUM_TRACKS and LAYERS, TRACK_STATES, LOOP_COMMANDS, and common CC's
    // denormalized H file common to circle-Looper and Arduino-teensyExpression projects

#define RIGLOOPER_NUM_SYNTH_BANKS   3
#define RIGLOOPER_NUM_SYNTH_PATCHES 12
#define RIGLOOPER_NUM_GUITAR_EFFECTS 4


class rigLooper : public rigBase
{
    public:

        rigLooper();

protected:

        // rigBase implementation
        // support for songMachine

        virtual int findPatchByName(const char *patch_name);
        virtual void setPatchNumber(int patch_number);

        virtual void clearGuitarEffects(bool display_only);
        virtual void setGuitarEffect(int effect_num, bool on);

        virtual void clearLooper(bool display_only);

        virtual void selectTrack(int num);
        virtual void setClipMute(int layer_num, bool on);
        virtual void setClipVolume(int layer_num, int val);

        virtual bool songUIAvailable()      { return !m_quick_mode; }

    private:

        virtual const char *name()          { return "Looper Rig"; }
        virtual const char *short_name()    { return "Looper Rig"; }

        virtual void begin(bool warm);
        virtual void updateUI();
        virtual void onButtonEvent(int row, int col, int event);
        virtual bool onRotaryEvent(int num, int val);
        virtual void onSerialMidiEvent(int cc_num, int value);
        virtual void onEndModal(expWindow *win, uint32_t param);

        void resetDisplay();

        void startQuickMode();
        void endQuickMode();

        static int patch_to_button(int patch_num);
        static int bank_button_to_patch(int bank, int button_num);

        // STATE

        bool m_quick_mode;
        bool m_quantiloop_mode;
        const char *m_pending_open_song;

        int  m_cur_bank_num;                     // synthesizer "bank" number (modal)
        int  m_cur_patch_num;                    // 0..23  (12 patches per bank, as defined by constants)
        int  m_last_set_poly_mode;

        int  m_guitar_state[RIGLOOPER_NUM_GUITAR_EFFECTS];

        int  m_dub_mode;
        int  m_stop_button_cmd;
        int  m_selected_track_num;
        int  m_track_state[LOOPER_NUM_TRACKS];   // track state
        int  m_clip_mute[LOOPER_NUM_TRACKS_TIMES_LAYERS];
        int  m_clip_vol[LOOPER_NUM_TRACKS_TIMES_LAYERS];
        bool m_track_flash;

        elapsedMillis m_track_flash_time;

        // REDISPLAY STATE

        bool m_full_redraw;
        int m_last_quick_mode;
        int m_last_song_state;

        int m_last_bank_num;
        int m_last_patch_num;
        int m_last_displayed_poly_mode;

        int  m_last_guitar_state[RIGLOOPER_NUM_GUITAR_EFFECTS];

        int m_last_dub_mode;
        int m_last_stop_button_cmd;
        int m_last_track_state[LOOPER_NUM_TRACKS];
        int m_last_clip_mute[LOOPER_NUM_TRACKS_TIMES_LAYERS];
        int m_last_clip_vol[LOOPER_NUM_TRACKS_TIMES_LAYERS];
        int m_last_erase_state[LOOPER_NUM_TRACKS];

        // static data

        static synthPatch_t synth_patch[RIGLOOPER_NUM_SYNTH_BANKS * RIGLOOPER_NUM_SYNTH_PATCHES];
        static int guitar_effect_ccs[RIGLOOPER_NUM_GUITAR_EFFECTS];
        static const char *guitar_effect_name[RIGLOOPER_NUM_GUITAR_EFFECTS];

};




#endif // !__rigLooper_h__