#ifndef _ftp_defs_h_
#define _ftp_defs_h_


// "CC" vslues (psram1)

#define FTP_NOTE_INFO           0x1E
#define FTP_SET_TUNING          0x1D
#define FTP_TUNING              0x3D
#define FTP_COMMAND_OR_REPLY    0x1F
#define FTP_COMMAND_VALUE       0x3F

// specific commands

#define FTP_SLIDER_POSITION     0x05
#define FTP_BATTERY_LEVEL       0x07
#define FTP_VOLUME_LEVEL        0x08
#define FTP_GET_SENSITIVITY     0x3C
#define FTP_SET_SENSITIVITY     0x42


//
//  +======+======+============================+==================+========================================================================
//  |  P0  |  P1  |  P2                        | Name             | Description                       
//  +======+======+============================+==================+========================================================================
//  |  B7  |  1E  |  xy = string, vel          | NoteInfo         | sent in context of most recent NoteOn or NoteOff message, provides
//  |      |      |                            |                  | the UI with the string number and compressed velocity of that event
//  |      |      |  from controller only      |                  | for use by the VU meter, tuner, and fretboard display.
//  +------+------+----------------------------+------------------+------------------------------------------------------------------------
//  |  B7  |  1D  |  nn=0x40 biased tuning     | SetTuning        | SetTuning is sent to notify that a note has been selected for tuning,
//  |      |  3D  |  from -0x40 to 0x40        | Tuning           |    whih is the most recent NoteOn or NoteOff message.
//  |      |      |                            |                  | Tuning then updates that value until the note is turned off 
//  |      |      |  from controller only      |                  | We maintain a list of active note_t's and my UI can check the
//  |      |      |                            |                  |    "tuning_note" global pointer to see if there is one, and if so,
//  |      |      |                            |                  |    the tuning values (note and +/-) to display.
//  +======+======+============================+==================+========================================================================
//  |  Bn  |  1F  |  command   |    reply      | "FTP Command"    | FTP_COMMAND_OR_REPLY
//  |      |  3F  |   param    } return_value  | "FTP Reply"      | FTP_COMMAND_VALUE
//  |      |      |                            |                  | 
//  |      |      |                            |                  | Most of the midi activity happens via these "FTP Command and Reply" messages
//  |      |      |                            |                  | which are generally always sent as a pair, and replied to as a pair.
//  |      |      |                            |                  | They are generally sent as commands, with a possible param, by the editor
//  |      |      |                            |                  | app, and replied to by the controller with an echo or distinct retrun value.
//  |      |      |                            |                  | 
//  +======+======+============================+==================+========================================================================
//  |  B7  |  1F  |  0z05=FTP_SLIDER_POSITION  | SliderPosition   | reply:    B7 1F 05, B7 3F nn    where nn=1, 3, or 2 (not in order)
//  |  B7  |  3F  |  1,3,2                     |                  | 
//  |      |      |                            |                  | I don't know if you can query this (with param zero)
//  |      |      |  possibly sent only        |                  | I do think the controller will report slider changes
//  |      |      |  by controller             |                  | outside of the context of the FTP editor ... which may mung
//  |      |      |                            |                  | their behavior.
//  |      |      |                            |                  |
//  |      |      |                            |                  | Note that it also has the nasty behavior of sending out 
//  |      |      |                            |                  | a midi volume change message (Bn 07 xx) on (t least) channels 1 
//  |      |      |                            |                  | and 2 each time it is changed!
//  |      |      |                            |                  |
//  +------+------+----------------------------+------------------+------------------------------------------------------------------------
//  |  B7  |  1F  |  0z07 = FTP_BATTERY_LEVEL  | GetBatteryLevel  | command:  B7 1F 07, B7 3F 00
//  |  B7  |  3F  |  00 | nn = battery level   | BatteryLevel     | reply:    B7 1F 07, B7 3F nn
//  |      |      |                            |                  | 
//  |      |      |                            |                  | The battery level return value nn is believed to be from 0x00 to 0x6A or
//  |      |      |                            |                  | something like that.  I am wathing it go down, saw 0x6B with the charger
//  |      |      |                            |                  | plugged in, and am trying to determine if the range and if there is a a
//  |      |      |                            |                  | 'charger' plugged in bit.
//  |      |      |                            |                  | 
//  |      |      |                            |                  | might be usable as "keep alive" monitor for the FTP editor
//  |      |      |                            |                  | 
//  +------+------+----------------------------+------------------+------------------------------------------------------------------------
//  |  B7  |  1F  |  0z08 = FTP_VOLUME         | GetVolume        | command:  B7 1F 08, B7 3F 00
//  |  B7  |  3F  |  00 | nn = volume levell   | Volume           | reply:    B7 1F 09, B7 3F nn
//  |      |      |                            |                  | 
//  |      |      |                            |                  | where nn is the value of the rotary volume control on the controller,
//  |      |      |                            |                  | from 0..127.   Dont ask me why its needed apart from *maybe* startup,
//  |      |      |                            |                  | since barely touching it sends out a slew of CC's on channel 1, but
//  |      |      |                            |                  | I guess the controller has it's own back channels to everything.
//  +------+------+----------------------------+------------------+------------------------------------------------------------------------
//  |  B7  |  1F  | 0x3C = GET_SENSITIVITY     | GetSensitivity   | command:  B7 1F 3C, B7 3F xx  = string 0..5
//  |  B7  |  3F  | xx = string | yy = level   |                  | reply:    B7 1F 3C, B7 3F yy  = level, 0..14
//  |      |      |                            |                  | 
//  |      |      |                            |                  | this is sent to the controller to ask it for the sensitivy level
//  |      |      |                            |                  | for a given string.
//  |      |      |                            |                  | 
//  |      |      |                            |                  | my state machine looks for this and 0x42 from the host to maintain 
//  |      |      |                            |                  | the array of sensetivities in memory for UI purposes
//  +------+------+----------------------------+------------------+------------------------------------------------------------------------
//  |  B7  |  1F  | 0x42 = SET_SENSITIVITY     | SetSensitivity   | command:  B7 1F 42, B7 3F xy    where x is the string and y is the level
//  |  B7  |  3F  | xy = string/level | echo   |                  | reply:    B7 1F 42, B7 3F xy    and the controller echos the command
//  |      |      |                            |                  | 
//  |      |      |                            |                  | this is sent to the controller to set the sensitivy level
//  |      |      |                            |                  | for a given string. It replies with an echo.
//  |      |      |                            |                  | 
//  |      |      |                            |                  | my state machine looks for this and 0x3C from the host to maintain 
//  |      |      |                            |                  | the array of sensetivities in memory for UI purposes
//  |      |      |                            |                  | 
//  |      |      |                            |                  | 
//  |      |      |                            |                  | 
//  |      |      |                            |                  | 
//  |      |      |                            |                  | 
//  |      |      |                            |                  | 
//  |      |      |                            |                  | 
//  |      |      |                            |                  | 
//  |      |      |                            |                  | 



