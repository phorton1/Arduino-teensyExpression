#include <myDebug.h>
#include "myMidiHost.h"


USBHost myusb;
// MIDIDevice midi1(myusb);
myMidiHostDevice midi1(myusb);
    

extern void enqueueProcess(uint32_t msg);
    // in expSystem.cpp



// made virtual in USBHost_t36.h
void myMidiHostDevice::rx_data(const Transfer_t *transfer)
{
    bool any = 0;
    uint32_t len = (transfer->length - ((transfer->qtd.token >> 16) & 0x7FFF)) >> 2;
    for (uint32_t i=0; i < len; i++)
    {
        uint32_t msg = rx_buffer[i];
        if (msg)
        {
            //===========================================================
            // WRITE THE MESSAGE DIRECTLY TO THE TEENSY_DUINO MIDI DEVICE
            //===========================================================

            any = 1;
            usb_midi_write_packed(msg);	
            
            // we just handle the mssages from the FTP controller cable 1
            // as it is the one that has the active sense messages and the
            // are completely duplicated on cable 0

            if (msg & 0x10)
            {
                // prh - set the high order bit of the "cable" to indicate
                // this came from the host ...
                
                msg |= 0x80;
                enqueueProcess(msg);
            }
            
        }
    }

    if (any)
        usb_midi_flush_output();
    queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);
}

