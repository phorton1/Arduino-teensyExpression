#include "patchNewRig.h"
#include <myDebug.h>
#include "defines.h"
#include "myLeds.h"
#include "myTFT.h"
#include "pedals.h"
#include "buttons.h"
#include "midiQueue.h"


#define GROUP_LOOPER 	7
#define GROUP_SYNTH		1
#define GROUP_GUITAR	2

#define FIRST_SYNTH_BUTTON   	0
#define LAST_SYNTH_BUTTON    	14
#define FIRST_EFFECT_BUTTON  	15
#define LAST_EFFECT_BUTTON    	19
#define FIRST_LOOP_BUTTON   	20
#define LAST_LOOP_BUTTON    	24



#define BUTTON_TYPE_LOOPER      (BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE | BUTTON_MASK_TOUCH | BUTTON_MASK_RADIO | BUTTON_GROUP(GROUP_LOOPER) )
#define BUTTON_TYPE_LOOP_CLEAR  (BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK | BUTTON_MASK_TOUCH | BUTTON_MASK_RADIO | BUTTON_GROUP(GROUP_LOOPER) )


//--------------------
// SampleTank
//--------------------



synthPatch_t patchNewRig::synth_patch[NUM_BUTTON_COLS * 3] = {
    {SYNTH_PATCH_BASS_MINUS,     "BASS_MINUS",  "P Bass Finger"},  // should be MM Bass Finger
    {SYNTH_PATCH_BRASS1,         "BRASS1",      "Drama Brass"},
    {SYNTH_PATCH_VOICES1,        "VOICES1",     "Vocal Oh"},
    {SYNTH_PATCH_SPACE1,         "SPACE1",      "Mega Motion 3"},   // was BRASS2
    {SYNTH_PATCH_SPACE3,         "SPACE3",      "Whispering Pad"},  // was STRINGS2

    {SYNTH_PATCH_BASS2,          "BASS2",       "Chorus Fretless"},
    {SYNTH_PATCH_ORGAN2,         "ORGAN2",      "Drawbars Bow"},
    {SYNTH_PATCH_EPIANO,         "EPIANO",      "Smooth FM Piano"},
    {SYNTH_PATCH_FLUTE2,         "FLUTE2",      "Psych Flute"},
    {SYNTH_PATCH_FX1,            "SPACE2",      "Mega Motion 4"},

    {SYNTH_PATCH_BASS1,          "BASS1",       "MM Bass Finger"},  //  + Jazz Kit
    {SYNTH_PATCH_ORGAN1,         "ORGAN1",      "Ballad B Pad"},
    {SYNTH_PATCH_PIANO1,         "PIANO1",      "Mellow Grand 2"},
    {SYNTH_PATCH_FLUTE1,         "FLUTE1",      "Orch Flute"},
    {SYNTH_PATCH_FX2,            "FX2",         "SFX Collection"},
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


//----------------
// Quantiloop
//----------------

int patchNewRig::loop_ccs[NUM_BUTTON_COLS] =
{
    LOOP_CONTROL_TRACK1,
    LOOP_CONTROL_TRACK2,
    LOOP_CONTROL_TRACK3,
    LOOP_CONTROL_TRACK4,
    LOOP_STOP_START_IMMEDIATE,
};







//====================================================================
// patchNewRig
//====================================================================



patchNewRig::patchNewRig()
{
    m_cur_patch_num = -1;    // 0..14
	for (int i=0; i<NUM_BUTTON_ROWS * NUM_BUTTON_COLS; i++)
	{
		m_event_state[i] = 0;
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

	// if the ipad rig needs to be changed, send the appropriate message

	if (g_ipad_is_new_rig != IPAD_RIG_NEW)
	{
		g_ipad_is_new_rig = IPAD_RIG_NEW;
		mySendDeviceProgramChange(NEW_PATCH_NUM_NEW_RIG, NEW_SELECT_RIG_CHANNEL);
	}

	// set system modal pedal CC's for the new rig (audiobus) upon entry

	thePedals.getPedal(PEDAL_SYNTH )->setCCs(AUDIO_BUS_CONTROL_CHANNEL,    NEW_AUDIOBUS_CC_SYNTH_VOLUME);
	thePedals.getPedal(PEDAL_LOOP  )->setCCs(AUDIO_BUS_CONTROL_CHANNEL,    NEW_AUDIOBUS_CC_LOOP_VOLUME);
 	thePedals.getPedal(PEDAL_WAH   )->setCCs(GUITAR_EFFECTS_CHANNEL, 	   GUITAR_WAH_CONTROL_CC);
	thePedals.getPedal(PEDAL_GUITAR)->setCCs(AUDIO_BUS_CONTROL_CHANNEL,    NEW_AUDIOBUS_CC_GUITAR_VOLUME);

	m_last_patch_num = -1;
    m_full_redraw = 1;

    for (int i=FIRST_SYNTH_BUTTON; i<=LAST_SYNTH_BUTTON; i++)
    	theButtons.setButtonType(i,	BUTTON_TYPE_RADIO(GROUP_SYNTH), 0);

    for (int i=FIRST_EFFECT_BUTTON; i<=LAST_EFFECT_BUTTON; i++)
        theButtons.setButtonType(i,	BUTTON_TYPE_TOGGLE | BUTTON_GROUP(GROUP_GUITAR), 0, LED_GREEN);
    theButtons.setButtonType(LAST_EFFECT_BUTTON, BUTTON_TYPE_TOGGLE | BUTTON_GROUP(GROUP_GUITAR) | BUTTON_EVENT_LONG_CLICK, 0, LED_GREEN);

    for (int i=FIRST_LOOP_BUTTON; i<=LAST_LOOP_BUTTON; i++)
        theButtons.setButtonType(i,BUTTON_TYPE_LOOPER, 0, LED_RED, LED_YELLOW);
    theButtons.setButtonType(LAST_LOOP_BUTTON,BUTTON_TYPE_LOOP_CLEAR, 0, LED_RED, LED_YELLOW);

	// set the (possibly saved) button states into the button array

	for (int i=0; i<NUM_BUTTON_ROWS * NUM_BUTTON_COLS; i++)
		theButtons.setEventState(i,m_event_state[i]);

    // if (m_cur_patch_num >= 0)
    //     theButtons.select(m_cur_patch_num,1);

    showLEDs();
}




// virtual
void patchNewRig::onButtonEvent(int row, int col, int event)
{
	// save the button state at any given time

	int num = row * NUM_BUTTON_COLS + col;
	// arrayedButton *pb = theButtons.getButton(row,col);
	// m_event_state[num] = pb->m_event_state & (BUTTON_STATE_SELECTED | BUTTON_STATE_TOUCHED);


    if (row < 3)
    {
		// we are relying on the quantiloop patch's track volume settings

        m_cur_patch_num = num;	// my patch number
		int prog_num = synth_patch[m_cur_patch_num].prog_num;	// device patch number

		// in New rig, we turn down the synth volume on Audio Bus on a patch change
		// and if the synth pedal(0) is Auto we send it to zero too ..

		expressionPedal *pedal = thePedals.getPedal(PEDAL_SYNTH);	// Synth Pedal
		mySendDeviceControlChange(
			pedal->getCCNum(),
			0,
			pedal->getCCChannel());

		// send out the patch change message to Sample Tank first ....

        mySendDeviceProgramChange(prog_num, SYNTH_PROGRAM_CHANNEL);

		// move the pedal if it's auto ...

		if (pedal->isAuto())
		{
			pedal->setAutoRawValue(0);
				// we don't need to back map through the curve for setting zero, fortunately

			// wait up to 1.5 second for change to complete
			// doesnt work because we are in a timer interrupt ...
			//    and I don't yet want to mess with changing priority of teensyReceiveByte
			//    (and am not even sure if that will work) so that it operates at a higher
			//    priority than the non-critical timer_handler()
			//
			// 		elapsedMillis  pedal_wait = 0;
			// 		while (pedal->getAutoRawValue() && pedal_wait < 1500) { delay(10); }
			// 		if (pedal->getAutoRawValue())
			// 		{
			// 			warning(0,"Synth Pedal did not get to zero.  value=%d",pedal->getAutoRawValue());
			// 		}
		}

		// then aftr giving as much time to sample tank as possible (by default) ...
		//
		// in New Rig we also set each voice in the multi to the default
		// volume (around 0.7).  For now we are only doing it for channel 1
		// but this would need to be done for all 6 channels in poly mode

		mySendDeviceControlChange(
			SYNTH_VOLUME_CC,  				// 7
			SYNTH_DEFAULT_VOICE_VOLUME,		// 0.7 * 127
			SYNTH_VOLUME_CHANNEL);			// 1


		#if 0
			// experimental ... bring synth pedal back to 0.7*127 (no back mapping through curves)
			delay(1000);
			pedal->setAutoRawValue(SYNTH_DEFAULT_VOICE_VOLUME);
		#endif

    }
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
    else // (row == 4)
    {
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
    }
}





// virtual
void patchNewRig::updateUI()
{
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


    if (m_cur_patch_num >= 0 &&
        (draw_full ||
         m_last_patch_num != m_cur_patch_num))
    {
        m_last_patch_num = m_cur_patch_num;
        mylcd.setFont(Arial_40_Bold);   // Arial_40);
        int y = 75;
        mylcd.printf_justified(
            0,y,mylcd.Get_Display_Width(),mylcd.getFontHeight(),
            LCD_JUST_CENTER,
            TFT_CYAN,
            TFT_BLACK,
			true,
            "%s",
            synth_patch[m_cur_patch_num].short_name);

        y += mylcd.getFontHeight();
        mylcd.setFont(Arial_32);   // Arial_40);
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