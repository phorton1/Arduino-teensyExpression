#include "rigOld.h"
#include <myDebug.h>
#include "defines.h"
#include "myLeds.h"
#include "myTFT.h"
#include "pedals.h"
#include "buttons.h"
#include "midiQueue.h"
#include "ftp.h"
#include "ftp_defs.h"

// Dont know if old rig is better of I should be using new rig
// this define, to become a pref or whatever, switches the behavior
// of the loop buttons to send out midi messages over the serial port.

#define SEND_LOOPER_BUTTONS_VIA_SERIAL3   0
	// 2020-09-22 This is turned off here, but became the default
	// in NewRig ....

// It would be better to divorce the button states from the patch state
// for the implementation of quick mode

#define GROUP_LOOPER 	7
#define GROUP_SYNTH		1
#define GROUP_GUITAR	2

#define IPAD_PROG_CHANGE_BUTTON  9
#define QUICK_MODE_BUTTON        14
#define QUICK_MODE_TIMEOUT       3000

#define FIRST_EFFECT_BUTTON  	15
#define LAST_EFFECT_BUTTON    	19
#define FIRST_LOOP_BUTTON   	20
#define LAST_LOOP_BUTTON    	24

#define BUTTON_TYPE_LOOPER      (BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE | BUTTON_MASK_TOUCH | BUTTON_MASK_RADIO | BUTTON_GROUP(GROUP_LOOPER) )
#define BUTTON_TYPE_LOOP_CLEAR  (BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK | BUTTON_MASK_TOUCH | BUTTON_MASK_RADIO | BUTTON_GROUP(GROUP_LOOPER) )


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
int rigOld::patch_to_button(int patch_num)
{
	patch_num %= NUM_PATCHES_PER_BANK;
	int col = patch_num / NUM_PATCH_ROWS;
	int row = patch_num % NUM_PATCH_ROWS;
	row = 2 - row;
	int button = (row * NUM_BUTTON_COLS + col);
	display(0,"patch_to_button(%d)=%d",patch_num,button);
	return button;
}


// stati
int rigOld::bank_button_to_patch(int bank, int button_num)
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


