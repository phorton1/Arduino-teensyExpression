#include <myDebug.h>
#include "myMidiHost.h"
#include "prefs.h"
#include "defines.h"
#include "midiQueue.h"
#include "winFtpSettings.h"


// #define HOST_CABLE_BIT  0x80


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



bool passFilter(uint32_t iii)
{
    // don't pass em if filter is not on

    msgUnion msg(iii);
    int type = msg.getMsgType();
    if (getPref8(PREF_PERF_FILTER))        // filter is on
    {
        // only accept messages from cable 0

        if (msg.isCable1())
            return 0;

        // only accept note on, note off, or pitchbends if the pref is cleared

        bool filter_bends = getPref8(PREF_PERF_FILTER_BENDS);
        if (type!=0x08 && type!=0x09 && (type!=0x0E || filter_bends))
            return 0;
    }

    // layers are independent of filter, though bends may be gone by now

    int layer_type = winFtpSettings::getSetting(FTP_SETTING_PERF_LAYER_TYPE);
    if (layer_type)
    {
        int channel = msg.getChannel() - 1;

        if (type==0x0E && (channel != 0 && channel != layer_type))
            return 0;

        int new_channel = channel >= layer_type ? 1 : 0;
        msg.i &= ~0x0f00;           // clear the old channel
        msg.i |= new_channel << 8;  // set the new channel
    }

    // send it to the teensyduino

    usb_midi_write_packed(msg.i);
    theSystem.midiActivity(INDEX_MASK_OUTPUT);
        // it IS port one, cable 0

    // if "monitor performanc" pref is set
    // enqueue it for display as PORT_INDEX_DUINO_OUTPUT0
    // with the PORT_MASK_PERFORM flag to display it differently

    if (getPref8(PREF_MONITOR_PERFORMANCE))
    {
        msg.i &= ~PORT_MASK;                            // clear the old port
        msg.i |= PORT_MASK_OUTPUT | PORT_MASK_PERFORM;  // output to teensyDuino0
        enqueueProcess(msg.i);
    }

    return 1;   // flush the usb_midi buffer
}



// made virtual in USBHost_t36.h
void myMidiHostDevice::rx_data(const Transfer_t *transfer)
{
   uint32_t len = (transfer->length - ((transfer->qtd.token >> 16) & 0x7FFF)) >> 2;

    if (len)
    {
        bool any = 0;
        bool spoof_ftp = getPref8(PREF_SPOOF_FTP);
        uint8_t ftp_port = getPref8(PREF_FTP_PORT);

        for (uint32_t i=0; i < len; i++)
        {
            uint32_t msg = rx_buffer[i];
            if (msg)
            {
                int pindex = ((msg >> 4) & PORT_INDEX_MASK) | INDEX_MASK_HOST;
                theSystem.midiActivity(pindex);

                //===========================================================
                // WRITE THE MESSAGE DIRECTLY TO THE TEENSY_DUINO MIDI DEVICE
                //===========================================================
                // if spoofing, otherwise, let the filter decide

                if (spoof_ftp)
                {
                    any = 1;
                    usb_midi_write_packed(msg);
                    theSystem.midiActivity((pindex & ~INDEX_MASK_HOST) | INDEX_MASK_OUTPUT);
                }

                //-------------------
                // Enqueue message
                //-------------------
                // We enqueue the message if (a) the port has been selected for monitoring,
                // or (b) if it is the ftp port

                msg |= PORT_MASK_HOST;
                if (getPref8(PREF_MONITOR_PORT0 + pindex) || (  // if monitoring the port, OR
                    (ftp_port == FTP_PORT_HOST) &&              // if this is the PREF_FTP_PORT==1==Host, AND
                     INDEX_CABLE(pindex)))                       // cable=1
                // if (msg & 0x10)
                {
                    enqueueProcess(msg);
                }

                //----------------------
                // output performance
                //----------------------

                if (!spoof_ftp && passFilter(msg))
                    any = 1;

            }
        }

        if (any)
            usb_midi_flush_output();
    }

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
