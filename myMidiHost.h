#ifndef __myMidiHost_h__
#define __myMidiHost_h__

#include <Arduino.h>
#include <USBHost_t36.h>


class myMidiHostDevice : public MIDIDevice
{
    public:
    
        myMidiHostDevice(USBHost &host) :
            MIDIDevice(host)   {}
        uint32_t myRead(uint8_t channel=0);
            // requires slightly modified USBHost_t36.h

};


    
extern USBHost myusb;
extern myMidiHostDevice midi1;


#endif  // __myMidiHost_h__