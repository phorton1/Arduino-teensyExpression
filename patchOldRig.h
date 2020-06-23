#ifndef __patchOldRig_h__
#define __patchOldRig_h__

#include "expSystem.h"


class patchOldRig : public expWindow
{
    public:
        
        patchOldRig();
        

    private:

        virtual const char *name() { return "Old Rig Configuration"; }
        virtual const char *short_name()    { return "Old Rig"; }
        
        virtual void begin(bool warm);
        virtual void onButtonEvent(int row, int col, int event);
        virtual void updateUI();
        
        int m_cur_patch_num;    // 0..14
};


#endif // !__patchOldRig_h__