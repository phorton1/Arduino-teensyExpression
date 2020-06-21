#ifndef __myMidiHost_h__
#define __myMidiHost_h__

#include <Arduino.h>
#include <USBHost_t36.h>


// 255 is reserved (EEPROM) value

#define MIDI_HOST_ON          0x01


class myMidiHostDevice : public MIDIDevice
    // requires slightly modified USBHost_t36.h
{
    public:
    
        myMidiHostDevice(USBHost &host) :
            MIDIDevice(host)
        {
            startup_state = 0;
        }
        
        void init();
        
        virtual void rx_data(const Transfer_t *transfer);
            // virtual override to handle USB irq,
            // write packet to teensyDuino device,
            // and enqueue packet for processing
        
        
        bool isOn()     { return startup_state & MIDI_HOST_ON; }
        
    private:
        
        int startup_state;

};


    
extern USBHost myusb;
extern myMidiHostDevice midi1;


#endif  // __myMidiHost_h__