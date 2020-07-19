#ifndef __patchNewRig_h__
#define __patchNewRig_h__

#include "expSystem.h"
#include "oldRig_defs.h"


class patchNewRig : public expWindow
{
    public:

        patchNewRig();


    private:

        virtual const char *name()          { return "New Rig"; }
        virtual const char *short_name()    { return "New Rig"; }

        virtual void end();
        virtual void begin(bool warm);
        virtual void onButtonEvent(int row, int col, int event);
        virtual void updateUI();

        int m_cur_patch_num;    // 0..14
        int m_last_patch_num;
        bool m_full_redraw;

        int m_event_state[NUM_BUTTON_ROWS * NUM_BUTTON_COLS];

        // static definitions, though currently same between old and new rigs

        static synthPatch_t synth_patch[NUM_BUTTON_COLS * 3];
        static int guitar_effect_ccs[NUM_BUTTON_COLS];
        static int loop_ccs[NUM_BUTTON_COLS];

};


#endif // !__patchOldRig_h__