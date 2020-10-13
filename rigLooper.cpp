#include "rigLooper.h"
#include <myDebug.h>
#include "oldRig_defs.h"
#include "myLeds.h"
#include "myTFT.h"
#include "pedals.h"
#include "buttons.h"
#include "midiQueue.h"
#include "ftp.h"
#include "ftp_defs.h"
#include "songMachine.h"
#include "songParser.h"
#include "winSelectSong.h"

// The difference between using Quantiloop and using the looper pedal can be detected using
// the setting on Pedal(1).  If the loop pedal is serial, we are working with the Looper.
// If not, we are working with Quantiloop.

#define dbg_patch_buttons    	1
#define dbg_serial_midi         1
#define dbg_rig					1
#define dbg_song_machine		0


#define SONG_MACHINE_BUTTON     9

#define FIRST_EFFECT_BUTTON  	15
#define LAST_EFFECT_BUTTON    	18

#define FIRST_LOOP_BUTTON   	19
#define LAST_LOOP_BUTTON    	24
#define LOOP_STOP_BUTTON		FIRST_LOOP_BUTTON
#define LOOP_FIRST_TRACK_BUTTON (FIRST_LOOP_BUTTON+1)
#define LOOP_DUB_BUTTON		    LAST_LOOP_BUTTON

#define TRACK_FLASH_MILLIS  150

// Quick mode is implemented as follows:
// The "bottom" row of buttons, red, are ERASE TRACK BUTTONS
// Clips "grow" from the 2nd to the bottom row upwards and
// have three buttons per column, MUTE(cyan/purple) DOWN(red) and UP(green)

#define QUICK_MODE_BUTTON        14
#define QUICK_ROW_MUTE           3
#define QUICK_ROW_VOL_DOWN       2
#define QUICK_ROW_VOL_UP         1
#define QUICK_ROW_ERASE_TRACK    0



//--------------------
// SampleTank
//--------------------

#define MULTI_OFFSET     16
#define NUM_PATCH_COLS   4
#define NUM_PATCH_ROWS   3
#define NUM_PATCHES_PER_BANK (NUM_PATCH_COLS * NUM_PATCH_ROWS)

// static
int rigLooper::patch_to_button(int patch_num)
{
	patch_num %= NUM_PATCHES_PER_BANK;
	int col = patch_num / NUM_PATCH_ROWS;
	int row = patch_num % NUM_PATCH_ROWS;
	row = 2 - row;
	int button = (row * NUM_BUTTON_COLS + col);
	display(dbg_patch_buttons,"patch_to_button(%d)=%d",patch_num,button);
	return button;
}

// static
int rigLooper::bank_button_to_patch(int bank, int button_num)
	// returns -1 if the button is not a patch button
{
	int patch = -1;
	int row = button_num / NUM_BUTTON_COLS;
	int col = button_num % NUM_BUTTON_COLS;
	if (row<NUM_PATCH_ROWS && col<NUM_PATCH_COLS)
	{
		row = 2 - row;
		patch = bank * RIGLOOPER_NUM_SYNTH_PATCHES + col * NUM_PATCH_ROWS + row;
	}

	display(dbg_patch_buttons,"bank_button_to_patch(%d,%d)=%d",bank,button_num,patch);
	return patch;

}

