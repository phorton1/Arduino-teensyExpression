#ifndef _midiHostConfig_h_
#define _midiHostConfig_h_

#include "expSystem.h"



class midiHostConfig : public expConfig
{
    public:
        
        midiHostConfig();

    private:
        
        virtual const char *name()          { return "Midi Host Test Configuration"; }
        virtual const char *short_name()    { return "MidiHost Test"; }
        virtual void begin();
        virtual void updateUI();
        
        virtual void onButtonEvent(int row, int col, int event);
        
        bool draw_needed;
        bool redraw_needed;
        
};


#endif      // !_midiHostConfig_h_