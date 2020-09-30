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
#define QUICK_COL_MUTE           0
#define QUICK_COL_VOL_DOWN       1
#define QUICK_COL_VOL_UP         2
#define QUICK_CLIP_FIRST_ROW     4


#define QUICK_ROW_ERASE_TRACK    0



//--------------------
// SampleTank
//--------------------

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
	{11,		"SFLUTE+BASS",	"Psych Flute + 2 String Bass",	1},			// 11

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

int patchNewRig::guitar_effect_ccs[NUM_GUITAR_EFFECTS] = {
    GUITAR_DISTORTION_EFFECT_CC,
    GUITAR_WAH_EFFECT_CC,
    GUITAR_CHORUS_EFFECT_CC,
    GUITAR_ECHO_EFFECT_CC,
};



//====================================================================
// patchNewRig
//====================================================================



patchNewRig::patchNewRig()
{
	m_cur_bank_num = 0;
	m_cur_patch_num = -1;    // 0..15
	m_last_set_poly_mode = -1;

	resetDisplay();
	clearGuitarEffects();
	clearLooper();

    m_quick_mode = false;
}



void patchNewRig::resetDisplay()
{
	m_full_redraw = 1;
	m_last_displayed_poly_mode = -1;
	m_last_bank_num	= -1;
	m_last_patch_num = -1;

	for (int i=0; i<NUM_GUITAR_EFFECTS; i++)
	{
		m_last_guitar_state[i] = 0;
	}

	m_last_dub_mode = -1;
	for (int i=0; i<LOOPER_NUM_TRACKS; i++)
	{
		m_last_track_state[i] = -1;
	}

	m_last_quick_mode = -1;
	for (int i=0; i<TRACKS_TIMES_CLIPS; i++)
	{
        m_last_clip_mute[i] = -1;
        m_last_clip_vol[i] = -1;
	}

}



void patchNewRig::clearGuitarEffects()
{
	for (int i=0; i<NUM_GUITAR_EFFECTS; i++)
	{
		m_guitar_state[i] = 0;
		m_last_guitar_state[i] = -1;
	}
}


void patchNewRig::clearLooper()
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
	}

	for (int i=0; i<TRACKS_TIMES_CLIPS; i++)
	{
        m_clip_mute[i] = 0;
        m_last_clip_mute[i] = -1;
        m_clip_vol[i] = 100;	// magic init value
        m_last_clip_vol[i] = -1;
	}
}