synthPatch_t rigLooper::synth_patch[RIGLOOPER_NUM_SYNTH_BANKS * RIGLOOPER_NUM_SYNTH_PATCHES] =
{
	// bank 0, starting with highest priorty bass sounds
	// The program number sent is MULTI_OFFSET + the program number from the left column

	{0,			"BASS1",		"MM Bass Finger",	 	0},					// 0
	{1,			"BASS2",		"Chorus Fretless",		0},					// 1
	{2,			"BASS-",		"2 String Bass",		1},					// 2

	{3,			"PIANO1",		"Mellow Grand 2",		0},					// 3
	{4,			"PIANO2",		"Classical Grand",		0},					// 4
	{5,			"EMPTY",		"Unassigned",			0},					// 5

	{6,			"ORGAN1",		"Ballad B Pad",			0},					// 6
	{7,			"ORGAN2",		"Drawbars Bow",			0},					// 7
	{8,			"BRASS",	    "Drama Brass",			0},					// 8

	{9,			"FLUTE1",		"Orch Flute",					0},			// 9
	{10,		"SFLUTE",		"Psych Flute",					0},			// 10
	{11,		"FL-BASS",	    "Psych Flute + Bass",			1},			// 11

	// bank 1, alternative sounds

	{12,		 "SPACE1",      "Mega Motion 3",		0},					// 12
	{13,		 "SPACE2",      "Mega Motion 4",		0},					// 13
	{14,		 "SPACE3",      "Whispering Pad",		0},					// 14

    {15,          "VOICES1",     "Vocal Oh",			0},					// 15
    {16,          "VOICES2",     "Vocal Ahh",			0},					// 16
    {17,          "FX",          "SFX Collection",		0},					// 17

    {18,          "EMPTY",     	"",			0},								// 18
    {19,          "EMPTY",     	"",			0},								// 19
    {20,          "EMPTY",     	"",			0},								// 20

    {21,          "EMPTY",     	"",			0},								// 21
    {22,          "EMPTY",     	"",			0},								// 22
    {23,          "EMPTY",     	"",			0},								// 23

};


//----------------
// toneStack
//----------------

int rigLooper::guitar_effect_ccs[RIGLOOPER_NUM_GUITAR_EFFECTS] = {
    GUITAR_DISTORTION_EFFECT_CC,
    GUITAR_WAH_EFFECT_CC,
    GUITAR_CHORUS_EFFECT_CC,
    GUITAR_ECHO_EFFECT_CC,
};

const char *rigLooper::guitar_effect_name[RIGLOOPER_NUM_GUITAR_EFFECTS] = {
	"DIST",
	"WAH ",
	"CHOR",
	"ECHO"
};


//--------------------
// quantiloop
//--------------------

int quantiloop_track_ccs[NUM_BUTTON_COLS] =
	// since they're not a rangeat this time
{
    QUANTILOOP_CC_TRACK1,
    QUANTILOOP_CC_TRACK2,
    QUANTILOOP_CC_TRACK3,
    QUANTILOOP_CC_TRACK4,
};




//====================================================================
// rigLooper
//====================================================================



rigLooper::rigLooper() :
	rigBase()
{
    m_quick_mode = false;
	m_quantiloop_mode = 0;
	m_cur_bank_num = 0;
	m_cur_patch_num = -1;    // 0..15
	m_last_set_poly_mode = -1;
	m_pending_open_song = 0;

	resetDisplay();
	clearGuitarEffects(true);
	clearLooper(true);
}



void rigLooper::resetDisplay()
{
	m_full_redraw = 1;
	m_last_displayed_poly_mode = -1;
	m_last_bank_num	= -1;
	m_last_patch_num = -1;
	m_last_song_state = -1;

	for (int i=0; i<RIGLOOPER_NUM_GUITAR_EFFECTS; i++)
	{
		m_last_guitar_state[i] = 0;
	}

	m_last_dub_mode = -1;
	for (int i=0; i<LOOPER_NUM_TRACKS; i++)
	{
		m_last_track_state[i] = -1;
		m_last_erase_state[i] = -1;
	}

	m_last_quick_mode = -1;
	for (int i=0; i<LOOPER_NUM_TRACKS_TIMES_LAYERS; i++)
	{
        m_last_clip_mute[i] = -1;
        m_last_clip_vol[i] = -1;
	}

	if (theSongMachine)
		theSongMachine->resetDisplay();
}




//============================================
// begin()
//============================================


