#ifndef __oldRig_defs_h_
#define __oldRig_defs_h_



//----------------
// synth
//----------------

#define SYNTH_PROGRAM_CHANNEL       7       // 1 based
    // program change come on specific channels
    
#define SYNTH_PATCH_PIANO1          0       // Mellow Grand 2
#define SYNTH_PATCH_ORGAN1          1       // Ballad B Pad
#define SYNTH_PATCH_BRASS1          2       // Drama Brass
#define SYNTH_PATCH_STRINGS1        3       // String Orch 1
#define SYNTH_PATCH_BASS1           4       // MM Bass Finger + Jazz Kit on ch10
#define SYNTH_PATCH_FLUTE1          5       // Orch Flute
#define SYNTH_PATCH_VOICES1         6       // Vocal Oh
#define SYNTH_PATCH_FX1             7       // Mega Motion 4

#define SYNTH_PATCH_EPIANO          8       // PIANO2 - Smooth FM Piano
#define SYNTH_PATCH_ORGAN2          9       // Drawbars Bow
#define SYNTH_PATCH_SPACE1          10      // BRASS2 = Mega Motion 3
#define SYNTH_PATCH_SPACE3          11      // STRINGS2 = Whispering Pad
#define SYNTH_PATCH_BASS2           12      // Chorus Fretless
#define SYNTH_PATCH_FLUTE2          13      // Psych Flute
#define SYNTH_PATCH_BASS_MINUS      14      // P Bass Finger (should be MM Bass Finger)
#define SYNTH_PATCH_FX2             15      // SFX Collection

#define SYNTH_VOLUME_CHANNEL        1
#define SYNTH_VOLUME_CC             7


//----------------
// toneStack
//----------------

#define GUITAR_EFFECTS_CHANNEL  9   // one based

#define GUITAR_WAH_CONTROL_CC              11
#define GUITAR_REVERB_DEPTH_CC             20

#define GUITAR_DISTORTION_EFFECT_CC        26
#define GUITAR_WAH_EFFECT_CC               27
#define GUITAR_FLANGER_EFFECT_CC           28
#define GUITAR_CHORUS_EFFECT_CC            29
#define GUITAR_ECHO_EFFECT_CC              30

#define GUITAR_VOLUME_CHANNEL               7       // one based
#define GUITAR_VOLUME_CC                    7


//----------------
// Quantiloop
//----------------

#define LOOP_CONTROL_CHANNEL        9   // one based

#define LOOP_CONTROL_TRACK1         21
#define LOOP_CONTROL_TRACK2         22
#define LOOP_CONTROL_TRACK3         23
#define LOOP_CONTROL_TRACK4         31      // ADDED!!
    // There are three of these buttons assigned to tracks 1-3
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Rec-Play-Dub TrackN
    // Hold Action: Undo/Red
    // Double Tap Action: Stop Track
#define LOOP_CONTROL_CLEAR_ALL      24
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Clear All
#define LOOP_STOP_START_IMMEDIATE   25
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Stop/Start Immediate
    // Hold Action: Clear All / End Song
    // Double Tap Action: Clear All / End Song
    
#define LOOP_VOLUME_CC        7
    // Midi Command: CC     (not Note On/Off)
    // Type: Continous   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Momentary Action
    // 4 separate control assignments in Quantiloop
    // Action: Track Volume: Track 1
    // Action: Track Volume: Track 2
    // Action: Track Volume: Track 3
    // Action: Track Volume: Track 4
#define LOOP_DUB_MODE       17
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Dub Mode
#define LOOP_TAP_TEMPO      88
    // NOTE CONFLICT WITH WAH_EFFECT_CC, also on ch9
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Tap Tempo


//--------------------------
// NOTES
//--------------------------
// MPD218
//
//      Buttons 0-15 (from bottom left) are Channel 7 "PROGRAM messages 0..15
//      The unused knobs are assigned t0 Channel 1 CCs: 3,9   12,13   14,15
//
// SOFTSTEP
//
//      The guitar pedals are programmed on the Softstep as
//           Data Source: "FootOn"  (Gain 1.00 offset 0)
//           Table: Toggle (Min:0, Max 127, Smooth 0)
//           Messasge Type: CC (26 thru 30) Channel 9
//           With "LED" Settings
//              Key Name: DIST, WAH, PHAS, CHOR, DLY  (display mode: Always)
//              Green Led: True, Red Led: None (on the assigned layer)
        
//      The Quantiloop Buttons are setup on the Softstep as 
//           Data Source: "FootOn"  (Gain 1.00 offset 0)
//           Table: Linear (Min:0, Max 127, Smooth 0)
//           Messasge Type: CC (21 thru 25) Channel 9, 
//           With "LED" Settings
//              Key Name: DIST, WAH, PHAS, CHOR, DLY (display mode: Always)
//              Green Led: None, Red True: None (on the assigned layer)
//      
//      The weird programming for the Arrow Button is
//      Line1
//           Data Source: "Pedal"  (Gain 1.18 offset 0)
//           Table: Linear (Min:0, Max 127, Smooth 0)
//           Messasge Type: CC (channel 11, 21 thru 25)
//      Line2 - controls Reverb Depth
//           Data Source: "NavY Inc/Dec"  (Gain 1.00 offset 0)
//           Table: Linear (Min:0, Max 127, Smooth 0)
//           Messasge Type: CC 20 Channel 9
//           With "LED" Settings
//              Prefix: V   Display Mode: Immed Para   Key Name: Exp
//              Key Name: DIST, WAH, PHAS, CHOR, DLY (display mode: Always)
//              Green Led: None, Red True: None (on the assigned layer)

#endif  // !__oldRig_defs_h_

