#ifndef __myMidiHost_h__
#define __myMidiHost_h__

#include <Arduino.h>
#include <USBHost_t36.h>



class myMidiHostDevice : public MIDIDevice
    // requires slightly modified USBHost_t36.h
{
    public:
    
        myMidiHostDevice(USBHost &host) :
            MIDIDevice(host)
        {
        }
        
        void init();
        
        virtual void rx_data(const Transfer_t *transfer);
            // virtual override to handle USB irq,
            // write packet to teensyDuino device,
            // and enqueue packet for processing

};


    
//extern USBHost myusb;
extern myMidiHostDevice midi_host;


#endif  // __myMidiHost_h__