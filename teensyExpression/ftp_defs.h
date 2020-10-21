#ifndef _ftp_defs_h_
#define _ftp_defs_h_


// "CC" vslues (psram1)

#define FTP_NOTE_INFO           0x1E
#define FTP_SET_TUNING          0x1D
#define FTP_TUNING              0x3D
#define FTP_COMMAND_OR_REPLY    0x1F
#define FTP_COMMAND_VALUE       0x3F

// specific commands

    // 0x04 editor sends at startup
#define FTP_CMD_EDITOR_MODE             0x04    // documented but not understood
#define FTP_CMD_SLIDER_POSITION         0x05    // documented
#define FTP_CMD_BATTERY_LEVEL           0x07    // documented
#define FTP_CMD_VOLUME_LEVEL            0x08    // documented
    // 0x0c host sends when controller turns on
    // 0x1e host sends when controller turns on
#define FTP_CMD_DYNAMICS_SENSITIVITY    0x2F
#define FTP_CMD_SET_PGM_NUMBER          0x28
#define FTP_CMD_SET_BANK_LSB            0x29
#define FTP_CMD_SET_BANK_MSB            0x2A
#define FTP_CMD_PITCHBEND_MODE          0x2B
#define FTP_CMD_TRANSPOSE               0x2C
#define FTP_CMD_MIDI_VOLUME             0x2D
#define FTP_CMD_DYNAMICS_OFFSET         0x30
#define FTP_CMD_MIDI_REVERB             0x31
#define FTP_CMD_SET_SENSITIVITY         0x3C    // documented
    // 0x3d editor sends at startup
#define FTP_CMD_POLY_MODE               0x3f    // 0=mono, 1=poly
    // 0x40 editor sends at startup
    // 0x41 editor sends at startup
#define FTP_CMD_GET_SENSITIVITY         0x42    // documented
#define FTP_CMD_BLOCK_MIDI_NOTES        0x46    // 1=block new midi notes 'off', 0=block new midi notes "on"
    // 0x47 releated to splits?
    // 0x48 related to splits?
#define FTP_CMD_TOUCH_SENSITIVITY       0x4f    // 0..4 for pick and finger_style 5..9  (default 4)
#define FTP_CMD_SPLIT_NUMBER            0x52    // 1 based split number, pedal is zero
    // 0x55 host sends when controller turns on .. ack?
    // 0x5b editor sends at startup  could be related to string 0
    // 0x5c editor sends at startup  could be related to string 1
    // 0x5d editor sends at startup  could be related to string 2
    // 0x5e editor sends at startup  could be related to string 3
    // 0x5f editor sends at startup  could be related to string 4
    // 0x60 editor sends at startup  could be related to string 5
    // 0x61 editor sends at startup
    // 0x62 editor sends at startup
    // 0x63 editor sends at startup
    // 0x60 editor sends at startup
    // 0x6b editor sends at startup






//  +======================================================================================================================================
//  | CONTINUOUS CONTROLLERS (CC's not 1F/3F)
//  +======+======+============================+==================+========================================================================
//  |  P0  |  P1  |  P2                        | Name             | Description
//  +======+======+============================+==================+========================================================================
//  |  B7  |  1E  |  xy = string, vel          | NoteInfo         | sent in context of most recent NoteOn or NoteOff message, provides
//  |      |      |                            |                  | the UI with the string number and compressed velocity of that event
//  |      |      |  from controller only      |                  | for use by the VU meter, tuner, and fretboard display.
//  +------+------+----------------------------+------------------+------------------------------------------------------------------------
//  |  B7  |  1D  |  nn=0x40 biased tuning     | SetTuning        | SetTuning is sent to notify that a note has been selected for tuning,
//  |      |  3D  |  from -0x40 to 0x40        | Tuning           |    whih is the most recent NoteOn or NoteOff message.
//  |      |      |  (-64 to 64)               |                  | Tuning then updates that value until the note is turned off
//  |      |      |  from controller only      |                  | We maintain a list of active note_t's and my UI can check the
//  |      |      |                            |                  |    "tuning_note" global pointer to see if there is one, and if so,
//  |      |      |                            |                  |    the tuning values (note and +/-) to display.
//  +======================================================================================================================================