// virtual
void rigLooper::begin(bool warm)
{
    expWindow::begin(warm);

	int pedal_mode = getPrefPedalMode(PEDAL_LOOP);
	m_quantiloop_mode = !(pedal_mode & PEDAL_MODE_SERIAL);
	thePedals.setLoopPedalRelativeVolumeMode(m_quantiloop_mode);

	if (!theSongMachine)
		new songMachine();
	theSongMachine->setBaseRig(this);

	resetDisplay();

	if (m_quick_mode)
	{
		startQuickMode();
	}
	else	// normal button settings
	{
		// the upper right hand button is the bank select button.
		// it is blue for bank 0 and cyan for bank 1

		theButtons.setButtonType(THE_SYSTEM_BUTTON,	BUTTON_TYPE_LONG_CLICK | BUTTON_TYPE_CLICK | BUTTON_MASK_USER_DRAW, 0);

		// 3rd from top right is the quick mode button

		theButtons.setButtonType(QUICK_MODE_BUTTON,	BUTTON_TYPE_CLICK, LED_ORANGE);

		// system boots up in bank 0 with no patch selected

		for (int r=0; r<NUM_PATCH_ROWS; r++)
		{
			for (int c=0; c<NUM_PATCH_COLS; c++)
			{
				theButtons.setButtonType(r * NUM_BUTTON_COLS + c, BUTTON_TYPE_CLICK | BUTTON_MASK_USER_DRAW, 0);
			}
		}

		// guitar effect buttons

		for (int i=FIRST_EFFECT_BUTTON; i<=LAST_EFFECT_BUTTON; i++)
			theButtons.setButtonType(i,	BUTTON_TYPE_CLICK, 0);
		theButtons.setButtonType(LAST_EFFECT_BUTTON, BUTTON_TYPE_CLICK | BUTTON_EVENT_LONG_CLICK | BUTTON_MASK_USER_DRAW, 0);

		// loop control buttons

		int ud = m_quantiloop_mode ? 0 : BUTTON_MASK_USER_DRAW;
		int color = m_quantiloop_mode ? LED_YELLOW : 0;

		for (int i=0; i<LOOPER_NUM_TRACKS; i++)
		{
			theButtons.setButtonType(LOOP_FIRST_TRACK_BUTTON+i,BUTTON_EVENT_PRESS | ud, color);
		}

		theButtons.setButtonType(LOOP_STOP_BUTTON,BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK | ud, color);
		theButtons.setButtonType(LOOP_DUB_BUTTON,BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK | ud, color);

		// songMachine button

		theButtons.setButtonType(SONG_MACHINE_BUTTON,BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK | BUTTON_MASK_USER_DRAW, 0);

	}	// normal (not quick mode) buttons and leds

	// If poly_mode was changed (in OldRig) reset it here

	if (m_last_set_poly_mode == -1 ||
		((bool)m_last_set_poly_mode) != ftp_poly_mode)
	{
		bool use_poly_mode = !synth_patch[m_cur_patch_num].mono_mode;
		m_last_set_poly_mode = use_poly_mode;
		sendFTPCommandAndValue(FTP_CMD_POLY_MODE,m_last_set_poly_mode);
	}
}



//------------------------------------
// quick mode
//------------------------------------

void rigLooper::startQuickMode()
{
	resetDisplay();

	theButtons.clear();
   	theButtons.setButtonType(QUICK_MODE_BUTTON,	BUTTON_TYPE_CLICK, LED_ORANGE);
	theButtons.setButtonType(THE_SYSTEM_BUTTON, BUTTON_TYPE_CLICK | BUTTON_TYPE_LONG_CLICK, LED_PURPLE);

	for (int layer=0; layer<LOOPER_NUM_LAYERS; layer++)
	{
		theButtons.setButtonType(layer + NUM_BUTTON_COLS * QUICK_ROW_MUTE,		BUTTON_EVENT_PRESS | BUTTON_MASK_USER_DRAW);
		theButtons.setButtonType(layer + NUM_BUTTON_COLS * QUICK_ROW_VOL_DOWN,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT, LED_ORANGE);
		theButtons.setButtonType(layer + NUM_BUTTON_COLS * QUICK_ROW_VOL_UP,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT, LED_GREEN);
	}

	for (int track=0; track<LOOPER_NUM_TRACKS; track++)
	{
		bool ud = m_quantiloop_mode ? 0 : BUTTON_MASK_USER_DRAW;
		int color = m_quantiloop_mode ? LED_RED : 0;
		theButtons.setButtonType(track + NUM_BUTTON_COLS * QUICK_ROW_ERASE_TRACK, BUTTON_EVENT_CLICK | ud, color);
	}

	showLEDs();
}


void rigLooper::endQuickMode()
{
	m_quick_mode = false;		// may be called from updateUI
	begin(true);
}




//--------------------------------------------------
// rigBase implementation to support songMachine
//--------------------------------------------------

