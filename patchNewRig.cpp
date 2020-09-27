#include "patchNewRig.h"
#include <myDebug.h>
#include "defines.h"
#include "myLeds.h"
#include "myTFT.h"
#include "pedals.h"
#include "buttons.h"
#include "midiQueue.h"
#include "ftp.h"
#include "ftp_defs.h"

// 2020-09-22 NEW newRig
//
// 		Designed to work with my looper box via serial data.
// 		Same synth and guitar buttons as before, mostly.
//
// 		- Got rid of separate guitar Chorus and Flanger buttons,
// 		  combining into "n" state "other effects" button. This
// 		  requires some changes to low level buttons (user drawn?)
//
// 		- moved guitar reverb (and "clear all") button one to the left,
// 		  to give one more button for the looper.
//
//  LOOPER BUTTONS
//										[ stop/clear all ]
//
//  	[track1]  [track2]  [track3]  [track4]   [ dub]
//
//  	A track can have a bunch of states ...
//
//           EMPTY (off)
//           RECORDING
//           PLAYING
//           RECORDED (stopped)
//           PENDING_RECORD
//           PENDING_PLAY
//           PENDING_STOP
//
// 		This is an odd place for the looper state machine discussion, but anyways.
//
// 		Clicking on a track makes it the "selected" track.  If the track is empty,
// 		the default command is "record", otherwise, it is "play".
//
// 		Recording the 0th clip in a track is special, as it's length has not been
// 		determined.   Pressing the same track's button again will toggle it to
// 		play (immeidately).  Pressing any other track's button will cause the recording
// 		to be saved and the other track to start playing or recording (immediately).
//
// 		Otherwise, all transitions between tracks take place at an integral loop
// 		of the base clip in the currently running (playing) track, and clicks have
// 		the effect of setting "pending" commands that will take place at that next
// 		loop point.
//
// 		One can "toggle" through the various "pending" commands in that window of
// 		time while the loop is coming around.
//
// LOOPER STATE MACHINE
//
// 		depending on (a) the state of the current track, and (b) whether or
// 		not a different track button was pressed, you can effect most desired
// 		behviors with the followings tate machine.
//
//      LOOPER STOPPED (or empty)
//
//           clicked track has content?      PLAY
//           clicked track has no content?   RECORD_BASE_CLIP
//
//      CURRENT TRACK BASE_CLIP recording?
//
//           save the recording, and then immediately:
//           same track clicked - start PLAYING
//           other track clicked - does it have content?
//                yes - start PLAYING immediately
//				  no - start RECORD_BASE_CLIP immediately
//
//      LOOPER PLAYING A TRACK
//
//           same track clicked:  toggles through following pending commands:
//                 STOP
//                 none   (effectively "continue playing")
//                 RECORD (if clip available)
//           different track clicked
//				   other track has content?
//                     yes:  PLAY
//                           RECORD (if clip available)
//                           none   (effectively "continue playing")
//            	   	   no:   the other track has no content, so the only option
//                           is RECORD/none
//
//      LOOPER RECORDING A (NON-BASE) Clip
//
//           same track clicked:  toggles through following pending commands:
//                 PLAY
//                 STOP
//                 none   (effectively "continue recording")
//                 RECORD (if clip available)
//           different track clicked
//				   other track has content?
//                     yes:  PLAY
//                           RECORD (if clip available)
//                           none   (effectively "continue playing")
//            	   	   no:   the other track has no content, so the only option
//                           is RECORD/none
//
// 		One thing that is NOT possible with the above state machine is to add
// 		new recorded clips (layers) "immediately" while recording a base clip, because
// 		a click on the same track will just start playing ... and likewise you cannot
// 		just jump from recording a base-clip to recording a second clip on another
// 		track, because (according to the above state machine) that click would
// 		start it playing.
//
// 		Hence the "DUB" button.
//
// DUB BUTTON
//
// 		It is not a "DUB" mode.
//
// 		It is a one-shot "shift" key, that says, "on the next track I click, if
// 		it can be recorded, start recording it, instead of playing it".
//
// 		The DUB key is cleared after any press of a track button.
//
// 		It changes the default order, if possible to RECORD first ...
// 		And is especially important for immedate use.
//
// 		The teensyExpression does not "model" the state of the looper.
// 		But it handles the "one time" aspect of the "DUB" key by sending
// 		an extra bit with the track number button.
//
// STOP BUTTON
//
// 		That just leaves the "stop/clear_all" button.
//
// 		The other thing you cannot do with the initial state machine above
// 		is just record a single (base) clip and NOT play it afterwards.
// 		On a secondary clip you *could* toggle through to the stop command,
// 		but on the base clip, there is no way to toggle that.
//
// 		Therefore ...
//
// 		The "stop" button is programmed in this TE to send the "stop".
// 		command (period) which is handled by the looper to sets the
// 		current track to "pending stop" (or stops it immediately if
// 		it's recording a base clip).
//
// 		If there is already a "pending stop" (i.e. the button is pressed
// 		twice) the Looper senses that and stops immediately.  If there
// 		was a track recording, it is NOT saved.
//
// 		So, two clicks will always stop immediately ... one click will
// 		stop immediately while recording a base clip, and one click
// 		will stop at the end of the next loop if the looper is playing.
//
// 		Note that in the case of one click, you *could* still use the
// 		track key to toggle that track to a different mode while in
// 		that window of time while the loop is playing ...
//
// CLEAR_ALL (long press "stop" button)
//
// 		A long press of the "stop/clear_all" key issues the CLEAR_ALL command,
// 		which resets the looper.
//
// QUICK MODE
//
// 		The "quick mode" will present (at least) an array of 4x3 "track
// 		mute buttons, which display, and change, the state of the mute
// 		for individual clips.