//-----------------------------------------------------------------------------
// POLY (MONO) MODE
//-----------------------------------------------------------------------------
//
//  poly = all strings on channel 1
//  mono = each string on channel 1..6
//  
//  DOES NOT REQUIRE BOOTING CONTROLLER WITH UP BUTTON PRESSED
//  which puts you in "hardware" mode.   The FTP accepts patch
//  and bank changes when booted normally.
//  
//  When booted normally it comes up in hardware patch #1, which
//  is PolyProgram1, which has it's poly/mono bit set to "poly".
//  
//  At this time the full first bank has that setting.
//  And the full second bank are called "MonoProgram1..n" and
//  have the bit set to "mono".
//  
//  So, to effect a change to mono (each note on a channel) mode,
//  you just have to select the second bank, or for poly, the 1st.
//  Which are 0 based, so you send out a "bank change" midi miessage
//  
//  
//  BANK CHANGE, sent in two messages
//  
//      Bx 00 yy ... x is channel, yy is bank MSB
//      Bx 20 zz ... x is channel, zz is bank LSB
//  
//  PATCH CHANGE
//  
//      Cx yy  ... x is channel yy is patch number
//      
//  FTP
//  
//      The FTP receives it's patch/bank messages on channel 1.
//      It requies that you also send a patch change
//      after you set the bank LSB.  It will respond with
//      a 25 byte "patch" name sysex.  So, skipping the MSB:
//      
//      poly mode:
//          
//          B0 20 00
//          C0 00 00
//  
//      mono mode:
//      
//          B0 20 01
//          C0 00 00
//  
//

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
        
        
    
//==============================================================================
//==============================================================================
// PATCHES
//==============================================================================
//==============================================================================

