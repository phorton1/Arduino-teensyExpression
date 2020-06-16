#ifndef __myMidiHost_h__
#define __myMidiHost_h__

#include <Arduino.h>
#include <USBHost_t36.h>


class myMidiHostDevice : public MIDIDevice
    // requires slightly modified USBHost_t36.h
{
    public:
    
        myMidiHostDevice(USBHost &host) :
            MIDIDevice(host)   {}
        
        #if 0   // obsolete
            virtual uint32_t myRead(uint8_t channel=0);
        #endif
        virtual void rx_data(const Transfer_t *transfer);

};


    
extern USBHost myusb;
extern myMidiHostDevice midi1;


#endif  // __myMidiHost_h__