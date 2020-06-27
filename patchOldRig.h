#ifndef __patchOldRig_h__
#define __patchOldRig_h__

#include "expSystem.h"


class patchOldRig : public expWindow
{
    public:

        patchOldRig();


    private:

        virtual const char *name()          { return "Old Rig"; }
        virtual const char *short_name()    { return "Old Rig"; }

        virtual void end();
        virtual void begin(bool warm);
        virtual void onButtonEvent(int row, int col, int event);
        virtual void updateUI();

        int m_cur_patch_num;    // 0..14

        int m_event_state[NUM_BUTTON_ROWS * NUM_BUTTON_COLS];
};


#endif // !__patchOldRig_h__