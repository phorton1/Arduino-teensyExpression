#include <myDebug.h>
#include "myMidiHost.h"


USBHost myusb;
// MIDIDevice midi1(myusb);
myMidiHostDevice midi1(myusb);
    

extern void enqueueProcess(uint32_t msg);
    // in expSystem.cpp

#define SPOOF_FTP_BATTERY  0
#if SPOOF_FTP_BATTERY
    uint8_t ftp_command = 0;
    uint8_t battery_level = 0x4c;
    int direction = -1;
#endif


//  071fb79b

#define FTP_COMMAND_MASK 0x001Fb71b
#define FTP_VALUE_MASK   0x003Fb71b

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

            #if SPOOF_FTP_BATTERY       // spoof the battery
                if ((msg & 0x00ffffff) == FTP_COMMAND_MASK)
                {
                    ftp_command = (msg >> 24) & 0xff;
                    // display(0,"cmd=%02x",ftp_command);
                }
                else if ( ((msg & 0x00ffffff) == FTP_VALUE_MASK) &&
                      ftp_command == 0x07)
                {
                    display(0,"spoof battery %02x",battery_level);
                    msg &= 0x00FFFFFF;
                    msg |= battery_level << 24;
                    
                    battery_level += direction;
                    if (battery_level == 0x3d || battery_level == 0x70)
                    {
                        direction = -direction;
                    }
                }
            #endif
            
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

