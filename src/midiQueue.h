#ifndef _midiQueue_h_
#define _midiQueue_h_

#include "defines.h"

// ORIGINAL MESSAGES
//
// The "cable" number is communicated in the high order
//      nibble of the first byte in the USB midi message
// The FTP transmits on cables 0 and 1
//     Cable 0 is the "Fishman Triple Play" device
//         sends Active Sensing
//     Cable 1 is the "MIDIIN2 (Fishman Triple Play Device)
// myMidiHost receives both
//     if spoofing the FTP editor, both must be echoed to
//     the output device which must have 2 cables
// my TeensyDuino device now has two cables
// my SpoofFTPTeensyDuino device has two cables
// Henceforth, all communications with the FTP take place
//     on cable1 of either port
// Cable 0 is reserved for messags that MY program sends
//     like patch changes, pedals, etc
// The ipad CoreMidi mungs them all anyways.
//
// MAPPED MESSAGES
//
// For display and processing purposes, we map the cables thusly:
//
//      0x0n = input from teensyDuino on cable 0
//      0x1n = input from teensyDuino on cable 1
//      0x2n = output to teensyDuino on cable 0
//      0x3n = output to teensyDuino on cable 1
//      0x4n = input from midi host on cable 0
//      0x5n = input from midi host on cable 1
//      0x6n = output to host on cable 0
//      0x7n = output to host on cable 1
//
// thus the bottom bit of the high nibble is the original cable number
// and the high two bits are the "port" number, furhter bitwise identifiable
// as input or output ports
//
//      x 0 0 0      unused
//      0 1 0 0      host bit
//      0 0 1 0      output bit
//      0 0 0 1      original cable bit


#define PORT_MASK           0xF0
#define PORT_MASK_HOST      0x40
#define PORT_MASK_OUTPUT    0x20
#define PORT_MASK_CABLE     0x10
#define PORT_MASK_PERFORM   0x80

#define PORT_INDEX_MASK             0x7
#define PORT_INDEX_DUINO_INPUT0     0x0   // input from teensyDuino on cable 0
#define PORT_INDEX_DUINO_INPUT1     0x1   // input from teensyDuino on cable 1
#define PORT_INDEX_DUINO_OUTPUT0    0x2   // output to teensyDuino on cable 0
#define PORT_INDEX_DUINO_OUTPUT1    0x3   // output to teensyDuino on cable 1
#define PORT_INDEX_HOST_INPUT0      0x4   // input from midi host on cable 0
#define PORT_INDEX_HOST_INPUT1      0x5   // input from midi host on cable 1
#define PORT_INDEX_HOST_OUTPUT0     0x6   // output to host on cable 0
#define PORT_INDEX_HOST_OUTPUT1     0x7   // output to host on cable 1

#define INDEX_MASK_HOST     0x04
#define INDEX_MASK_OUTPUT   0x02
#define INDEX_MASK_CABLE    0x01

#define INDEX_IS_HOST(i)       (i & INDEX_MASK_HOST)
#define INDEX_IS_OUTPUT(i)     (i & INDEX_MASK_OUTPUT)
#define INDEX_CABLE(i)         (i & INDEX_MASK_CABLE)

#define PORT_OF_MSG(i)       ((i>>4) & PORT_MASK))



class msgUnion
{
    public:

        msgUnion()  { i = 0; }
        msgUnion(uint32_t msg)  { i = msg; }
        msgUnion(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) { b[0]=b0; b[1]=b1; b[2]=b2; b[3]=b3; }

        // "port" numbers are dealt with in their shifted state for performance

        void    setPort(uint8_t p)  { i |= p; }
        uint8_t getPort()           { return i & PORT_MASK; }

        bool    isHost()            { return i & PORT_MASK_HOST; }
        bool    isOutput()          { return i & PORT_MASK_OUTPUT; }
        bool    isCable1()          { return i & PORT_MASK_CABLE; }

        // for certain purposes, like processing sysex messages
        // into an array of buffers, we want an index
        // 0..7 for the eight possible port/cable numbers.
        // portIndex() is shifted for client use and has
        // macros for the bits

        uint8_t portIndex()  { return (i>>4) & PORT_INDEX_MASK; }
        uint8_t getMsgType() { return i & 0x0f; }
        uint8_t getChannel() { return (b[1] & 0xf) + 1; }

        uint8_t type()       { return b[1]; }
        uint8_t param1()     { return b[2]; }
        uint8_t param2()     { return b[3]; }

        bool isActiveSense()    { return (i & 0xff0f) == 0xfe0f; }

    union {
        uint32_t i;
        uint8_t b[4];
    };
};


extern int  showSysex;
extern bool showActiveSense;
extern bool showTuningMessages;
extern bool showNoteInfoMessages;
extern bool showVolumeLevel;
extern bool showBatteryLevel;
extern bool showPerformanceCCs;



extern void enqueueProcess(uint32_t msg);
    // called by myMidiHost::rx_data for messages from host and
    // expSystem::critical_timer_handler() for messages from device