//  +======================================================================================================================================
//  | FTP COMMANDS
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
//  |      |  1F  |  0x04                      | Editor Mode      | For lack of a better term, I call this "editor" mode
//  |      |  3F  |  0x02  = tuner on          |                  | Might better be called "TUNER_MODE", though I suspect it is
//  |      |      |  0x00  = tuner off         |                  | interpreted bitwise as a tri-state value.
//  |      |      |                            |                  |
//  |      |      |                            |                  | The only parameters I know which "do" something are "0x02" which
//  |      |      |                            |                  | turns the tuner on, and 0x00 which turns the tuner off.
//  |      |      |                            |                  |
//  |      |      |                            |                  | Without this 0x02 you don't get the Tuning messages needed for the Tuner UI.
//  |      |      |                            |                  |
//  |      |      |                            |                  | But as I said, I suspect there is more going on here than just tuning mode.
//  |      |      |                            |                  | When this message is sent the controller also may send out a slew of stuff, on channels 1-7 and 11-16 which are probably "performance state"
//  |      |      |                            |                  |
//  |      |      |                            |                  |      host(1, 1)  B0  ControlChange     65  00
//  |      |      |                            |                  |      host(1, 1)  B0  ControlChange     64  00
//  |      |      |                            |                  |      host(1, 1)  B0  ControlChange     06  0c
//  |      |      |                            |                  |      host(1, 1)  B0  ControlChange     26  00
//  |      |      |                            |                  |
//  |      |      |                            |                  | These four messages are sent to those 13 channels, regardless of poly mode
//  |      |      |                            |                  | Yet when the same command is sent again, you DON'T get thses "performance state"
//  |      |      |                            |                  | values, like it only sends them out on a "change" to this mode.
//  |      |      |                            |                  |
//  |      |      |                            |                  | The controller also sends out some other 1f/3C replies, including
//  |      |      |                            |                  | unknown 0c, slider position 05, the ubiquitous unknown 55, unknown 1e,
//  |      |      |                            |                  | pitch bends on channel 1 & 2, another 55, and a 25 byte sysex with the
//  |      |      |                            |                  | current patch name.
//  |      |      |                            |                  |
//  |      |      |                            |                  |
//  |      |      |                            |                  |
//  |      |      |                            |                  |

