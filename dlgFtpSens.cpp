#include <myDebug.h>
#include "dlgFtpSens.h"
#include "defines.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"

#include "myMidiHost.h"
#include "midiQueue.h"
#include "FTP.h"
#include "ftp_defs.h"

	


	
//------------------------------------------------------------
// life cycle
//------------------------------------------------------------

dlgFtpSens::dlgFtpSens() 
{
	init();
}


void dlgFtpSens::init()
{
	draw_needed = 1;
	for (int i=0; i<NUM_STRINGS; i++)
	{
		last_string_val[i] = -1;
		last_string_sens[i] = -1;
	}
}


// virtual
void dlgFtpSens::begin()
{
	init();
	initFTPifNeeded();
	expConfig::begin();	
	
	//for (int i=0; i<5; i++)
	//	last_tuner_box_color[i] = -1;
    
	theButtons.setButtonEventMask(BUTTON_MOVE_UP,      BUTTON_EVENT_CLICK);		// BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(BUTTON_MOVE_DOWN,    BUTTON_EVENT_CLICK);		// BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(BUTTON_MOVE_LEFT,    BUTTON_EVENT_CLICK);		// BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(BUTTON_MOVE_RIGHT,   BUTTON_EVENT_CLICK);		// BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(BUTTON_SELECT,       BUTTON_EVENT_CLICK);		// BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	
	setLED(BUTTON_MOVE_UP,      LED_BLUE);
	setLED(BUTTON_MOVE_DOWN,    LED_BLUE);
	setLED(BUTTON_MOVE_LEFT,    LED_BLUE);
	setLED(BUTTON_MOVE_RIGHT,   LED_BLUE);
	setLED(BUTTON_SELECT,       LED_GREEN);
	
	showLEDs();
}




//------------------------------------------------------------
// events
//------------------------------------------------------------

int string_num = 0;


// virtual
void dlgFtpSens::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_COLS + col;
	if (num == BUTTON_MOVE_UP)
	{
		initFTPifNeeded(1);		// nada
	}
	else if (num == BUTTON_MOVE_LEFT)
	{
		// gotta send the ID request before it will respond correctly
		#define REQUEST_LEN  6
		uint8_t data[REQUEST_LEN] = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};
		display(0,"sending request(%d) to MIDI_HOST",REQUEST_LEN);
		midi1.sendSysEx(REQUEST_LEN,data,true);
		// midi1.flush();
		
        //sendFTPCommandAndValue(0x04, 0x02);

	}
	else if (num == BUTTON_MOVE_DOWN)
	{
        sendFTPCommandAndValue(FTP_BATTERY_LEVEL, 0);
	}
	else if (num == BUTTON_MOVE_RIGHT)
	{
        sendGetFTPSensitivityCommand(string_num++);
		if (string_num == 6) string_num = 0;
	}
}


//------------------------------------------------------------
// updateUI (draw)
//------------------------------------------------------------


void dlgFtpSens::vel2ToInts(int *ints)
{
	for (int i=0; i<NUM_STRINGS; i++)
	{
		ints[i] = -1;
	}

	// __disable_irq();
	note_t *note = first_note;
	while (note)
	{
		if (note->string != -1 && note->fret != -1)
		{
			ints[note->string] = note->vel2;
		}
		note = note->next;
	}
	// __enable_irq();
}


// virtual
void dlgFtpSens::updateUI()	// draw
{
	bool full_draw = 0;
	if (draw_needed)
	{
		full_draw = 1;
		draw_needed = 0;
	}
}