// virtual
void patchNewRig::begin(bool warm)
{
    expWindow::begin(warm);

	thePedals.setLoopPedalRelativeVolumeMode(false);
		// 2020-09-22 - vestigial

	resetDisplay();

	if (m_quick_mode)
	{
		startQuickMode();
	}
	else	// normal button settings
	{
		// the upper right hand button is the bank select button.
		// it is blue for bank 0 and cyan for bank 1

		theButtons.setButtonType(THE_SYSTEM_BUTTON,	BUTTON_TYPE_CLICK | BUTTON_MASK_USER_DRAW, 0);

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

		for (int i=0; i<LOOPER_NUM_TRACKS; i++)
			theButtons.setButtonType(LOOP_FIRST_TRACK_BUTTON+i,BUTTON_EVENT_PRESS | BUTTON_MASK_USER_DRAW, 0);
		theButtons.setButtonType(LOOP_STOP_BUTTON,BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK  | BUTTON_MASK_USER_DRAW, 0);
		theButtons.setButtonType(LOOP_DUB_BUTTON,BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK | BUTTON_MASK_USER_DRAW, 0);

	    // showLEDs();

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

void patchNewRig::startQuickMode()
{
	resetDisplay();

	theButtons.clear();
   	theButtons.setButtonType(QUICK_MODE_BUTTON,	BUTTON_TYPE_CLICK, LED_ORANGE);
	theButtons.getButton(0,THE_SYSTEM_BUTTON)->m_event_mask |= BUTTON_EVENT_LONG_CLICK;

	for (int layer=0; layer<LOOPER_NUM_LAYERS; layer++)
	{
		int row = QUICK_CLIP_FIRST_ROW-layer;					// from bottom up
		int num = row * NUM_BUTTON_COLS;	// 0th element in row
		theButtons.setButtonType(num+QUICK_COL_MUTE,		BUTTON_EVENT_PRESS | BUTTON_MASK_USER_DRAW);
		theButtons.setButtonType(num+QUICK_COL_VOL_DOWN,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT, LED_RED);
		theButtons.setButtonType(num+QUICK_COL_VOL_UP,		BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT, LED_GREEN);
	}

	for (int c=0; c<LOOPER_NUM_TRACKS; c++)
	{
		int num = QUICK_ROW_ERASE_TRACK * NUM_BUTTON_COLS + c;
		theButtons.setButtonType(num, BUTTON_EVENT_CLICK, LED_RED);
	}
}


void patchNewRig::endQuickMode()
{
	m_quick_mode = false;		// may be called from updateUI
	begin(true);

	// restore the system button after begin() ...
	// normally done in expSystem::activatePatch)()

	theButtons.getButton(0,THE_SYSTEM_BUTTON)->m_event_mask |= BUTTON_EVENT_LONG_CLICK;
}


//------------------------------------
// Events
//------------------------------------
// rotary controllers

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
	else if (cc_num >= CLIP_MUTE_BASE_CC &&
			 cc_num < CLIP_MUTE_BASE_CC + 12)
	{
		int num = cc_num - CLIP_MUTE_BASE_CC;
		m_clip_mute[num] = value;
	}
}




//---------------------------------------------------------------------------------
// BUTTONS
//---------------------------------------------------------------------------------

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

	if (m_quick_mode)
	{
		if (row == QUICK_ROW_ERASE_TRACK)
		{
			display(0,"patchNewRig ERASE TRACK(%d)",col);
			sendSerialControlChange(LOOP_COMMAND_CC,LOOP_COMMAND_ERASE_TRACK_BASE+col,"ERASE_TRACK button click");
		}
		else if (row > QUICK_CLIP_FIRST_ROW-LOOPER_NUM_LAYERS)
		{
			int layer_num = 4-row;
			int clip_num = m_selected_track_num * LOOPER_NUM_LAYERS + layer_num;
			display(0,"quick_mode row(%d) col(%d)  layer=%d  clip_num=%d ",row,col,layer_num,clip_num);

			if (col == QUICK_COL_MUTE) 	// mute
			{
				int mute = m_clip_mute[clip_num];
				mute = mute ? 0 : 1;
				m_clip_mute[clip_num] = mute;
				sendSerialControlChange(CLIP_MUTE_BASE_CC+clip_num,mute,"MUTE_CLIP button click");
			}
			else if (col == QUICK_COL_VOL_DOWN || col == QUICK_COL_VOL_UP)	// volume up or down
			{
				int inc = col == QUICK_COL_VOL_DOWN ? -1 : 1;
				int val = m_clip_vol[clip_num];
				val += inc;
				if (val < 0) val = 0;
				if (val > 127) val = 127;
				m_clip_vol[clip_num] = val;

				// send it to the rPi ...
				// we'll update the display later ...

				sendSerialControlChange(CLIP_VOL_BASE_CC+clip_num,val,"CLIP_VOL button click");
			}

		}
	}

	// PATCH select

    else if (row < NUM_PATCH_ROWS &&
		     col < NUM_PATCH_COLS)
	{
		if (event == BUTTON_EVENT_CLICK)
		{
			// turn off the old patch button if any

			if (m_cur_patch_num != -1)
			{
				int button_num = patch_to_button(m_cur_patch_num);
				setLED(button_num,0);
			}

			m_cur_patch_num = bank_button_to_patch(m_cur_bank_num,num);	// my patch number
			m_last_patch_num = -1;

			// int bank_led_color = m_cur_bank_num == 0 ? LED_CYAN : LED_BLUE;
			// setLED(num,bank_led_color);


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

	// BANK SELECT

	else if (num == THE_SYSTEM_BUTTON &&		// bank select
		event == BUTTON_EVENT_CLICK)
	{
		m_cur_bank_num = (m_cur_bank_num + 1) % NUM_SYNTH_BANKS;
	}

	//	Guitar effects

    else if (num >= FIRST_EFFECT_BUTTON && num <= LAST_EFFECT_BUTTON)
    {
        if (event == BUTTON_EVENT_LONG_CLICK)           // turn off all effects on long click
        {
            clearGuitarEffects();
            for (int c=0; c<NUM_GUITAR_EFFECTS; c++)
            {
                mySendDeviceControlChange(
                    guitar_effect_ccs[c],
                    0x00,
                    GUITAR_EFFECTS_CHANNEL);
            }
        }
        else
        {
			m_guitar_state[col] = m_guitar_state[col] ? 0 : 1;
			m_last_guitar_state[col] = -1;

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



//---------------------------------------------------------------------------------
// Update UI
//---------------------------------------------------------------------------------
// Update UI sets both the LEDs, as well as the screen, for any changes in state


// virtual
void patchNewRig::updateUI()
{
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


	//----------------------------------
	// PEDALS
	//----------------------------------
	// draw the pedal frame

    if (redraw_all)
    {
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

	// and draw the pedal numbers if they've changed

    for (int i=0; i<NUM_PEDALS; i++)
    {
        expressionPedal *pedal = thePedals.getPedal(i);
        if (redraw_all || pedal->displayValueChanged())
        {
            pedal->clearDisplayValueChanged();
            int v = pedal->getValue();

			mylcd.setFont(Arial_40_Bold);   // Arial_40);
			mylcd.Set_Text_colour(TFT_WHITE);

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


	//-----------------------------
	// QUICK MODE
	//-----------------------------

	// if quick mode changed, clear the whole display area
	// title line is at 36, pedal names start at 230

	if (m_quick_mode != m_last_quick_mode)
	{
		m_last_quick_mode = m_quick_mode;
        mylcd.Fill_Rect(0,37,480,194,TFT_BLACK);
	}


	if (m_quick_mode)
	{
		for (int i=0; i<LOOPER_NUM_LAYERS; i++)
		{
			if (m_last_clip_mute[i] != m_clip_mute[i])
			{
				m_last_clip_mute[i] = m_clip_mute[i];
				setLED(QUICK_CLIP_FIRST_ROW-i,QUICK_COL_MUTE,m_clip_mute[i] ? LED_PURPLE : LED_CYAN);
				leds_changed = true;
			}

			if (m_last_clip_vol[i] != m_clip_vol[i])
			{
				m_last_clip_vol[i] = m_clip_vol[i];

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
					m_clip_vol[i]);

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
		// LOOPER BUTTONS

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

		// GUITAR BUTTONS

		for (int i=0; i<NUM_GUITAR_EFFECTS; i++)
		{
			if (m_last_guitar_state[i] != m_guitar_state[i])
			{
				m_last_guitar_state[i] = m_guitar_state[i];
				int color = m_guitar_state[i] ? LED_GREEN : 0;
				setLED(FIRST_EFFECT_BUTTON+i,color);
				leds_changed = true;
			}
		}

		// 'normal'	SCREEN STUFF

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

		// BANK LED

		if (m_last_bank_num != m_cur_bank_num)
		{
			m_last_bank_num = m_cur_bank_num;
			int bank_led_color = m_cur_bank_num == 0 ? LED_CYAN : LED_BLUE;
			setLED(THE_SYSTEM_BUTTON,bank_led_color);
			leds_changed = true;
		}


		// PATCH display SCREEN AND LED

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

	}	// Normal redraw (!m_quick_mode)

	if (leds_changed)
		showLEDs();

}
