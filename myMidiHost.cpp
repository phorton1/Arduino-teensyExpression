#include <myDebug.h>
#include "myMidiHost.h"


USBHost myusb;
// USBHub hub1(myusb);          // turns out that the hub devices are superflous
// USBHub hub2(myusb);
// MIDIDevice midi1(myusb);
myMidiHostDevice midi1(myusb);
    


#if USE_MIDI_HOST_IRQ

    // outputs on multiple cables

    // made virtual in USBHost_t36.h
    void myMidiHostDevice::rx_data(const Transfer_t *transfer)
        // This method IS the host midi device IRQ handler.
        // It has been copied and modified to
        //      - use display() and display_bytes() instead of USBHost::println(), print() and print_bytes()
        //        which is now commented out
        //      - filter out active sense messages
        //        which is now commented out
        //  AND - SEND the 32bit messages directly to the device as they are received.
        //
        // It seems to work.
        //
        // The messages are queued, but only, I guess for UI.
        // Note that Paul's (this) code allows for the queue buffer
        // to overflow.  Thats part of why I increased RX_QUEUE_SIZE
        // from 80 to 2048. But that should be fixed here to give
        // at least one error message.
    {
        uint32_t len = (transfer->length - ((transfer->qtd.token >> 16) & 0x7FFF)) >> 2;
        
        bool show_it = 0;
        
        if (((uint8_t *)transfer->buffer)[1] == 0xFE)        // don't show active sense
            show_it = 0;
        
        if (show_it)
        {
            display(0,"myMidiHostDevice Receive",0);
            display_bytes(0,"<--",(uint8_t *)transfer->buffer,len * 4);
        }
        
        uint32_t head = rx_head;
        uint32_t tail = rx_tail;
        for (uint32_t i=0; i < len; i++)
        {
            uint32_t msg = rx_buffer[i];
            if (msg)
            {
                //===========================================================
                // WRITE THE MESSAGE DIRECTLY TO THE TEENSY_DUINO MIDI DEVICE
                //===========================================================
    
                usb_midi_write_packed(msg);	
                usb_midi_flush_output();
    
                //===========================================================
                // pauls code
                
                if (++head >= RX_QUEUE_SIZE) head = 0;
                rx_queue[head] = msg;
            }
        }
        rx_head = head;
        rx_tail = tail;
        uint32_t avail = (head < tail) ? tail - head - 1 : RX_QUEUE_SIZE - 1 - head + tail;
        
        if (show_it)
            display(0,"    avail=%d", avail);
        
        if (avail >= (uint32_t)(rx_size>>2))
        {
            // enough space to accept another full packet
            // if (show_it)
            //     display(0,"queue another receive packet",0);
            queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);
            rx_packet_queued = true;
        }
        else
        {
            // queue can't accept another packet's data, so leave
            // the data waiting on the device until we can accept it
            // if (show_it)
            
            if (show_it)
                display(0,"wait to receive more packets",0);
            rx_packet_queued = false;
        }
    }

#endif   // USE_MIDI_HOST_IRQ

    // but only outputs on one cable?

    // made virtual in USBHost_t37.h
    uint32_t myMidiHostDevice::myRead(uint8_t channel)
    {
        uint32_t n, head, tail, avail;
        // uint32_t ch,bl,type1,type2;
    
        head = rx_head;
        tail = rx_tail;
        if (head == tail) return false;
        if (++tail >= RX_QUEUE_SIZE) tail = 0;
        n = rx_queue[tail];
        rx_tail = tail;
        if (!rx_packet_queued && rxpipe)
        {
            avail = (head < tail) ? tail - head - 1 : RX_QUEUE_SIZE - 1 - head + tail;
            if (avail >= (uint32_t)(rx_size>>2))
            {
                __disable_irq();
                queue_Data_Transfer(rxpipe, rx_buffer, rx_size, this);
                __enable_irq();
            }
        }
        // if (n) display(0,"myRead(%d)=%08x",channel,n);
        return n;
    }

// #endif