//----------------------------
// From rPi Looper.h:
//----------------------------

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
#define LOOP_COMMAND_DUB_MODE           0x08      // the dub mode is handled by rPi and modeled here
#define LOOP_COMMAND_TRACK_BASE         0x10      // the seven possible "track" buttons are 0x10..0x17

#define LOOP_STOP_CMD_STATE_CC 0x26		// TE recv: the value is 0, LOOP_COMMAND_STOP or STOP_IMMEDIATE
#define LOOP_DUB_STATE_CC      0x25		// TE recv: the value is currently only the DUB state
#define LOOP_COMMAND_CC        0x24		// TE send: the value is the LOOP command
#define TRACK_STATE_BASE_CC    0x14		// TE recv: value is track state
#define CLIP_VOL_BASE_CC       0x30		// TE send ONLY: value is volume 0..127 x 12 clips
#define CLIP_MUTE_BASE_CC      0x40		// TE send and recv: value is mute state x 12 clips


//----------------------------
// local defines
//----------------------------

#define GROUP_SYNTH		1
#define GROUP_GUITAR	2

#define IPAD_PROG_CHANGE_BUTTON  9

#define QUICK_MODE_BUTTON        14
#define QUICK_MODE_TIMEOUT       3000

#define FIRST_EFFECT_BUTTON  	15
#define LAST_EFFECT_BUTTON    	18

#define FIRST_LOOP_BUTTON   	19
#define LAST_LOOP_BUTTON    	24
#define LOOP_STOP_BUTTON		FIRST_LOOP_BUTTON
#define LOOP_FIRST_TRACK_BUTTON (FIRST_LOOP_BUTTON+1)
#define LOOP_DUB_BUTTON		    LAST_LOOP_BUTTON

#define TRACK_FLASH_MILLIS  150


//--------------------
// SampleTank
//--------------------
// PatchNewRig has a different layout than the old rig and
// "knows" that there ar two banks of 12 patches

// Complicated, not sure how to proceed
// It feels as if I want to have BASS + various other spacy sounds especially

// So now I am thinking of NOT using SampleTank patches 0..15 for these,
// allowing oldRig to remain unchanged, as it were, in it's "in-between" state,
// and using SampleTank patches 16..39 for a completely seperate mapping.

// The BUTTONS are ordered from the 3rd row, left column, by columns,
// So patches 16,17, and 18, are the highest priority bass sounds, and
// the mapping of buttons to patches handles the row/col/offset issues

#define MULTI_OFFSET     16
#define NUM_PATCH_COLS   4
#define NUM_PATCH_ROWS   3
#define NUM_PATCHES_PER_BANK (NUM_PATCH_COLS * NUM_PATCH_ROWS)


