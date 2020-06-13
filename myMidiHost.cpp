#include "myMidiHost.h"


USBHost myusb;
USBHub hub1(myusb);
USBHub hub2(myusb);
// MIDIDevice midi1(myusb);
myMidiHostDevice midi1(myusb);
    


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
    // println("read: ", n, HEX);
    return n;
}
