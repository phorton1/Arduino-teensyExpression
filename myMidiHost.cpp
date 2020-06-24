#include <myDebug.h>
#include "myMidiHost.h"
#include "defines.h"
#include "midiQueue.h"


#define HOST_CABLE_BIT  0x80


USBHost myusb;
// MIDIDevice midi1(myusb);
myMidiHostDevice midi_host(myusb);


void myMidiHostDevice::init()
{
    // Wait 1.5 seconds before turning on USB Host.  If connected USB devices
    // use too much power, Teensy at least completes USB enumeration, which
    // makes isolating the power issue easier.
    myusb.begin();
}



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

                msg |= HOST_CABLE_BIT;
                enqueueProcess(msg);
            }
        }
    }

    if (any)
        usb_midi_flush_output();
    queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);
}



#if 0   // possible code

    int cur_buffer = 0;

    void myMidiHostDevice::flush()
    {
        uint32_t tx_max = tx_size / 4;
        if (cur_buffer == 0 && tx1_count)
        {
            display(0,"flushing buffer1 tx1_count=%d",tx1_count);
            display_bytes(0,"bf1",(uint8_t *)tx_buffer1,tx_max*4);

            tx1_count = tx_max;
            queue_Data_Transfer(txpipe, tx_buffer1, tx_max*4, this);
            cur_buffer = 1;
        }
        else if (cur_buffer == 1 && tx2_count)
        {
            display(0,"flushing buffer2 tx2_count=%d",tx2_count);
            display_bytes(0,"bf2",(uint8_t *)tx_buffer2,tx_max*4);

            tx2_count = tx_max;
            queue_Data_Transfer(txpipe, tx_buffer2, tx_max*4, this);
            cur_buffer = 0;
        }
    }


    void myMidiHostDevice::write_packed(uint32_t data)
    {
        if (!txpipe) return;
        uint32_t tx_max = tx_size / 4;

        uint32_t tx_count = cur_buffer ? tx2_count : tx1_count;
        uint32_t *tx_buffer = cur_buffer ? tx_buffer2 : tx_buffer1;

        display(0,"my_write_packed(%08x) cur_buffer=%d tx_max=%d  tx_count=%d",data,cur_buffer,tx_max,tx_count);

        if (tx_count >= tx_max)
        {
            display(0,"my_write_packed() calling flush()",0);
            flush();
            tx_count = cur_buffer ? tx2_count : tx1_count;
            tx_buffer = cur_buffer ? tx_buffer2 : tx_buffer1;
            display(0,"after flush my_write_packed(%08x) cur_buffer=%d tx_max=%d  tx_count=%d",data,cur_buffer,tx_max,tx_count);

            if (tx_count >= tx_max)
            {
                my_error("could not write midi cur_buffer=%d!!!",cur_buffer);
                return;
            }
        }

        tx_buffer[tx_count++] = data;
        if (cur_buffer)
            tx2_count = tx_count;
        else
            tx1_count = tx_count;
        return;


        while (1)
        {
            uint32_t tx1 = tx1_count;
            uint32_t tx2 = tx2_count;

            display(0,"my_write_packed tx1=%d  tx2=%d",tx1,tx2);
            display_bytes(0,"bf1",(uint8_t *)tx_buffer1,tx_max*4);
            display_bytes(0,"bf2",(uint8_t *)tx_buffer2,tx_max*4);


            if (tx1 < tx_max && (tx2 == 0 || tx2 >= tx_max))
            {
                // use tx_buffer1
                tx_buffer1[tx1++] = data;
                tx1_count = tx1;
                if (tx1 >= tx_max)
                {
                    queue_Data_Transfer(txpipe, tx_buffer1, tx_max*4, this);
                }
                else
                {
                    // TODO: start a timer, rather than sending the buffer
                    // before it's full, to make best use of bandwidth
                    tx1_count = tx_max;
                    queue_Data_Transfer(txpipe, tx_buffer1, tx_max*4, this);
                }
                return;
            }

            if (tx2 < tx_max)
            {
                // use tx_buffer2
                tx_buffer2[tx2++] = data;
                tx2_count = tx2;
                if (tx2 >= tx_max)
                {
                    queue_Data_Transfer(txpipe, tx_buffer2, tx_max*4, this);
                }
                else
                {
                    // TODO: start a timer, rather than sending the buffer
                    // before it's full, to make best use of bandwidth
                    tx2_count = tx_max;
                    queue_Data_Transfer(txpipe, tx_buffer2, tx_max*4, this);
                }
                return;
            }
        }
    }
#endif  // disabled
