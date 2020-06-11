#ifndef _midiHostConfig_h_
#define _midiHostConfig_h_

#include "expSystem.h"


class midiHostConfig : public expConfig
{
    public:
        
        midiHostConfig(expSystem *pSystem);

    protected:
        
        virtual const char *name()          { return "Midi Host Test Configuration"; }
        virtual const char *short_name()    { return "MidiHost Test"; }
        virtual void begin();
        virtual void updateUI();
        
        virtual void onButtonEvent(int row, int col, int event) {}
        virtual void onRotaryEvent(int num, int val) {}        
        virtual void timer_handler() {}
        
        bool draw_needed;
        bool redraw_needed;
        
};


#endif      // !_midiHostConfig_h_