extern void dequeueProcess();
    // called by expSystem::timer_handler() or updateUI()
    // this method is currently doing all the work
    // and the return value is ignored, there are
    // no messages eneuqued for display

extern void mySendFtpSysex(int length, uint8_t *buf);
extern void mySendMidiMessage(uint8_t msg_type, uint8_t channel, uint8_t p1, uint8_t p2);
extern void mySendDeviceProgramChange(uint8_t prog_num, uint8_t channel);
extern void mySendDeviceControlChange(uint8_t cc_num, uint8_t value, uint8_t channel);

extern void sendSerialControlChange(uint8_t cc_num, uint8_t value, const char *debug_msg);

//-----------------------------------------------------------------------------
// COMMAND/REPLY   rough notes
//-----------------------------------------------------------------------------
//
// The button pad, absent the FTP editor (I only got this good behavior one time, can't reproduce it)
//
//     BACK (towards tail piece):     .... the manual calls the "ENTER"
//              B7 3F 10 - pressed
//              B7 3F 00 - released
//     FORWARD (towards sound hole)   .. the manual calls this "BACK" (from DPad days)
//              B7 3F 11 - pressed    ... I uses this to "improve" playability
//              B7 3F 01 - released   ... but I don't know what it really does, if anything
//     UP (towards me)
//              B7 3F 12 - pressed
//              B7 3F 02 - released
//     DOWN (towards floor)
//              B7 3F 13 - pressed
//              B7 3F 03 - released
//
// About here, it went haywire, as the buttons do not act underwtandably
//
//  First time button is pressed after reboot sends
//
//      host( 8)  B7  ControlChange     1f  06  <-----
//      host( 8)  B7  ControlChange     3f  13
//      host( 8)  B7  ControlChange     3f  03
//
// Upon a fresh reboot without FTP editor running,
//      DOWN = B7  3F  nn  where nn increments
//      and it looks like the controller sends
//      out sysex patches at intervals ... weird
// And on another reboot, UP and DOWN send out
//      "C0 nn" program changes with the controllers own internal counter from 0 .. 7F
//     and holding it down repeats
// The FORWARD and BACK buttons
//      send out a bunch of stuff, including a sysex patch
//      and, apart form analyzing the sysex message, FORWARD and BACK send the same thing.
//          host( 8)  B7  ControlChange     1f  0c
//          host( 8)  B7  ControlChange     3f  00
//          host( 8)  B7  ControlChange     1f  05
//          host( 8)  B7  ControlChange     3f  02
//          host( 8)  B7  ControlChange     1f  55
//          host( 8)  B7  ControlChange     3f  00
//          host( 8)  B7  ControlChange     1f  1e
//          host( 8)  B7  ControlChange     3f  00
//          host( 2)  E1  Pitch Bend                0
//          host( 1)  E0  Pitch Bend                0
//          host( 8)  B7  ControlChange     1f  55
//          host( 8)  B7  ControlChange     3f  00
//          host          sysex len=25
//          host                f0 00 01 6e 01 43 0e 50 6f 6c 79 20 50 72 6f 67
//                              72 61 6d 20 31 06 06 01 f7
//      which does not seem to change
//      maybe I will figure this out one day.

//   Too bad, I liked the idea of context free controls ...
//
//  In this mode the FORWARD button seems to turn of Pitch Bend (though it still sends one zero per note)
//  and the back button sends bunches.  The default at boot up is to send none, and now it sends none,
//  even after pressing FORWARD, so I'm not sure if FORWARD IS functional in the context of my usage.
//
//   SHEESH this is inconsistent.
//
//         Now on fresh boot, no pitch bends, BACK turns on lots of em, rho FORWARD does not stop em.
//
//  In my final consideration on fresh boot (with no hold-downs) UP and DOWN send C0 nn program changes
//  based on an internal counter that is initialized to 0, ao rhw DOWN button does nothing iniitaially
//  on such a boot and MAYBE the BACK button merely "turns on" pitch bending and the FORWARD one turns
//  it off (though it sends out a slew of messages)
//
//
// HARDWARE MODE (hold down UP while booting) is even more complicated
//
//             makes it look b7 1f 9c
//                           b7 3f nn    might be a patch change message
//
// and I don't wannt try a factory reset or other buttons right now
//
//-------------------------------------
// and there's more!
//-------------------------------------
// it looks like there are B7 1F/3F commands to edit details of the current patch

//  B7 1F 2F      B7  3F nn     dynamics sensitivity nn = 0xA .. 0x14
//  B7 1F 30      B7  3F nn     dynamics offset nn = 0x0 .. 0x14
//  B7 1F 4F      B7  3F nn     touch sensitivity nn = 0x0 .. 0x04

//  But do I wanna go down that path?
//  I have not even really conclusively determined if any of these things have any effect in Basic Mode
//  or hardware mode, or if the Controller is just a complicated memory device ... though the "poly mode"
//  setting in the selected patch (bank) DOES have an effect on the output.



#endif // !_midiQueue_h_