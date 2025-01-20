#ifndef __rigLooper_h__
#define __rigLooper_h__

#include "expSystem.h"
#include "commonDefines.h"


#define RIGLOOPER_NUM_SYNTH_BANKS   2
#define RIGLOOPER_NUM_SYNTH_PATCHES 12
#define RIGLOOPER_NUM_GUITAR_EFFECTS 4


class rigLooper : public expWindow
{
    public:

        rigLooper();

        int findPatchByName(const char *patch_name);
        void setPatchNumber(int patch_number);

        void clearGuitarEffects(bool display_only);
        void setGuitarEffect(int effect_num, bool on);

        void clearLooper(bool display_only);
        void selectTrack(int num);
        void stopLooper();
        void stopLooperImmediate();
        void loopImmediate();
        void toggleDubMode();
        void setStartMark();
        void setClipMute(int layer_num, bool on);
        void setClipVolume(int layer_num, int val);

        bool songUIAvailable()      { return !m_quick_mode; }

        const char *name()          { return "Looper Rig"; }
        const char *short_name()    { return "Looper Rig"; }

        void begin(bool warm);
        void updateUI();
        void onButtonEvent(int row, int col, int event);
        bool onRotaryEvent(int num, int val);
        void onSerialMidiEvent(int cc_num, int value);
        void onEndModal(expWindow *win, uint32_t param);

        void resetDisplay();

        void startQuickMode();
        void endQuickMode();

    private:

        static int patch_to_button(int patch_num);
        static int bank_button_to_patch(int bank, int button_num);

        // STATE

        bool m_quick_mode;
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
        int m_last_erase_state[LOOPER_NUM_TRACKS];
        int m_last_clip_vol[LOOPER_NUM_TRACKS_TIMES_LAYERS];
        int m_last_clip_mute[LOOPER_NUM_TRACKS_TIMES_LAYERS];

        // static data

        static synthPatch_t synth_patch[RIGLOOPER_NUM_SYNTH_BANKS * RIGLOOPER_NUM_SYNTH_PATCHES];
        static int guitar_effect_ccs[RIGLOOPER_NUM_GUITAR_EFFECTS];
        static const char *guitar_effect_name[RIGLOOPER_NUM_GUITAR_EFFECTS];

};


extern rigLooper rig_looper;


#endif // !__rigLooper_h__