// static
int patchNewRig::patch_to_button(int patch_num)
{
	patch_num %= NUM_PATCHES_PER_BANK;
	int col = patch_num / NUM_PATCH_ROWS;
	int row = patch_num % NUM_PATCH_ROWS;
	row = 2 - row;
	int button = (row * NUM_BUTTON_COLS + col);
	display(0,"patch_to_button(%d)=%d",patch_num,button);
	return button;
}


// static
int patchNewRig::bank_button_to_patch(int bank, int button_num)
	// returns -1 if the button is not a patch button
{
	int patch = -1;
	int row = button_num / NUM_BUTTON_COLS;
	int col = button_num % NUM_BUTTON_COLS;
	if (row<NUM_PATCH_ROWS && col<NUM_PATCH_COLS)
	{
		row = 2 - row;
		patch = bank * NUM_SYNTH_PATCHES + col * NUM_PATCH_ROWS + row;
	}

	display(0,"bank_button_to_patch(%d,%d)=%d",bank,button_num,patch);
	return patch;

}


synthPatch_t patchNewRig::synth_patch[NUM_SYNTH_BANKS * NUM_SYNTH_PATCHES] = {

	// bank 0, starting with highest priorty bass sounds
	// INDEX IS BY POSITION (first parameter, included for clarity, is ignored)

	{0,			"BASS1",		"MM Bass Finger",	 	0},
	{1,			"BASS2",		"Chorus Fretless",		0},
	{2,			"BASS-",		"2 String Bass",		1},

	{3,			"PIANO1",		"Mellow Grand 2",		0},
	{4,			"PIANO2",		"Classical Grand",		0},
	{5,			"EMPTY",		"Unassigned",			0},

	{6,			"ORGAN1",		"Ballad B Pad",			0},
	{7,			"ORGAN2",		"Drawbars Bow",			0},
	{8,			"BRASS",	    "Drama Brass",			0},

	{9,			"FLUTE1",		"Orch Flute",					0},
	{10,		"SFLUTE",		"Psych Flute",					0},
	{11,		"SFLUTE+BASS",	"Psych Flute + 2 String Bass",	1},

	// bank 1, alternative sounds

	{12,		 "SPACE1",      "Mega Motion 3",		0},
	{13,		 "SPACE2",      "Mega Motion 4",		0},
	{14,		 "SPACE3",      "Whispering Pad",		0},

    {15,          "VOICES1",     "Vocal Oh",			0},
    {16,          "VOICES2",     "Vocal Ahh",			0},
    {17,          "FX",          "SFX Collection",		0},

    {18,          "EMPTY",     	"",			0},
    {19,          "EMPTY",     	"",			0},
    {20,          "EMPTY",     	"",			0},

    {21,          "EMPTY",     	"",			0},
    {22,          "EMPTY",     	"",			0},
    {23,          "EMPTY",     	"",			0},

};



//----------------
// toneStack
//----------------

int patchNewRig::guitar_effect_ccs[NUM_BUTTON_COLS] = {
    GUITAR_DISTORTION_EFFECT_CC,
    GUITAR_WAH_EFFECT_CC,
    GUITAR_FLANGER_EFFECT_CC,
    GUITAR_CHORUS_EFFECT_CC,
    GUITAR_ECHO_EFFECT_CC,
};



//====================================================================
// patchNewRig
//====================================================================

patchNewRig::patchNewRig()
{
    m_quick_mode = false;
	m_cur_bank_num = 0;
	m_cur_patch_num = -1;    // 0..14
	m_last_set_poly_mode = -1;
	m_last_displayed_poly_mode = -1;
	clearLooper();

	for (int i=0; i<NUM_BUTTON_ROWS * NUM_BUTTON_COLS; i++)
	{
		m_event_state[i] = 0;
	}
	for (int i=0; i<4; i++)
		m_last_relative_vol[i] = -1;
}


