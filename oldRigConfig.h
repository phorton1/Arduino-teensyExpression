#ifndef __oldrig_config_h__
#define __oldrig_config_h__

#include "expSystem.h"


class oldRigConfig : public expConfig
{
    public:
        
        oldRigConfig(expSystem *pSystem);
        
        virtual void begin();

        virtual void buttonEventHandler(int row, int col, int event);

    private:
        
        int m_cur_patch_num;    // 0..14
        bool m_effect_toggle[NUM_BUTTON_COLS];
        bool m_loop_touched[NUM_BUTTON_COLS];
        int  m_loop_last_touched;
        bool m_loop_event_cleared;
        
};


#endif // !__oldrig_config_h__