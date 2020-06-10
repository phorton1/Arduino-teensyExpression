#ifndef __oldrig_config_h__
#define __oldrig_config_h__

#include "expSystem.h"


class oldRigConfig : public expConfig
{
    public:
        
        oldRigConfig(expSystem *pSystem);
        virtual const char *name() { return "Old Rig Configuration"; }
        virtual const char *short_name()    { return "Old Rig"; }
        
        
        virtual void begin();

        virtual void onButtonEvent(int row, int col, int event);
        virtual void onPedalEvent(int num, int val);
        virtual void updateUI();
        

    private:
        
        int m_cur_patch_num;    // 0..14
        bool m_effect_toggle[NUM_BUTTON_COLS];
        bool m_loop_touched[NUM_BUTTON_COLS];
        int  m_loop_last_touched;
        
};


#endif // !__oldrig_config_h__