void patchNewRig::clearLooper()
{
	m_dub_mode = false;
	m_last_dub_mode = true;
	m_track_flash = 0;
	m_track_flash_time = 0;
	m_selected_track_num = -1;
    m_stop_button_cmd = 0;
    m_last_stop_button_cmd = -1;

	for (int i=0; i<4; i++)
	{
		m_track_state[i] = 0;
		m_last_track_state[i] = -1;
	}
}


// virtual
void patchNewRig::end()
{
	// save off the button states
	for (int i=0; i<NUM_BUTTON_ROWS * NUM_BUTTON_COLS; i++)
	{
		m_event_state[i] = theButtons.getButton(i)->m_event_state &
		 (BUTTON_STATE_SELECTED | BUTTON_STATE_TOUCHED);
	}
}


// virtual
void patchNewRig::begin(bool warm)
{
    expWindow::begin(warm);

	thePedals.setLoopPedalRelativeVolumeMode(false);
		// 2020-09-22 - vestigial

	m_last_bank_num = -1;
	m_last_patch_num = -1;
	m_last_displayed_poly_mode = -1;
    m_full_redraw = 1;

	// the upper right hand button is the bank select button.
	// it is blue for bank 0 and cyan for bank 1

   	theButtons.setButtonType(THE_SYSTEM_BUTTON,	BUTTON_TYPE_TOGGLE, LED_CYAN, LED_BLUE);

	// 2nd from top right is the ipad program change button

   	theButtons.setButtonType(IPAD_PROG_CHANGE_BUTTON,BUTTON_EVENT_CLICK, LED_GREEN);

	// 3rd from top right is the quick mode button

   	theButtons.setButtonType(QUICK_MODE_BUTTON,	BUTTON_TYPE_CLICK, LED_ORANGE);

	// system boots up in bank 0 with no patch selected

	int bank_led_color = m_cur_bank_num == 0 ? LED_CYAN : LED_BLUE;
    for (int r=0; r<NUM_PATCH_ROWS; r++)
	{
		for (int c=0; c<NUM_PATCH_COLS; c++)
		{
			theButtons.setButtonType(r * NUM_BUTTON_COLS + c, BUTTON_TYPE_RADIO(GROUP_SYNTH), LED_NONE, bank_led_color);
		}
	}

	// guitar effect buttons

    for (int i=FIRST_EFFECT_BUTTON; i<=LAST_EFFECT_BUTTON; i++)
        theButtons.setButtonType(i,	BUTTON_TYPE_TOGGLE | BUTTON_GROUP(GROUP_GUITAR), 0, LED_GREEN);
    theButtons.setButtonType(LAST_EFFECT_BUTTON, BUTTON_TYPE_TOGGLE | BUTTON_GROUP(GROUP_GUITAR) | BUTTON_EVENT_LONG_CLICK, 0, LED_GREEN);

	// loop control buttons

    for (int i=0; i<4; i++)
        theButtons.setButtonType(LOOP_FIRST_TRACK_BUTTON+i,BUTTON_EVENT_PRESS | BUTTON_MASK_USER_DRAW, 0, 0, 0);
    theButtons.setButtonType(LOOP_STOP_BUTTON,BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK  | BUTTON_MASK_USER_DRAW, 0);
    theButtons.setButtonType(LOOP_DUB_BUTTON,BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK | BUTTON_MASK_USER_DRAW, 0);

	// set the (possibly saved) button states into the button array

	for (int i=0; i<NUM_BUTTON_ROWS * NUM_BUTTON_COLS; i++)
		theButtons.setEventState(i,m_event_state[i]);

	// kludge ... manually reset the current patch to it's current color

	if (m_cur_patch_num > 0)
	{
		int led_num = patch_to_button(m_cur_patch_num);
		int color = m_cur_patch_num >= NUM_PATCHES_PER_BANK ?
			LED_BLUE : LED_CYAN;
		setLED(led_num,color);

		if (m_last_set_poly_mode == -1 ||
			((bool)m_last_set_poly_mode) != ftp_poly_mode)
		{
			bool use_poly_mode = !synth_patch[m_cur_patch_num].mono_mode;
			m_last_set_poly_mode = use_poly_mode;
			sendFTPCommandAndValue(FTP_CMD_POLY_MODE,m_last_set_poly_mode);
		}
	}

    showLEDs();
}




