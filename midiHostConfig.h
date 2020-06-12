#ifndef _midiHostConfig_h_
#define _midiHostConfig_h_

#include "expSystem.h"
#include <USBHost_t36.h>



#define FAST_ECHO_MIDI    1
    // 32bit words in timer_handler()
#define PASS_THRU    0
    // unworking calls from onXXX methods



class myMidiHostDevice : public MIDIDevice
{
    public:
    
        myMidiHostDevice(USBHost &host) :
            MIDIDevice(host)   {}

        #if FAST_ECHO_MIDI
            uint32_t myRead(uint8_t channel=0);
        #endif
            
};




class midiHostConfig : public expConfig
{
    public:
        
        midiHostConfig(expSystem *pSystem);

    protected:
        
        virtual const char *name()          { return "Midi Host Test Configuration"; }
        virtual const char *short_name()    { return "MidiHost Test"; }
        virtual void begin();
        virtual void end();
        virtual void updateUI();
        
        virtual void onButtonEvent(int row, int col, int event);
        virtual void onRotaryEvent(int num, int val) {}        
        virtual void timer_handler();
        
        bool draw_needed;
        bool redraw_needed;
        
};


#endif      // !_midiHostConfig_h_