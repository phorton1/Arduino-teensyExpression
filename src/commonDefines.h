// Defines that are common between rPi Looper and Arduino TeensyExpression projects.
// This file is currently denormalized and stored in both the Arduino-TeensyExpresiona
// project and in the circle-Looper projecte

#pragma once

#define LOOPER_NUM_TRACKS     4
#define LOOPER_NUM_LAYERS     4

#define TRACK_STATE_EMPTY               0x0000
#define TRACK_STATE_RECORDING           0x0001
#define TRACK_STATE_PLAYING             0x0002
#define TRACK_STATE_STOPPED             0x0004
#define TRACK_STATE_PENDING_RECORD      0x0008
#define TRACK_STATE_PENDING_PLAY        0x0010
#define TRACK_STATE_PENDING_STOP        0x0020
#define TRACK_STATE_PENDING			    (TRACK_STATE_PENDING_RECORD | TRACK_STATE_PENDING_PLAY | TRACK_STATE_PENDING_STOP)

#define LOOP_COMMAND_NONE               0x00
#define LOOP_COMMAND_CLEAR_ALL          0x01
#define LOOP_COMMAND_STOP_IMMEDIATE     0x02      // stop the looper immediately
#define LOOP_COMMAND_STOP               0x03      // stop at next cycle point
#define LOOP_COMMAND_DUB_MODE           0x04      // the dub mode is handled by rPi and modeled here
#define LOOP_COMMAND_ABORT_RECORDING    0x06      // abort the current recording if any
#define LOOP_COMMAND_LOOP_IMMEDIATE     0x08      // immediatly loop back to all clip starts ...
#define LOOP_COMMAND_SET_LOOP_START     0x09      // immediatly set the "restart point" for the clips in the track
#define LOOP_COMMAND_CLEAR_LOOP_START   0x0A      // immediatly set the "restart point" for the clips in the track
#define LOOP_COMMAND_TRACK_BASE         0x10      // the seven possible "track" buttons are 0x10..0x17
#define LOOP_COMMAND_ERASE_TRACK_BASE   0x20      // erase the given track (stops it if playing)

        // the above commands can be sent to the loop machine.
        // the following are for internal "pending" command use only
#define LOOP_COMMAND_RECORD             0x80
#define LOOP_COMMAND_PLAY               0x81


// Looper Serial CC numbers             // TE       rPi         descrip
#define LOOP_CONTROL_BASE_CC   0x65     // send     recv        for 0..LOOPER_NUM_CONTROLS the value is the volume control (Looper pedal == 0x67)
#define LOOP_STOP_CMD_STATE_CC 0x26		// recv     send        the value is 0, LOOP_COMMAND_STOP or STOP_IMMEDIATE
#define LOOP_DUB_STATE_CC      0x25		// recv     send        value is currently only the DUB state
#define LOOP_COMMAND_CC        0x24		// send     recv        the value is the LOOP command
#define TRACK_STATE_BASE_CC    0x14		// recv     send        for 0..3 tracks, value is track state
#define CLIP_VOL_BASE_CC       0x30		// send     recv        for 4 tracks * 3 clips - value is volume 0..127
#define CLIP_MUTE_BASE_CC      0x50		// both     both        for 4 tracks * 3 clips - value is mute state
#define NOTIFY_LOOP            0x64     // recv     send        value=number of pending loop notifies


// Looper Volume Controls
// these are based off of LOOP_CONTROL_BASE_CC

#define LOOPER_CONTROL_INPUT_GAIN      0
#define LOOPER_CONTROL_THRU_VOLUME     1
#define LOOPER_CONTROL_LOOP_VOLUME     2
#define LOOPER_CONTROL_MIX_VOLUME      3
#define LOOPER_CONTROL_OUTPUT_GAIN     4
#define LOOPER_NUM_CONTROLS            5