void patchNewRig::startQuickMode()
{
	end();	// save off the button state
	for (int c=0; c<4; c++)
	{
		theButtons.setButtonType(c,						BUTTON_TYPE_CLICK, LED_PURPLE);
		theButtons.setButtonType(c + NUM_BUTTON_COLS,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT, LED_GREEN);
		theButtons.setButtonType(c + NUM_BUTTON_COLS*2,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT, LED_RED);
	}

	showLEDs();
	m_quick_mode_time = 0;
}


void patchNewRig::endQuickMode()
{
	m_quick_mode = false;		// may be called from updateUI
	begin(true);
	// restore the system button after begin() ...
	// normally done in expSystem::activatePatch)()
	theButtons.getButton(0,THE_SYSTEM_BUTTON)->m_event_mask |= BUTTON_EVENT_LONG_CLICK;
	for (int i=0; i<4; i++)
		m_last_relative_vol[i] = -1;
	for (int i=0; i<4; i++)
		m_last_track_state[i] = -1;
}



// virtual
void patchNewRig::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_COLS + col;

	// QUICK MODE

	if (num == QUICK_MODE_BUTTON &&
		event == BUTTON_EVENT_CLICK)
	{
		m_quick_mode = !m_quick_mode;
		if (!m_quick_mode)
			endQuickMode();
		else
			startQuickMode();
	}
	else
	{
		// timer is restarted even if you press loop or guitar buttons
		m_quick_mode_time = 0;
	}

	// PATCH select OR quick mode loop volumes

    if (row < NUM_PATCH_ROWS &&
		col < NUM_PATCH_COLS)
	{
		if (m_quick_mode)
		{
			// currently un-programmed
		}
		else if (event == BUTTON_EVENT_CLICK)
		{
			m_cur_patch_num = bank_button_to_patch(m_cur_bank_num,num);	// my patch number
			int prog_num = MULTI_OFFSET + m_cur_patch_num;
			mySendDeviceProgramChange(prog_num, SYNTH_PROGRAM_CHANNEL);

			// send the mono/poly ftp mode command if needed

			bool use_poly_mode = !synth_patch[m_cur_patch_num].mono_mode;
			if (((bool)m_last_set_poly_mode) != use_poly_mode)
			{
				m_last_set_poly_mode = use_poly_mode;
				sendFTPCommandAndValue(FTP_CMD_POLY_MODE,m_last_set_poly_mode);
			}
		}
	}

	// BANK SELECT

	else if (num == THE_SYSTEM_BUTTON &&		// bank select
		event == BUTTON_EVENT_CLICK)
	{
		m_cur_bank_num = (m_cur_bank_num + 1) % NUM_SYNTH_BANKS;
		// weird - the current patch should stay at the old color, but
		// NEXT time it should change to the new color

		int bank_led_color = m_cur_bank_num == 0 ? LED_CYAN : LED_BLUE;
		for (int r=0; r<NUM_PATCH_ROWS; r++)
		{
			for (int c=0; c<NUM_PATCH_COLS; c++)
			{
				theButtons.setButtonType(r * NUM_BUTTON_COLS + c, BUTTON_TYPE_RADIO(GROUP_SYNTH), LED_NONE, bank_led_color);
			}
		}

		// kludge ... manually reset the current patch to it's current color

		if (m_cur_patch_num > 0)
		{
			int led_num = patch_to_button(m_cur_patch_num);
			int color = m_cur_patch_num >= NUM_PATCHES_PER_BANK ?
				LED_BLUE : LED_CYAN;
			setLED(led_num,color);
		}

		showLEDs();
	}

	// IPAD PROGRAM SELECT

	else if (num == IPAD_PROG_CHANGE_BUTTON)
	{
		// if (event == BUTTON_EVENT_CLICK)
		{
			static int prog_num = 2;		// zero based  0..3 = audiobus, tonestack, sampletank, quantiloop
				// the note we send out is prog_num + 1
				// setup so first button press takes us to Quantiloop
			prog_num = (prog_num + 1) % 4;
			mySendMidiMessage(0x09, NEW_SELECT_RIG_CHANNEL, prog_num + 1, 0x7f);
			mySendMidiMessage(0x08, NEW_SELECT_RIG_CHANNEL, prog_num + 1, 0);
				// send a NOTE_ON message (with 7f velocity) to channel 9 with the
				// note 1==Audiobus, 2=Tonestack, 3=SampleTank, 4=Quantiloop, followed
				// by a NOTE_OFF
		}
	}

	//	Guitar effects

    else if (num >= FIRST_EFFECT_BUTTON && num <= LAST_EFFECT_BUTTON)
    {
        if (event == BUTTON_EVENT_LONG_CLICK)           // turn off all effects on long click
        {
            theButtons.clearRadioGroup(GROUP_GUITAR);
            for (int c=0; c<NUM_BUTTON_COLS; c++)
            {
                mySendDeviceControlChange(
                    guitar_effect_ccs[c],
                    0x00,
                    GUITAR_EFFECTS_CHANNEL);
            }
        }
        else
        {
			arrayedButton *pb = theButtons.getButton(row,col);
			int value = pb->isSelected() ? 0x7f : 0;
            mySendDeviceControlChange(
                guitar_effect_ccs[col],
                value,
                GUITAR_EFFECTS_CHANNEL);
        }
    }

	// LOOPER

    else if (num >= FIRST_LOOP_BUTTON && num <= LAST_LOOP_BUTTON)
    {
		// Both the DUB and the STOP button allow for long click to
		// send LOOP_COMMAND_CLEAR_ALL and clear our model

		if (event == BUTTON_EVENT_LONG_CLICK)	// only button with long press
		{
			clearLooper();
			sendSerialControlChange(LOOP_COMMAND_CC,LOOP_COMMAND_CLEAR_ALL,"LOOP BUTTON long click");
		}
		else if (event == BUTTON_EVENT_CLICK)
		{
			if (num == LOOP_STOP_BUTTON)
			{
				if (m_stop_button_cmd)
					sendSerialControlChange(LOOP_COMMAND_CC,m_stop_button_cmd,"LOOP STOP BUTTON click");
			}
			if (num == LOOP_DUB_BUTTON)
			{
				sendSerialControlChange(LOOP_COMMAND_CC,LOOP_COMMAND_DUB_MODE,"LOOP DUB BUTTON click");
			}
		}

		// otherwise, it is presumed that those button presses are track buttons

		else
		{
			m_selected_track_num = num - LOOP_FIRST_TRACK_BUTTON;
			int value = m_selected_track_num + LOOP_COMMAND_TRACK_BASE;
			sendSerialControlChange(LOOP_COMMAND_CC,value,"LOOP TRACK BUTTON");
		}
	}
}