typedef struct 
    // Each data patch has an array of five of these substructures in it.
    // Unlike the UI, which puts the "pedal" split at the end, the "pedal"
    // is the 0th element in the array, and the splits 1-4 follow in slots
    // 1-4 in the array.
{
	uint8_t pgm_change;     // 0..127
	uint8_t bank_lsb;       // 0..127
	uint8_t bank_msb;       // 0..127
	uint8_t pitch_bend;     // 0=auto, 1=smooth, 2=stepped, 3=trigger
	uint8_t transpose;      // 24=none, plus or minus 24 1/2 steps (up or down 2 octaves)
	uint8_t midi_volume;    // 0=unchecked, 1..126, weirdness at 127
                            // goes to zero and sets split bit in main patch max_volume
	uint8_t dyn_sens;       // Dynamics Sensitivity 0x0A..0x14 (10..20) weird
	uint8_t dyn_offs;       // Dynamic Offset 0..20
	uint8_t midi_reverb;    // 0=unchecked, 1..126, weirdness at 127
                            // goes to zero and sets split bit in main patch max_reverbe
} split_section_t;


typedef struct  // 142 byte "data" packet (subpatch)
    // This structure is stored in banks 0 and 1.  The "hardware poly" and
    // "hardware mono" patches match this structure.  For each of these, there is
    // another structure patch_buffer1, which I call the "name" patch, which is
    // just the header, the name length byte, the name, then  the checksum in a
    // similarly sized (142 byte) packet stored in banks 2 and 3.
    //
    // So the "data" for "Poly Program n" is in bank(0) patch(n-1) whereas it's "name"
    // is bank(2) patch(n-1) ... "Mono Program n" is in (1,n-1) and (3,n-1)
{
	uint8_t header1[6];	// F0 00 01 6E 01 21
	uint8_t bank_num;                   // 0=hardware poly, 1=hardware mono
	uint8_t patch_num;                  // patch number within bank (0..127 .. only to 112 for mono bank?!?)
	uint8_t pedal_mode; 	            // HoldUp(2), HoldDown(3), Alternate(4), Loop(6), DontBlockNewNotes(1), BlockNewNotes(0)
	uint8_t fret_range_low_up_12;       // the range of frets, from the lowest fret (open position) for hardware patches 1 & 2
	uint8_t fret_range_high_down_34;    // the range of frets from 1f down to this number, for hardware patches 3 and 4.
	uint8_t string_range_12; 		    // the strings covered by the red hardware patch1 (as opposed to yellow hardware patch 2)
	uint8_t string_range_34;   		    // the strings covered by the blue hardware patch3 (as opposed to green hardware patch 4)
	uint8_t azero0;                     // arpeggio mode which is "SEQUENTIAL" in all csv file examples
	uint8_t arp_speed;                  // arpeggio speed gleened from csv file
	uint8_t arp_length;	                // arp. length gleened from csv file (arpegio length?)
	uint8_t touch_sens;                 // 0..4 Default(4)
	uint8_t poly_mode;                  // 0=mono, 1=poly  (Default 1 for bank0, 0 for bank1)
   
	split_section_t split[5];

	uint8_t azero1;         // a zero .. I have never seen this byte change
	uint8_t seqbytes[64];   // 64 bytes from 0..0x3f .. I have never seen these bytes change
	uint8_t program[8];	    // the word "Program " with trailing space ... I have never seen these bytes change
	uint8_t azero2;         // a zero .. I have never seen this byte change
	uint8_t max_volume;     // bitwise bits set if split midi_volume *would be* 127
                            // split1=0x08, split2=0x04, split3=0x02, split1=0x01, pedal=0x10
	uint8_t max_reverb;     // bitwise bits set if split midi_reverbe *would be* 127
                            // split1=0x08, split2=0x04, split3=0x02, split1=0x01, pedal=0x10
	uint8_t checksum[2];    // two byte checksum
	uint8_t endmidi;        // 0xf7
    
}   patch_buffer0_t;


const uint8_t FTP_CODE_READ_PATCH = 0x01;	// get patch from controller (short message)
const uint8_t FTP_CODE_ACK = 0x11;			// ack from the controller
const uint8_t FTP_CODE_WRITE_PATCH = 0x41;	// write patch to controller
const uint8_t FTP_CODE_PATCH_REPLY = 0x21;	// patch request reply from controller

// const uint8_t FTP_CODE_UNKNOWN = 0x02;		// ? clear the patch ?
// const uint8_t FTP_CODE_ERROR1 = 0x12;		// ? error ?
// const uint8_t FTP_CODE_ERROR2 = 0x22;		// ? Error from controller ?




uint8_t  ftpRequestPatch[]	= { 0xF0, 0x00, 0x01, 0x6E, 0x01, FTP_CODE_READ_PATCH, 0x00, 0x00, 0xf7 };
         // bytes 6 and 7 (0 based) are the bank, and patch, respectively
         
         

#endif // !_ftp_defs_h_