//  |      |      |                            |                  |
//  |      |      |                            |                  | param values
//  |      |      |                            |                  | 0x00 - also sends out some message on channel 8, including the sysex for a patch
//  |      |      |                            |                  |        turns off tuning and pitch bends
//  |      |      |                            |                  | 0x01 - does not send out patches
//  |      |      |                            |                  | 0x02 - only sends performance controller stuff first time?
//  |      |      |                            |                  | 0x03 - does nothing? (but IS echoed)
//  |      |      |                            |                  | 0x04 - checked bitwise,looks like 0 -
//  |      |      |                            |                  |
//  |      |      |                            |                  | 0x02 - definitely turns the tuner on ...
//  |      |      |                            |                  |
//  +======+======+============================+==================+========================================================================
//  |  B7  |  1F  |  0z05                      | SliderPosition   | reply:    B7 1F 05, B7 3F nn    where nn=1, 3, or 2 (not in order)
//  |  B7  |  3F  |  1,3,2                     | (get or notif)   |
//  |      |      |                            |                  | You can query this nn==0
//  |      |      |  value sent only           |                  | The controller reports this on default startup.
//  |      |      |  by controller             |                  | and it is part of the Editor startup conversation.
//  |      |      |                            |                  |
//  |      |      |                            |                  | Changing the slider position has the additional behavior of sending
//  |      |      |                            |                  | midi volume CC messages (Bn 07 xx) on channels 1 and 2 in poly mode,
//  |      |      |                            |                  | and 1-7 AND 11-16 in mono mode.
//  |      |      |                            |                  |
//  +------+------+----------------------------+------------------+------------------------------------------------------------------------
//  |  B7  |  1F  |  0z07                      | GetBatteryLevel  | command:  B7 1F 07, B7 3F 00
//  |  B7  |  3F  |  00 | nn = battery level   | BatteryLevel     | reply:    B7 1F 07, B7 3F nn
//  |      |      |                            |                  |
//  |      |      |                            |                  | The battery level return value nn is believed to be from 0x40 to 0x6f.
//  |      |      |                            |                  | It changes to red at 0x4b and is green at or above 0x4c.
//  |      |      |                            |                  | It shows full at 0x63 and there is one red line (min) at 0x43 or below.
//  |      |      |                            |                  | The FTP editor did something strange at 0x3F, like an internal reset.
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
//  |  B7  |  1F  | 0x3c                       | SetSensitivity   | command:  B7 1F 3c, B7 3F xy    where x is the string and y is the level
//  |  B7  |  3F  | xy = string/level | echo   |                  | reply:    B7 1F 3c, B7 3F xy    and the controller echos the command
//  |      |      |                            |                  |
//  |      |      |                            |                  | this is sent to the controller to set the sensitivy level
//  |      |      |                            |                  | for a given string. It replies with an echo.
//  |      |      |                            |                  |
//  |      |      |                            |                  | my state machine looks for this and 0x3C from the host to maintain
//  |      |      |                            |                  | the array of sensetivities in memory for UI purposes
//  +------+------+----------------------------+------------------+------------------------------------------------------------------------
//  |      |  1F  |  0x3f = POLY/MONO MODE
//  |      |  3F  |  xx == 0=mono, 1=poly
//  +------+------+----------------------------+------------------+------------------------------------------------------------------------
//  |  B7  |  1F  | 0x42                       | GetSensitivity   | command:  B7 1F 42, B7 3F xx  = string 0..5
//  |  B7  |  3F  | xx = string | yy = level   |                  | reply:    B7 1F 42, B7 3F yy  = level, 0..14
//  |      |      |                            |                  |
//  |      |      |                            |                  | this is sent to the controller to ask it for the sensitivy level
//  |      |      |                            |                  | for a given string.
//  |      |      |                            |                  |
//  |      |      |                            |                  | my state machine looks for this and 0x42 from the host to maintain
//  |      |      |                            |                  | the array of sensetivities in memory for UI purposes
//  |      |      |                            |                  |
//  +------+------+----------------------------+------------------+------------------------------------------------------------------------
//  |      |  1F  |  0x4f = TOUCH_SENSITIVITY
//  |      |  3F  |  xx == 0..4 for pick and finger_style 5..9  (default 4)
//  +------+------+----------------------------+------------------+------------------------------------------------------------------------



// NOTE INFO MESSAGES
//
//      Following most NoteOn and NoteOff messages is a B7 1E xy
//      "NoteInfo" message, where x is the string number and y is a
//      compressed 0..15 velocity value (for the VU meter).
//
//      Although these appear to be primarly used to drive the FTP editor
//      sensitivity VU meter, they also appear to be neded to drive the FTP
//      'fretboard' display, as there is no other apparent way for the editor
//      to know which string was meant in a NOTE_ON message.
//
//      In my system, the lifetime of the "Note" (note_t object) is
//      bracketed by the 1E messages, and NOT by the note on and note off messages.
//      I cache the NOTE_ON and NOTE_OFF values, as a pair of uint8_s.
//
//      Thus I assume that these 1E messages refer to the most recent
//      NoteOn or NoteOff message.   There is a notion of the "most_recent"
//      note object, which is the most recently created one, which
//      comes into play in the Tuning messages.
//
// TUNING MESSAGES:
//
//      Following the NoteOn/1E message is usually a single B7 1D yy
//      and a series of B7 3D yy messages.  These are generally tuning
//      messages, where yy is the 0x40 biased signed number (from -0x40
//      to 0x40).
//
//      I call the 1D message a "SetTuning" message as it appears to
//      set the tunning relative to the most recent NoteOn/Off message,
//      whereas the 3D messages appear to be continuations, which I just
//      call "Tuning" messages.   It is not invariant that a NoteOn or
//      NoteOff is followed by a 1D ... the tuner will keep working on
//      a given string if multiple strings are picked.
//
//      Therefore I grab the "most recent" note upon a 1D and asusme that
//      is the note we are tuning.  If it goes away, the tuner is turned
//      off until the next 1D in the context of a new NoteOn/1E message