// loopMachine commands

// virtual
void patchNewRig::onSerialMidiEvent(int cc_num, int value)
{
	// track state messages
	if (cc_num >= TRACK_STATE_BASE_CC && cc_num < TRACK_STATE_BASE_CC+4)
	{
		int track_num = cc_num - TRACK_STATE_BASE_CC;
		display(0,"patchNewRig track_state[%d] = 0x%02x",track_num,value);
		m_track_state[track_num] = value;
	}
	else if (cc_num == LOOP_DUB_STATE_CC)
	{
		m_dub_mode = value;
	}
	else if (cc_num == LOOP_STOP_CMD_STATE_CC)
	{
		m_stop_button_cmd = value;
	}
}




// virtual
void patchNewRig::updateUI()
{
	if (m_quick_mode && m_quick_mode_time > QUICK_MODE_TIMEOUT)
	{
		endQuickMode();
	}

	// handle looper button LED track state changes ..

	// toggle a flasher bit always
    bool last_track_flash = m_track_flash;
    if (m_track_flash_time > TRACK_FLASH_MILLIS)
	{
		m_track_flash_time = 0;
		m_track_flash = !m_track_flash;
	}

	if (!m_quick_mode)
	{
		bool leds_changed = false;

		if (m_last_stop_button_cmd != m_stop_button_cmd)
		{
			m_last_stop_button_cmd = m_stop_button_cmd;
			int color =
				m_stop_button_cmd == LOOP_COMMAND_STOP_IMMEDIATE ? LED_PURPLE :
				m_stop_button_cmd == LOOP_COMMAND_STOP ? LED_CYAN : 0;
			setLED(LOOP_STOP_BUTTON,color);
			leds_changed = true;
		}

		if (m_last_dub_mode != m_dub_mode)
		{
			m_last_dub_mode = m_dub_mode;
			setLED(LOOP_DUB_BUTTON,m_dub_mode ? LED_ORANGE : 0);
			leds_changed = true;
		}

		for (int i=0; i<4; i++)
		{
			int state = m_track_state[i] ;
			if ((state != m_last_track_state[i]) ||
				((last_track_flash != m_track_flash) && (state & TRACK_STATE_PENDING)))
			{
				m_last_track_state[i] = state;

				int color = 0;			// EMPTY
				if (state & (TRACK_STATE_RECORDING | TRACK_STATE_PENDING_RECORD))
					color = LED_RED;
				else if (state & (TRACK_STATE_PLAYING | TRACK_STATE_PENDING_PLAY))
					color = LED_YELLOW;
				else if (state & TRACK_STATE_PENDING_STOP)
					color = LED_CYAN;
				else if (state & TRACK_STATE_STOPPED)
					color = LED_GREEN;

				if ((state & TRACK_STATE_PENDING) && !m_track_flash)
					color = 0;

				setLED(LOOP_FIRST_TRACK_BUTTON+i,color);
				leds_changed = true;
			}
		}
		if (leds_changed)
			showLEDs();
	}

	// regular redraw screen stuff

    bool draw_full = false;
    if (m_full_redraw)
    {
        draw_full = true;
        m_full_redraw = false;
        mylcd.Fill_Rect(0,230,480,30,TFT_YELLOW);
        mylcd.setFont(Arial_18_Bold);   // Arial_16);
        mylcd.Set_Text_colour(0);
        mylcd.Set_Draw_color(TFT_YELLOW);
        for (int i=0; i<NUM_PEDALS; i++)
        {
            mylcd.printf_justified(
                i*120,
                235,
                120,
                30,
                LCD_JUST_CENTER,
                TFT_BLACK,
                TFT_YELLOW,
				false,
                "%s",
                thePedals.getPedal(i)->getName());

            if (i && i<NUM_PEDALS)
                mylcd.Draw_Line(i*120,260,i*120,mylcd.Get_Display_Height()-1);
        }
    }

	if (m_last_displayed_poly_mode != ftp_poly_mode)
	{
		m_last_displayed_poly_mode = ftp_poly_mode;
        mylcd.setFont(Arial_12_Bold);
		mylcd.printf_justified(
			10,
			40,
			50,
			20,
			LCD_JUST_CENTER,
			TFT_YELLOW,
			TFT_BLACK,
			true,
			"%s",
			ftp_poly_mode ? "" : "MONO");
	}


    bool font_set = false;
    for (int i=0; i<4; i++)
    {
        expressionPedal *pedal = thePedals.getPedal(i);
        if (draw_full || pedal->displayValueChanged())
        {
            pedal->clearDisplayValueChanged();
            int v = pedal->getValue();

			// shows the frequency of UI vs MIDI messages on pedals
			// display(0,"updateUI pedal(%d) changed to %d",i,v);

            if (!font_set)
            {
                mylcd.setFont(Arial_40_Bold);   // Arial_40);
                mylcd.Set_Text_colour(TFT_WHITE);
                font_set = 1;
            }

            mylcd.printf_justified(
                12+i*120,
                260+14,
                100,
                45,
                LCD_JUST_CENTER,
                TFT_WHITE,
                TFT_BLACK,
				true,
                "%d",
                v);
        }
    }

	// if quick mode changed, clear the whole display area
	// title line is at 36, pedal names start at 230

	if (m_quick_mode != m_last_quick_mode)
	{
		m_last_quick_mode = m_quick_mode;
        mylcd.Fill_Rect(0,37,480,194,TFT_BLACK);
	}

	// if in quick mode and a volume changed raw bar and value

	if (m_quick_mode)
	{
		for (int i=0; i<4; i++)
		{
			int cur_relative_volume = 99;
			if (m_last_relative_vol[i] != cur_relative_volume)
			{
				m_last_relative_vol[i] = cur_relative_volume;


				// each region is 80 wide with 20 on the outside and 40 between
				// the region starts at y==40 to allow 4 pixels from bar
				// text is 20 pixels high.  bars go from 229 upto 65

				#define REL_VOL_BAR_WIDTH  80
				#define REL_VOL_LEFT_SPACE 20
				#define REL_VOL_IN_BETWEEN_SPACE 40
				#define REL_VOL_TEXT_Y     40
				#define REL_VOL_BAR_START  65
				#define REL_VOL_BAR_END    229
				#define REL_VOL_BAR_HEIGHT (REL_VOL_BAR_END-REL_VOL_BAR_START)

				int region_left = REL_VOL_LEFT_SPACE + i * (REL_VOL_BAR_WIDTH + REL_VOL_IN_BETWEEN_SPACE);

		        mylcd.setFont(Arial_20_Bold);
				mylcd.printf_justified(
					region_left,
					REL_VOL_TEXT_Y,
					REL_VOL_BAR_WIDTH,
					24,						// font height, more or less
					LCD_JUST_CENTER,
					TFT_YELLOW,
					TFT_BLACK,
					true,
					"%d",
					cur_relative_volume);


				float bar_pct = ((float)cur_relative_volume)/127.00;
				float bar_height = bar_pct * REL_VOL_BAR_HEIGHT;
				int bar_h = bar_height;
				int black_h = REL_VOL_BAR_HEIGHT - bar_h;

				// draw the black portion

				mylcd.Fill_Rect(
					region_left,
					REL_VOL_BAR_START,
					REL_VOL_BAR_WIDTH,
					black_h,
					TFT_BLACK);

				// draw the green portion

				mylcd.Fill_Rect(
					region_left,
					REL_VOL_BAR_START + black_h,
					REL_VOL_BAR_WIDTH,
					bar_h,
					TFT_DARKGREEN);

			}
		}
	}


    if (!m_quick_mode &&
		 m_cur_patch_num >= 0 &&
        (draw_full ||
         m_last_patch_num != m_cur_patch_num))
    {
        m_last_patch_num = m_cur_patch_num;
        mylcd.setFont(Arial_40_Bold);   // Arial_40);
        int y = 90;
        mylcd.printf_justified(
            0,y,mylcd.Get_Display_Width(),mylcd.getFontHeight(),
            LCD_JUST_CENTER,
            TFT_CYAN,
            TFT_BLACK,
			true,
            "%s",
            synth_patch[m_cur_patch_num].short_name);

        y += mylcd.getFontHeight();
        mylcd.setFont(Arial_24);   // Arial_40);
        mylcd.printf_justified(
            0,y,mylcd.Get_Display_Width(),mylcd.getFontHeight(),
            LCD_JUST_CENTER,
            TFT_MAGENTA,
            TFT_BLACK,
			true,
            "%s",
            synth_patch[m_cur_patch_num].long_name);
    }

}