synthPatch_t rigOld::synth_patch[NUM_SYNTH_BANKS * NUM_SYNTH_PATCHES] = {

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

int rigOld::guitar_effect_ccs[NUM_BUTTON_COLS] = {
    GUITAR_DISTORTION_EFFECT_CC,
    GUITAR_WAH_EFFECT_CC,
    GUITAR_FLANGER_EFFECT_CC,
    GUITAR_CHORUS_EFFECT_CC,
    GUITAR_ECHO_EFFECT_CC,
};


//----------------
// Quantiloop
//----------------

int rigOld::loop_ccs[NUM_BUTTON_COLS] =
{
    LOOP_CONTROL_TRACK1,
    LOOP_CONTROL_TRACK2,
    LOOP_CONTROL_TRACK3,
    LOOP_CONTROL_TRACK4,
    LOOP_STOP_START_IMMEDIATE,
};







//====================================================================
// rigOld
//====================================================================

rigOld::rigOld() :
	expWindow(WIN_FLAG_SHOW_PEDALS)
{
    m_quick_mode = false;
	m_cur_bank_num = 0;
	m_cur_patch_num = -1;    // 0..14
	m_last_set_poly_mode = -1;
	m_last_displayed_poly_mode = -1;

	for (int i=0; i<NUM_BUTTON_ROWS * NUM_BUTTON_COLS; i++)
	{
		m_event_state[i] = 0;
	}

    for (int i=0; i<4; i++)
		m_last_relative_vol[i] = -1;
}



// virtual
void rigOld::end()
{
	// save off the button states
	for (int i=0; i<NUM_BUTTON_ROWS * NUM_BUTTON_COLS; i++)
	{
		m_event_state[i] = theButtons.getButton(i)->m_event_state &
		 (BUTTON_STATE_SELECTED | BUTTON_STATE_TOUCHED);
	}
}


// virtual
void rigOld::begin(bool warm)
{
    expWindow::begin(warm);

	thePedals.setLoopPedalRelativeVolumeMode(true);

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


    for (int i=FIRST_EFFECT_BUTTON; i<=LAST_EFFECT_BUTTON; i++)
        theButtons.setButtonType(i,	BUTTON_TYPE_TOGGLE | BUTTON_GROUP(GROUP_GUITAR), 0, LED_GREEN);
    theButtons.setButtonType(LAST_EFFECT_BUTTON, BUTTON_TYPE_TOGGLE | BUTTON_GROUP(GROUP_GUITAR) | BUTTON_EVENT_LONG_CLICK, 0, LED_GREEN);

    for (int i=FIRST_LOOP_BUTTON; i<=LAST_LOOP_BUTTON; i++)
        theButtons.setButtonType(i,BUTTON_TYPE_LOOPER, 0, LED_RED, LED_YELLOW);
    theButtons.setButtonType(LAST_LOOP_BUTTON,BUTTON_TYPE_LOOP_CLEAR, 0, LED_RED, LED_YELLOW);

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




void rigOld::startQuickMode()
{
	end();	// save off the button state
	for (int c=0; c<4; c++)
	{
		theButtons.setButtonType(c + NUM_BUTTON_COLS,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT, LED_GREEN);
		theButtons.setButtonType(c + NUM_BUTTON_COLS*2,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT, LED_RED);
	}

	showLEDs();
	m_quick_mode_time = 0;
}


void rigOld::endQuickMode()
{
	m_quick_mode = false;		// may be called from updateUI
	begin(true);
	// restore the system button after begin() ...
	// normally done in expSystem::activateRig)()
	theButtons.getButton(0,THE_SYSTEM_BUTTON)->m_event_mask |= BUTTON_EVENT_LONG_CLICK;
	for (int i=0; i<4; i++)
		m_last_relative_vol[i] = -1;
}



// virtual
void rigOld::onButtonEvent(int row, int col, int event)
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
			int inc = row == 1 ? 1 : -1;
			int vol = thePedals.getRelativeLoopVolume(col) + inc;
			if (vol < 0) vol = 0;
			if (vol > 127) vol = 127;
			thePedals.setRelativeLoopVolume(col,vol);

			// send the CC for the one pedal that has (possibly) changed

			expressionPedal *pedal = thePedals.getPedal(col);
			float pedal_vol = pedal->getDisplayValue();
			float rel_vol = vol;
			float new_value = (pedal_vol/127.0) * rel_vol;
			int cc = NEW_LOOP_VOLUME_TRACK1 + col;

			mySendDeviceControlChange(
				cc,
				new_value,
				pedal->getCCChannel());
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

	else if (num == IPAD_PROG_CHANGE_BUTTON &&
		event == BUTTON_EVENT_CLICK)
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

	//	Guitar effects

    else if (row == 3)
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

    else if (row == 4)
    {
		#if SEND_LOOPER_BUTTONS_VIA_SERIAL3

			if (event == BUTTON_EVENT_PRESS ||
				event == BUTTON_EVENT_CLICK)
			{
				sendSerialControlChange(loop_ccs[col],0x7f,"rigOld Loop Button");
					// sends out cc_nums 21, 22, 23, 31, and 25 for the 5 buttons left to right
			}

		#else

			if (event == BUTTON_EVENT_LONG_CLICK)
			{
				theButtons.clearRadioGroup(GROUP_LOOPER);
				mySendDeviceControlChange(
					LOOP_CONTROL_CLEAR_ALL,
					0x7f,
					LOOP_CONTROL_CHANNEL);

				mySendDeviceControlChange(
					LOOP_CONTROL_CLEAR_ALL,
					0x00,
					LOOP_CONTROL_CHANNEL);

				// on a clear of the looper we reset
				// the relative volumes

				for (int i=0; i<4; i++)
					thePedals.setRelativeLoopVolume(i,90);

			}
			else if (event == BUTTON_EVENT_PRESS)
			{
				mySendDeviceControlChange(
					loop_ccs[col],
					0x7f,
					LOOP_CONTROL_CHANNEL);
			}
			else // RELEASE or CLICK
			{
				if (event == BUTTON_EVENT_CLICK)
					mySendDeviceControlChange(
						loop_ccs[col],
						0x7f,
						LOOP_CONTROL_CHANNEL);

				mySendDeviceControlChange(
					loop_ccs[col],
					0x00,
					LOOP_CONTROL_CHANNEL);
			}
		#endif
    }
}





// virtual
void rigOld::updateUI()
{
	if (m_quick_mode && m_quick_mode_time > QUICK_MODE_TIMEOUT)
	{
		endQuickMode();
	}

    bool draw_full = false;
    if (m_full_redraw)
	{
        draw_full = true;
        m_full_redraw = false;
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
			int cur_relative_volume = thePedals.getRelativeLoopVolume(i);
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
bool rigOld::onRotaryEvent(int num, int val)
{
	display(0,"rigOld::onRotaryEvent(%d,%d)",num,val);

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

	sendSerialControlChange(control_num + RPI_CONTROL_NUM_CC_OFFSET,val,"rigOld Rotary Control");
	return true;
}