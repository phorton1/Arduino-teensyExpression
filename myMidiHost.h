#ifndef __myMidiHost_h__
#define __myMidiHost_h__

#include <Arduino.h>
#include <USBHost_t36.h>

// there are four bits avaiable for the cable
// the FTP sends out on two cables, 0, and 1
// 1 is the main one with active sense.

// we echo everything from the FTP to the editor
// but don't want to see duplicates

// the FTP editor sends out on cable 1 only

// and my program sends out to cable 1 as well,
// to the controller, or cable 0 as "teensyExpression"




// 
#define HOST_CABLE_BIT  0x80



class myMidiHostDevice : public MIDIDevice
    // requires slightly modified USBHost_t36.h
{
    public:
    
        myMidiHostDevice(USBHost &host) :
            MIDIDevice(host)   {}
        
        virtual void rx_data(const Transfer_t *transfer);
            // virtual override to handle USB irq,
            // write packet to teensyDuino device,
            // and enqueue packet for processing
        #if 0   // disabled
            virtual void write_packed(uint32_t data);
            void flush();
        #endif
};


    
extern USBHost myusb;
extern myMidiHostDevice midi1;


#endif  // __myMidiHost_h__