//-------------------------------------
// rotary controllers
//-------------------------------------

// virtual
bool patchNewRig::onRotaryEvent(int num, int val)
{
	display(0,"patchNewRig::onRotaryEvent(%d,%d)",num,val);

	// send the value out on CC's 101 thru 105
	// mapped from rotary controls
	//   0==input volume      1==output volume
	//   2==thru volume       3==mix volume
	// to rPi control numbers + 101

	#define RPI_CONTROL_INPUT_GAIN          0
	#define RPI_CONTROL_THRU_VOLUME         1
	#define RPI_CONTROL_LOOP_VOLUME         2
	#define RPI_CONTROL_MIX_VOLUME          3
	#define RPI_CONTROL_OUTPUT_GAIN         4

	#define RPI_CONTROL_NUM_CC_OFFSET  0x65
		// so the loop volume pedal will be at 0x65 + 2

	int control_num =
		num == 0 ? RPI_CONTROL_INPUT_GAIN :
		num == 1 ? RPI_CONTROL_OUTPUT_GAIN :
		num == 2 ? RPI_CONTROL_THRU_VOLUME :
		RPI_CONTROL_MIX_VOLUME;

	sendSerialControlChange(control_num + RPI_CONTROL_NUM_CC_OFFSET,val,"patchNewRig Rotary Control");
	return true;
}