// virtual
int rigLooper::findPatchByName(const char *patch_name)
{
	char buf1[80];
	strcpy(buf1,patch_name);
	songMachine::uc(buf1);

	for (int i=0; i<RIGLOOPER_NUM_SYNTH_BANKS * RIGLOOPER_NUM_SYNTH_PATCHES; i++)
	{
		char buf2[80];
		strcpy(buf2,synth_patch[i].short_name);
		songMachine::uc(buf2);

		if (!strcmp(buf1,buf2))
			return i;
	}
	return -1;
}


// virtual
void rigLooper::selectTrack(int num)
{
	display(dbg_song_machine,"rigLooper::selectTrack(%d)",num);
	m_selected_track_num = num;
	if (m_quantiloop_mode)
	{
		mySendDeviceControlChange(
			quantiloop_track_ccs[num],
			0x7f,
			QUANTILOOP_CHANNEL);
		mySendDeviceControlChange(
			quantiloop_track_ccs[num],
			0x00,
			QUANTILOOP_CHANNEL);
	}
	else
	{
		int value = m_selected_track_num + LOOP_COMMAND_TRACK_BASE;
		sendSerialControlChange(LOOP_COMMAND_CC,value,"rigLooper::selectTrack()");
	}
}

// virtual
void rigLooper::setClipMute(int layer_num, bool mute_on)
{
	display(dbg_song_machine,"rigLooper::setClipMute(%d,%d)",layer_num,mute_on);
	if (m_quantiloop_mode)
	{
		thePedals.setRelativeLoopVolume(layer_num,
			mute_on ? 0 : m_clip_vol[layer_num]);
		m_clip_mute[layer_num] = mute_on;
	}
	else if (m_selected_track_num >= 0)
	{
		int clip_num = m_selected_track_num * LOOPER_NUM_LAYERS + layer_num;
		sendSerialControlChange(CLIP_MUTE_BASE_CC+clip_num,mute_on,"rigLooper::setClipMute()");
		m_clip_mute[clip_num] = mute_on;
	}
	else
	{
		display(0,"WARNING: setClipMute(%d,%d) called with no selected track",layer_num,mute_on);
	}
}

// virtual
void rigLooper::setClipVolume(int layer_num, int val)
{
	display(dbg_rig,"rigLooper::setClipVolume(%d,%d)",layer_num,val);
	if (val < 0) val = 0;
	if (val > 127) val = 127;
	if (m_quantiloop_mode)
	{
		thePedals.setRelativeLoopVolume(layer_num,val);
		m_clip_vol[layer_num] = val;
	}
	else if (m_selected_track_num >= 0)
	{
		int clip_num = m_selected_track_num * LOOPER_NUM_LAYERS + layer_num;
		sendSerialControlChange(CLIP_VOL_BASE_CC+clip_num,val,"rigLooper::setClipVolume()");
		m_clip_vol[clip_num] = val;
	}
	else
	{
		display(0,"WARNING: setClipVolume(%d,%d) called with no selected track",layer_num,val);
	}
}