//-----------------------------------------------------------------------------
// POLY (MONO) MODE via patch change
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
// The CHECKSUM is simply the uint16_t sum of the bytes from bytes 1 thru 139,
// which does not include the opening F0 in the sysex message, or the ending
// checksum and endmidi(0xf7) ... spread into two 7 bit parts, in MSB order:
//
//    uint16_t checksum
//    uint8_t  checksum_byte[2];
//    checksum_byte[0] = (checksum >> 7) & 0x7f;
//    checksum_byte[1] = checksum & 0x7f;


typedef struct
    // Each data patch has an array of five of these substructures in it.
    // Unlike the UI, which puts the "pedal" split at the end, the "pedal"
    // is the 0th element in the array, and the splits 1-4 follow in slots
    // 1-4 in the array.
    //
    // I believe the controller split behavior comes into play if a note on a string within the split is struck,
    // but still dont really understand if or when the split pgm_change, banks, reverb and volumes
    // are sent by the controller.  I have only seen them sent in the context of the Windows editor.
    //
    // Also do not understand controller pedal behavior, if any.
    // Perhaps the pedal behavior overrides the split behavior when the pedal is pressed?
    //
    // The user manual says: "Hold / Loop. Send CC 66 value 127, 0 on release, for hold/loop"
    //
    // PS: I'm afraid of an editor software update, much less a controller firmware update!

{
	uint8_t pgm_change;     // 0..127
	uint8_t bank_lsb;       // 0..127
	uint8_t bank_msb;       // 0..127
        // it looks like these values are ONLY sent by the editor when you change patches.
        // they DO not seem to be sent when you change to the patch in "hardware mode"
        // same for "midi_volume" (which gets sent as CC 0x43 "Expression MSB") and
        // "midi_reverb" (which gets sent as CC 4a) "Reverb Level.
        // TODO: try sending program and bank changes to the controller in both modes

	uint8_t pitch_bend;     // 0=auto, 1=smooth, 2=stepped, 3=trigger
	uint8_t transpose;      // 24=none, plus or minus 24 1/2 steps (up or down 2 octaves)
	uint8_t midi_volume;    // 0=unchecked, 1..126, weirdness at 127
                            // goes to zero and sets split bit in main patch max_volume

	uint8_t dyn_sens;       // Dynamics Sensitivity 0x0A..0x14 (10..20) weird
	uint8_t dyn_offs;       // Dynamic Offset 0..20
	uint8_t midi_reverb;    // 0=unchecked, 1..126, weirdness at 127
                            // goes to zero and sets split bit in main patch max_reverb
} split_section_t;


