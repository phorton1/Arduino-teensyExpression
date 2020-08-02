#ifndef __oldRig_defs_h_
#define __oldRig_defs_h_

// prh 2020-08-02
//
// Got rid of the notion of the "new rig" working by effecting synth,
// guitar, and loop volumes via CC's to AudioBus's mixer.  I did not
// like the exponential curves presented by AudioBus's mixer.
//
// I NEED to make some progress and implement:
//
//     - relative volumes for Quantiloop tracks
//     - utilization of FTP polymode vis-a-vis SampleTank patches
//       which also requires, at a minimum, sending synth volume
//       CC's for channels 1 thru 6 when in polymode.
//
// and would LIKE to:
//
//     - "wrap up" using teensyEpression with the "old rig",
//     - clean up the "public" fusion presentation
//     - make an initial set of readme's for the source
//     - remove unused myDebug libraries from public presentation
//     - create thingeeverse page
//     - do at least one decent recording on youtube, with emotion
//     - do at least one youtube technical presentation video
//     - create hackaday page
//
// and:
//
//     - create a public presentation of progress on the foot pedal
//     - create hackaday, thingeeverse, and maybe youtube pages for that
//     - get back to the rPi looper, implement *at least* pre-sampling
//       and cross fading of tracks so as to
//     - update the vGuitar project (and or create "looper" hackaday project),
//       so that I can ...
//     - do an initial implementation of the serial teensyExpression --> rPi tech
//     - so that I create a youtube video with an initial demo of that and
//     - record at least one more decent recording and/or youtube videoo
//       using the rpi Looper
//
// NEW THINGS
//
//     FTP polymode patches
//     polymode aware synth volume
//        possibly "latching" pedal controls
//     relative volumes for Quantiloop tracks
//     change programs on iPad
//     re-bury FTP tuner and sensitivity in settings so
//        single press upper right button is patch (rig) specific
//     sequential vs. parallel modes in quantiloop
//     tempo experiments?
//
// OTHER IDEAS
//
//     midi event recorder / sequencer
//
// TEMPO still requires:
//
//       audiobus has ST, TS, and QL all routed to output
//           “Settings – Sync Settings – Ableton Link” enabled
//       quantiloop
//           “Force Link Tempo” ON for both Old and New rigs
//           Enable “Ableton Link”
//           Sync To: Audiobus




//----------------------------
// AudioBus
//----------------------------

#define IPAD_RIG_NONE  -1
#define IPAD_RIG_OLD   0
#define IPAD_RIG_NEW   1
extern int g_ipad_is_new_rig;
    // defaults to -1
    // 0 == old rig
    // 1 == new rig


#define NEW_SELECT_RIG_CHANNEL    9
    // Rig selection on the iPad is orchestrated by sending a simple 0xCn patch
    // change message on channel 9 (0xC8) where "patch 0" is the old rig,
    // and "patch 1" is the new rig
    //
    // AudioBus, ToneStack, and Quantiloop are all setup to accept patch
    // changes on channel 9, and fortunately do not change out of the range
    // of defined presets, and do not seem to respond badly to setting the patch
    // to the one it's already at.  So as currently configured, once we have
    // established/ "new rig" by sending the "patch 1" change, we can then
    // subsequently switch between QL "Parellel" and "Serial" modes by sending
    // "patch 1" and "patch 2" messages, respectively, on channel 9.

#define NEW_PATCH_NUM_OLD_RIG  0
#define NEW_PATCH_NUM_NEW_RIG  1
#define NEW_PATCH_NUM_QUANTILOOP_PARALLEL  NEW_PATCH_NUM_NEW_RIG
#define NEW_PATCH_NUM_QUANTILOOP_SERIAL  2

#define AUDIO_BUS_CONTROL_CHANNEL       9       // also 9
#define NEW_AUDIOBUS_CC_GUITAR_VOLUME   50
#define NEW_AUDIOBUS_CC_SYNTH_VOLUME    51
#define NEW_AUDIOBUS_CC_LOOP_VOLUME     52

#define NEW_AUDIOBUS_NOTE_SHOW_AUDIOBUS     1
#define NEW_AUDIOBUS_NOTE_SHOW_TONESTACK    2
#define NEW_AUDIOBUS_NOTE_SHOW_SAMPLETANK   3
#define NEW_AUDIOBUS_NOTE_SHOW_QUANTILOOP   4

#define NEW_AUDIOBUS_CC_TEMPO  49



//----------------
// synth
//----------------

#define SYNTH_DEFAULT_VOICE_VOLUME   (0.7 * 127)


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
    // prh - in new rig this is dependent on the FTP poly mode setting
    // and may be split into "groups" based on fake "split" ftp settings
#define SYNTH_VOLUME_CC             7
    // this remains the per-channel sample tank "volume" control

// #define NEW_SYNTH_MASTER_VOLUME_CC   nn
// For time being, all sample tank master volumes are set to the
// default of 1.0 (full on), so I don't have to edit each single
// everytime I add a new one to a multi.



typedef struct
    // structure common to New and Old rig patches
{
    int prog_num;
    const char *short_name;
    const char *long_name;
}   synthPatch_t;



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
    // not used in new rig

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
    // not used in new rig
    // Midi Command: CC     (not Note On/Off)
    // Type: Continous   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Momentary Action
    // 4 separate control assignments in Quantiloop
    // Action: Track Volume: Track 1
    // Action: Track Volume: Track 2
    // Action: Track Volume: Track 3
    // Action: Track Volume: Track 4

// previously unused

#define NEW_LOOP_DUB_MODE       17
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Dub Mode
#define UNUSED_LOOP_TAP_TEMPO      88
    // NOTE CONFLICT WITH WAH_EFFECT_CC, also on ch9
    // Midi Command: CC     (not Note On/Off)
    // Type: Momentary Action   Up/MinValue: 0   Down/MaxValue:127
    //       not Latching, Continuous
    // Action: Tap Tempo

// new rig

#define NEW_LOOP_VOLUME_TRACK1    75
#define NEW_LOOP_VOLUME_TRACK2    76
#define NEW_LOOP_VOLUME_TRACK3    77
#define NEW_LOOP_VOLUME_TRACK4    78


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