// virtual
void rigLooper::setPatchNumber(int patch_number)
{
	if (m_cur_patch_num != -1)
	{
		int button_num = patch_to_button(m_cur_patch_num);
		setLED(button_num,0);
	}

	m_cur_patch_num = patch_number;

	if (patch_number == -1)
	{
		m_cur_bank_num = 0;
	}
	else
	{
		m_cur_bank_num = patch_number / RIGLOOPER_NUM_SYNTH_PATCHES;
	}

	if (m_cur_patch_num != -1)
	{
		int prog_num = MULTI_OFFSET + synth_patch[m_cur_patch_num].prog_num;
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


// virtual
void rigLooper::setGuitarEffect(int effect_num, bool on)
{
	m_guitar_state[effect_num] = on;
	m_last_guitar_state[effect_num] = -1;

	int value = on ? 0x7f : 0;
	mySendDeviceControlChange(
		guitar_effect_ccs[effect_num],
		value,
		GUITAR_EFFECTS_CHANNEL);
}


// virtual
void rigLooper::clearGuitarEffects(bool display_only /* = false */)
{
	for (int i=0; i<RIGLOOPER_NUM_GUITAR_EFFECTS; i++)
	{
		m_guitar_state[i] = 0;
		m_last_guitar_state[i] = -1;
		if (!display_only)
		{
            mySendDeviceControlChange(
                guitar_effect_ccs[i],
                0x00,
                GUITAR_EFFECTS_CHANNEL);
		}
	}
}


// virtual
void rigLooper::clearLooper(bool display_only)
{
	m_dub_mode = 0;
	m_track_flash = 0;
	m_track_flash_time = 0;
	m_selected_track_num = -1;
    m_stop_button_cmd = 0;
    m_last_stop_button_cmd = -1;

	for (int i=0; i<LOOPER_NUM_TRACKS; i++)
	{
		m_track_state[i] = 0;
		m_last_track_state[i] = -1;
		m_last_erase_state[i] = -1;
	}

	for (int i=0; i<LOOPER_NUM_TRACKS_TIMES_LAYERS; i++)
	{
        m_clip_mute[i] = 0;
        m_last_clip_mute[i] = -1;
        m_clip_vol[i] = 100;	// magic init value
        m_last_clip_vol[i] = -1;
	}

	if (!display_only)
	{
		if (m_quantiloop_mode)
		{
			mySendDeviceControlChange(
				QUANTILOOP_CC_CLEAR_ALL,
				0x7f,
				QUANTILOOP_CHANNEL);
			mySendDeviceControlChange(
				QUANTILOOP_CC_CLEAR_ALL,
				0x00,
				QUANTILOOP_CHANNEL);
		}
		else
		{
			sendSerialControlChange(LOOP_COMMAND_CC,LOOP_COMMAND_CLEAR_ALL,"LOOP BUTTON long click");
		}
	}
}



//------------------------------------
// Events
//------------------------------------
// rotary controllers

// virtual
bool rigLooper::onRotaryEvent(int num, int val)
{
	display(dbg_rig,"rigLooper::onRotaryEvent(%d,%d)",num,val);

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

	if (!m_quantiloop_mode)
	{
		// prh - no rotary events for quantiloop - should be relative volumes
		sendSerialControlChange(control_num + RPI_CONTROL_NUM_CC_OFFSET,val,"rigLooper Rotary Control");
	}

	return true;
}




// virtual
void rigLooper::onSerialMidiEvent(int cc_num, int value)
{
	display(dbg_serial_midi+1,"rigLooper::SerialMidiEvent(0x%02x,0x%02x)",cc_num,value);

	// track state messages
	if (cc_num >= TRACK_STATE_BASE_CC && cc_num < TRACK_STATE_BASE_CC+4)
	{
		int track_num = cc_num - TRACK_STATE_BASE_CC;
		display(dbg_serial_midi,"rigLooper track_state[%d] = 0x%02x",track_num,value);
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
	else if (cc_num >= CLIP_MUTE_BASE_CC &&
			 cc_num < CLIP_MUTE_BASE_CC + 12)
	{
		int num = cc_num - CLIP_MUTE_BASE_CC;
		m_clip_mute[num] = value;
	}
	else if (cc_num == NOTIFY_LOOP)
	{
		theSongMachine->notifyLoop();
	}
}




//---------------------------------------------------------------------------------
// BUTTONS
//---------------------------------------------------------------------------------

// virtual
void rigLooper::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_COLS + col;
	int song_state = theSongMachine->getMachineState();

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

	if (m_quick_mode)
	{
		if (col<4)
		{
			int clip_num = m_quantiloop_mode ? col :
				m_selected_track_num * LOOPER_NUM_LAYERS + col;

			if (row == QUICK_ROW_ERASE_TRACK)
			{
				display(dbg_rig,"rigLooper ERASE TRACK(%d)",col);
				if (m_quantiloop_mode)
				{
					// send the cc command to erase quantiloop track %d
				}
				else
				{
					sendSerialControlChange(LOOP_COMMAND_CC,LOOP_COMMAND_ERASE_TRACK_BASE+col,"ERASE_TRACK button click");
				}
			}
			else if (row == QUICK_ROW_MUTE)
			{
				int mute = m_clip_mute[clip_num];
				mute = mute ? 0 : 1;
				setClipMute(col,mute);
			}
			else if (row == QUICK_ROW_VOL_DOWN || row == QUICK_ROW_VOL_UP)	// volume up or down
			{
				int inc = row == QUICK_ROW_VOL_DOWN ? -1 : 1;
				int val = m_clip_vol[clip_num];
				val += inc;
				setClipVolume(col,val);
			}
		}
	}

	// PATCH select

    else if (row < NUM_PATCH_ROWS &&
		     col < NUM_PATCH_COLS)
	{
		if (event == BUTTON_EVENT_CLICK)
		{
			int patch_num = bank_button_to_patch(m_cur_bank_num,num);	// my patch number
			display(0,"set_patch_num %d m_cur_bank_num=%d",patch_num,m_cur_bank_num);
			setPatchNumber(patch_num);
		}
	}

	// BANK SELECT

	else if (num == THE_SYSTEM_BUTTON &&		// bank select
		event == BUTTON_EVENT_CLICK)
	{
		m_cur_bank_num = (m_cur_bank_num + 1) % RIGLOOPER_NUM_SYNTH_BANKS;
		display(0,"m_cur_bank_num=%d",m_cur_bank_num);
	}

	//	Guitar effects

    else if (num >= FIRST_EFFECT_BUTTON && num <= LAST_EFFECT_BUTTON)
    {
        if (event == BUTTON_EVENT_LONG_CLICK)           // turn off all effects on long click
        {
            clearGuitarEffects(false);
        }
        else
        {
			bool set_state = m_guitar_state[col] ? 0 : 1;
			setGuitarEffect(col,set_state);
        }
    }

	// LOOPER

    else if (num >= FIRST_LOOP_BUTTON && num <= LAST_LOOP_BUTTON)
    {
		// Both the DUB and the STOP button allow for long click to
		// send LOOP_COMMAND_CLEAR_ALL and clear our model

		if (event == BUTTON_EVENT_LONG_CLICK)	// only button with long press
		{
			clearLooper(false);
			// also clear the song machine
			theSongMachine->setMachineState(SONG_STATE_EMPTY);
		}
		else if (event == BUTTON_EVENT_CLICK)
		{
			if (num == LOOP_STOP_BUTTON)
			{
				// in serial looper m_stop_button_cmd is set to NOTHING, STOP or STOP_IMMEDIATE
				// in quantiloop, the lower right button is always STOP_IMMEDIATE

				if (m_quantiloop_mode)
				{
					mySendDeviceControlChange(
						QUANTILOOP_CC_STOP_START_IMMEDIATE,
						0x7f,
						QUANTILOOP_CHANNEL);
					mySendDeviceControlChange(
						QUANTILOOP_CC_STOP_START_IMMEDIATE,
						0x00,
						QUANTILOOP_CHANNEL);
				}
				else if (m_stop_button_cmd)
				{
					sendSerialControlChange(LOOP_COMMAND_CC,m_stop_button_cmd,"LOOP STOP BUTTON click");
				}
				else
				{
					setLED(num,0);
					showLEDs();
				}
			}
			if (num == LOOP_DUB_BUTTON)
			{
				if (m_quantiloop_mode)
				{
					mySendDeviceControlChange(
						QUANTILOOP_CC_DUB_MODE,
						0x7f,
						QUANTILOOP_CHANNEL);
					mySendDeviceControlChange(
						QUANTILOOP_CC_DUB_MODE,
						0x00,
						QUANTILOOP_CHANNEL);
				}
				else
				{
					sendSerialControlChange(LOOP_COMMAND_CC,LOOP_COMMAND_DUB_MODE,"LOOP DUB BUTTON click");
					setLED(num,0);
					showLEDs();
				}
			}
		}

		// otherwise, the press is on one of the four "track" buttons
		// which *may* be redirected to the songMachine ...
		// if the machine is in non-zero state without FINISHED
		// or PAUSE the buttons go to the song machine ..

		else if (song_state &&
				 !(song_state & (SONG_STATE_PAUSED | SONG_STATE_FINISHED | SONG_STATE_ERROR)))
		{
			theSongMachine->notifyPress(num - LOOP_FIRST_TRACK_BUTTON + 1);
		}

		// otherwise they are serial TRACK1..n commands to the looper

		else
		{
			selectTrack(num - LOOP_FIRST_TRACK_BUTTON);
		}
	}

	// SONG MACHINE

	else if (num == SONG_MACHINE_BUTTON)
	{
		if (event == BUTTON_EVENT_LONG_CLICK)
		{
			expWindow *win = new winSelectSong(songParser::getTheSongName());
			theSystem.startModal(win);
		}

		// CLICK is only available if the machine is actually
		// running (the button is lit up, in which case we
		// toggle the PAUSED bit

		else
		{
			// in song state empty, the button sends out COMMAND_SET_LOOP_START

			setLED(num,0);
			showLEDs();

			if (!m_quantiloop_mode && !song_state)
			{
				sendSerialControlChange(LOOP_COMMAND_CC,LOOP_COMMAND_SET_LOOP_START,"temp song machine button");
			}

			// otherwise, it toggles song state if it's running

			else if (song_state && !(song_state & (SONG_STATE_FINISHED | SONG_STATE_ERROR)))
			{
				song_state ^= SONG_STATE_PAUSED;
				theSongMachine->setMachineState(song_state);

				for (int i=0; i<LOOPER_NUM_TRACKS; i++)
				{
					m_last_track_state[i] = -1;
				}
			}
		}
	}
}




//---------------------------------------------------------------------------------
// Update UI
//---------------------------------------------------------------------------------
// Update UI sets both the LEDs, as well as the screen, for any changes in state


// virtual
void rigLooper::updateUI()
{
	// other stuff

	bool leds_changed = false;
	bool redraw_all = m_full_redraw;
	m_full_redraw = false;

	// toggle the track flasher bit perpetually

    bool last_track_flash = m_track_flash;
    if (m_track_flash_time > TRACK_FLASH_MILLIS)
	{
		m_track_flash_time = 0;
		m_track_flash = !m_track_flash;
	}


	//-----------------------------
	// QUICK MODE
	//-----------------------------

	// if quick mode changed, clear the whole display area
	// title line is at 36, pedal names start at 230

	if (redraw_all || m_quick_mode != m_last_quick_mode)
	{
		m_last_quick_mode = m_quick_mode;
		fillRect(client_rect,TFT_BLACK);
	}


	if (m_quick_mode)
	{
		// "enable" the erase track buttons if there's a track state

		if (!m_quantiloop_mode)
		{
			for (int i=0; i<LOOPER_NUM_TRACKS; i++)
			{
				if (m_last_erase_state[i] != m_track_state[i])
				{
					m_last_erase_state[i] = m_track_state[i];
					int color = m_track_state[i] ? LED_RED : 0;
					setLED(QUICK_ROW_ERASE_TRACK,i,color);
					leds_changed = true;
				}
			}
		}

		for (int i=0; i<LOOPER_NUM_LAYERS; i++)
		{
			int clip_num = m_quantiloop_mode ? i :
				m_selected_track_num * LOOPER_NUM_LAYERS + i;

			if (m_last_clip_mute[clip_num] != m_clip_mute[clip_num])
			{
				m_last_clip_mute[clip_num] = m_clip_mute[clip_num];
				setLED(QUICK_ROW_MUTE,i, m_last_clip_mute[clip_num] ? LED_PURPLE : LED_CYAN);
				leds_changed = true;
			}

			if (m_last_clip_vol[clip_num] != m_clip_vol[clip_num])
			{
				m_last_clip_vol[clip_num] = m_clip_vol[clip_num];

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
					32,						// font height, more or less
					LCD_JUST_CENTER,
					TFT_YELLOW,
					TFT_BLACK,
					true,
					"%d",
					m_clip_vol[clip_num]);

				float bar_pct = ((float)m_clip_vol[i])/127.00;
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

	//-----------------------------------------------
	// "NORMAL" MODE
	//-----------------------------------------------
	// LED updating code interspersed

	else		// !m_quick_mode
	{
		mylcd.Set_Text_Back_colour(TFT_BLACK);

		// LOOPER BUTTONS

		if (!m_quantiloop_mode && m_last_stop_button_cmd != m_stop_button_cmd)
		{
			m_last_stop_button_cmd = m_stop_button_cmd;
			int color =
				m_stop_button_cmd == LOOP_COMMAND_STOP_IMMEDIATE ? LED_PURPLE :
				m_stop_button_cmd == LOOP_COMMAND_STOP ? LED_CYAN : 0;
			setLED(LOOP_STOP_BUTTON,color);
			leds_changed = true;
		}

		if (!m_quantiloop_mode && m_last_dub_mode != m_dub_mode)
		{
			m_last_dub_mode = m_dub_mode;
			setLED(LOOP_DUB_BUTTON,m_dub_mode ? LED_ORANGE : 0);
			leds_changed = true;
		}

		int song_state = theSongMachine->getMachineState();
		if (m_last_song_state != song_state)
		{
			m_last_song_state = song_state;
			int color =
				song_state & SONG_STATE_ERROR ? LED_RED :
				song_state & SONG_STATE_FINISHED ? 0 :
				song_state & SONG_STATE_PAUSED ?  LED_YELLOW:
				song_state & SONG_STATE_RUNNING ? LED_PURPLE : 0;
			setLED(SONG_MACHINE_BUTTON,color);
			leds_changed = true;
		}

		if (!m_quantiloop_mode && (!song_state ||
			(song_state & (SONG_STATE_PAUSED | SONG_STATE_FINISHED | SONG_STATE_ERROR))))
		{
			for (int i=0; i<LOOPER_NUM_TRACKS; i++)
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
		}

		// GUITAR BUTTONS

		for (int i=0; i<RIGLOOPER_NUM_GUITAR_EFFECTS; i++)
		{
			if (redraw_all || m_last_guitar_state[i] != m_guitar_state[i])
			{
				m_last_guitar_state[i] = m_guitar_state[i];
				int color = m_guitar_state[i] ? LED_GREEN : 0;
				setLED(FIRST_EFFECT_BUTTON+i,color);
				leds_changed = true;
			}
		}

		// BANK LED

		if (m_last_bank_num != m_cur_bank_num)
		{
			m_last_bank_num = m_cur_bank_num;
			int bank_led_color = m_cur_bank_num == 0 ? LED_CYAN : LED_BLUE;
			setLED(THE_SYSTEM_BUTTON,bank_led_color);
			leds_changed = true;
		}


		// PATCH display SCREEN AND LED

		bool redraw_patch = 0;
		if (m_cur_patch_num >= 0 &&
			(redraw_all || m_last_patch_num != m_cur_patch_num))
		{
			m_last_patch_num = m_cur_patch_num;

			// button

			int button_num = patch_to_button(m_cur_patch_num);
			int color = m_cur_patch_num >= NUM_PATCHES_PER_BANK ? LED_BLUE : LED_CYAN;
			setLED(button_num,color);
			leds_changed = true;

			// text

			redraw_patch = 1;
			if (!redraw_all)
				fillRect(synth_rect,TFT_BLACK);

			mylcd.setFont(Arial_32_Bold);
			mylcd.Set_Text_colour(TFT_CYAN);
			mylcd.Print_String(
				synth_patch[m_cur_patch_num].short_name,
				synth_rect.xs+5,
				synth_rect.ys+5);

			mylcd.setFont(Arial_18_Bold);
			mylcd.Set_Text_colour(TFT_MAGENTA);
			mylcd.Print_String(
				synth_patch[m_cur_patch_num].long_name,
				synth_rect.xs+5,
				synth_rect.ys+43);
		}

		// POLY MODE INDICATOR

		if (redraw_patch ||
			m_last_displayed_poly_mode != ftp_poly_mode)
		{
			m_last_displayed_poly_mode = ftp_poly_mode;
			mylcd.setFont(Arial_18_Bold);
			mylcd.printf_justified(
				synth_rect.xe - 60,
				synth_rect.ys + 10,
				50,
				20,
				LCD_JUST_LEFT,
				TFT_WHITE,
				TFT_BLACK,
				true,
				"%s",
				ftp_poly_mode ? "" : "MONO");
		}

	}	// Normal redraw (!m_quick_mode)

	// the song machine takes over the four bottom left buttons
	// and starts off as running if load() succeeds ...

	if (m_pending_open_song)
	{
		theSongMachine->load(m_pending_open_song);
		m_pending_open_song = 0;
	}

	if (theSongMachine)
		theSongMachine->updateUI();

	if (leds_changed)
		showLEDs();

}



//---------------------------
// songMachine hooks
//---------------------------

// virtual
void rigLooper::onEndModal(expWindow *win, uint32_t param)
{
	display(dbg_song_machine,"rigLooper::onEndModal(%08x,%08x)",(uint32_t)win,param);
	if (param)
	{
		display(dbg_song_machine,"SELECTED_NAME=%s",winSelectSong::selected_name);
		m_pending_open_song = winSelectSong::selected_name;
	}
	else
	{
		theSongMachine->setMachineState(SONG_STATE_EMPTY);
	}
}