typedef struct  // 142 byte "data" packet (subpatch)
    // This structure is stored in banks 0 and 1.  For each of these, there is
    // another structure patch_buffer1 (below), which I call the "name" patch, which is
    // just the header, the name length byte, the name, then  the checksum in a
    // similarly sized (142 byte) packet stored in banks 2 and 3.
    //
    // So the "data" for "Poly Program n" is in bank(0) patch(n-1) whereas it's "name"
    // is bank(2) patch(n-1) ... "Mono Program n" is in (1,n-1) and (3,n-1)
    //
    // It looks like maybe the controller's "current active patch" is banks0/2 patch 127
    // When you change patches in the editor, it sends the patch you are editing
    // to those numbers, and only upon a "save" does it write it to the "real" location.
{
	uint8_t header1[6];	                // F0 00 01 6E 01 (21=reply, or 41=set)
	uint8_t bank_num;                   // 0=hardware poly, 1=hardware mono
	uint8_t patch_num;                  // patch number within bank (0..127 .. only to 112 for mono bank?!?)

	uint8_t pedal_mode; 	            // HoldUp(2), HoldDown(3), Alternate(4), Loop(6), DontBlockNewNotes(1), BlockNewNotes(0)
        // The controller use (or not) of the "Pedal" "split" might determined by
        // the pedal_mode byte.
        //
        // 0 or 1 (DontBlock or Block) seem to indicate there is NO pedal split
        // whereas the other four values appear to indicate there IS a pedal split
        // still not clear on the semantics of WHAT a pedal split is ...
        //
        // it displays the extra set of pedal stuff, where, of course, poly_mode and touch_sens
        // are really part of the patch
        //
        // see note below on split_section_t split[5]

	uint8_t fret_range_low_up_12;       // the range of frets, from the lowest fret (open position) for hardware patches 1 & 2
	uint8_t fret_range_high_down_34;    // the range of frets from 1f down to this number, for hardware patches 3 and 4.
	uint8_t string_range_12; 		    // the strings covered by the red hardware patch1 (as opposed to yellow hardware patch 2)
	uint8_t string_range_34;   		    // the strings covered by the blue hardware patch3 (as opposed to green hardware patch 4)
	uint8_t azero0;                     // arpeggio mode which is "SEQUENTIAL" in all csv file examples

	uint8_t arp_speed;                  // arpeggio speed gleened from csv file
	uint8_t arp_length;	                // arp. length gleened from csv file (arpegio length?)
        // these arpeggio value do not appear to be used (they do not appear to affect the behavior of the controller)

	uint8_t touch_sens;                 // 0..4 "pick" 5-9 "fingerstyle"  Default(4)
	uint8_t poly_mode;                  // 0=mono, 1=poly  (Default 1 for bank0, 0 for bank1)

	split_section_t split[5];
        // The presence (or absence) of a "Hardware Synth Plugin" for a particular split
        // does NOT seem to be stored in the patch, as nothing in the patch changes when
        // I add or remove one (to an otherwise untouched split number).
        //
        // Which means the Editor must be caching it on the Windows machine (in
        // in User/name/AppData/Local/TriplePlay64/TriplePlay.settings).
        // Ir might heuristically derive it if the data on the controller changes
        // and it "loads" the patches from the controller .. or it might barf and
        // overwrite the patches on the controller with whatever the editor has.
        // Should be better tested with two seperate dongles and controllers ...

	uint8_t azero1;         // a zero .. I have never seen this byte change
	uint8_t seqbytes[64];   // 64 bytes from 0..0x3f .. I have never seen these bytes change
	uint8_t program[8];	    // the word "Program " with trailing space ... I have never seen these bytes change
	uint8_t azero2;         // a zero .. I have never seen this byte change

    // weird use of overflow bit from splits

	uint8_t max_volume;     // bitwise bits set if split midi_volume *would be* 127
                            // split1=0x08, split2=0x04, split3=0x02, split1=0x01, pedal=0x10
	uint8_t max_reverb;     // bitwise bits set if split midi_reverbe *would be* 127
                            // split1=0x08, split2=0x04, split3=0x02, split1=0x01, pedal=0x10

	uint8_t checksum[2];    // two byte checksum
	uint8_t endmidi;        // 0xf7

}   patch_buffer0_t;        // "data" patch in banks 0 and 1



typedef struct
{
	uint8_t header1[6];	    // F0 00 01 6E 01 (21=reply, or 41=set)
	uint8_t bank_num;       // 2 or 3 (2=hardware poly, 3=hardware mono)
	uint8_t patch_num;      // patch number within bank (0..127 .. only to 112 for mono bank?!?)
	uint8_t name_len; 	    // length of the name string
    uint8_t name[130];      // max name length
   	uint8_t checksum[2];    // two byte checksum
	uint8_t endmidi;        // 0xf7
}   patch_buffer1_t;        // "name" patches in banks 2 and 3



