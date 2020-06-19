#ifndef _midiQueue_h_
#define _midiQueue_h_


class msgUnion
{
    public:
        
        msgUnion()  { i = 0; }
        msgUnion(uint32_t msg)  { i = msg; }
        msgUnion(uint8_t b0, uint8_t b1, uint8_t b2, uint8_t b3) { b[0]=b0; b[1]=b1; b[2]=b2; b[3]=b3; }
        
        bool isHost()           { return (i & 0x80); }
        int  hostIndex()        { return isHost() ? 1 : 0; }
        uint8_t getMsgType()    { return i & 0x0f; }
        uint8_t getCable()      { return (i>>4) & 0x7; }
        uint8_t getChannel()    { return (b[1] & 0xf) + 1; }
        
        uint8_t type()       { return b[1]; }
        uint8_t param1()     { return b[2]; }
        uint8_t param2()     { return b[3]; }
        
        bool isActiveSense()    { return (i & 0xff0f) == 0xfe0f; }
        
    union {
        uint32_t i;
        uint8_t b[4];
    };
};


// debugging visibility fileter

#if 0
    extern int showSysex;   // 0,1, or 2 for detail
    extern bool showActiveSense;
    extern bool showTuningMessages;
    extern bool showNoteInfoMessages;
#endif

// message queue processing functions

// extern void enqueueDisplay(uint32_t msg);

extern void enqueueProcess(uint32_t msg);
extern uint32_t dequeueProcess();
    // this method is currently doing all the work
    // and the return value is ignored, there are
    // no messages eneuqued for display

// extern void processMsg(uint32_t i);
    // called by dequeueProcess
// extern void showDisplayQueue();
    // does nothing at this time, even if it was called, which it is not
// extern void showRawMessage(uint32_t i);
    // not currently used either


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