const uint8_t FTP_CODE_READ_PATCH = 0x01;	// get patch from controller (short message)
const uint8_t FTP_CODE_ACK = 0x11;			// ack from the controller?
const uint8_t FTP_CODE_WRITE_PATCH = 0x41;	// write patch to controller
const uint8_t FTP_CODE_PATCH_REPLY = 0x21;	// patch request reply from controller
const uint8_t FTP_CODE_PATCH_NAME  = 0x43;  // a 25 byte informative packet with the current patch name
    // implies the maximum length might be much less than the patch_buffer1_t can hold.

// const uint8_t FTP_CODE_UNKNOWN = 0x02;		// ? clear the patch ?
// const uint8_t FTP_CODE_ERROR1 = 0x12;		// ? error ?
// const uint8_t FTP_CODE_ERROR2 = 0x22;		// ? Error from controller ?

// uint8_t  ftpRequestPatch[]	= { 0xF0, 0x00, 0x01, 0x6E, 0x01, FTP_CODE_READ_PATCH, 0x00, 0x00, 0xf7 };
    // bytes 6 and 7 (0 based) are the bank, and patch, respectively
    // this example gets 0,0


// Fishman TriplePlay MIDI HOST Spoof Notes
//
// This version WORKS as a midi host to the FTP dongle, appears in
// windows as a "Fishman TriplePlay" with similarly named
// midi ports, and successfully runs within the Windows FTP Editor,
// based on an pref setting "SPOOF_FTP"
//
// REQUIRES setting MIDI4+SERIAL in Arduino IDE, as I did not want
// to muck around with Paul's midi.h file where it checks MIDI_NUM_CABLES
// inline, and IT is compiled with the original usb_desc.h, and it
// will not work properly as just a MIDI device (which uses SEREMU).
//
// Also note that the COM port changes from 3 to 11 when you change
// the SPOOF_FTP setting.
//
// As it stands right now, I am using a modified version of Paul's
// USBHost_t36.h file that exposes variables on it's MIDIDevice class,
// and makes a couple of functions (the usb IRQ handlers) virutal,
// so that I can inherit from it and implement the myMidiHost object,
// which tightly sends the hosted midi messges directly to the
// teensyDuino USBMidi, via the low level 'C' calls underlying
// it's hardwired "usbMIDI" class.
//
// HOST    myMidiHost : public USBHost_t36.h MIDIDevice
//
//      myMidiHost
//      Variable Name:  midi_host
//      Derives from USBHost_t36.h MIDIDevice
//      Which has been modified to expose protected vars
//         and make a method rx_data() virtual
//      Spoof requires setting MIDI4+SERIAL in Arduino IDE
//      Hooks rx_data(), which is the host usb IRQ handler, to
//           directly call the low level 'C' routines
//           usb_midi_write_packed(msg) and usb_midi_flush_output()
//           upon every received packet.
//
// DEVICE (teensyDuino "self") usbMidi
//      Variable Name: usbMIDI (hardwired)
//      available based on USB Setting in Arduino IDE
//      I get it's messages based on calls to low calls to
//         low levl usb_midi_read_message() 'C' function
//         in the critical_timer_handler() implementation
//      which is where they get written TO the hosted device (FTP)
//         via the exposed USBHost_t36 MIDIDevice myMidiHost
//         midi_host.write_packed(msg) method
//
// IT WAS IMPORTANT AND HARD TO FIND THAT I HAD TO LOWER THE PRIORITY
// OF THE critical_timer to let the host usb IRQs have priority.
//
// THE SYSTEM IS NOT SYMETTRIC.  We read from the host based on direct
// usb IRQ's, but we read from the device based on a timer loop and the
// usb_midi_read_message() function.
//
// The IRQ is enqueing the 32bit messages (and I also modified USBHost_t36.h
// to increase the midi rx buffer size from 80 to 2048), which are currently,
// and messily, then dequeud in the "critical_timer_handler()" method, then
// printed to buffered text, and finally displayed in the updateUI() method
// called from loop().   That whole thing could be cleaned up to work with
// a single queue of 32 bit words, and to decode and show the queued messages
// separately for display.




#endif // !_ftp_defs_h_
