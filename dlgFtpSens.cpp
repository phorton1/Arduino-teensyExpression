#include <myDebug.h>
#include "dlgFtpSens.h"
#include "defines.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"
#include "ftp.h"
#include "ftp_defs.h"
#include "myMidiHost.h"
#include "midiQueue.h"


#define PAD1_UP      1
#define PAD1_DOWN    11
#define PAD1_LEFT    5
#define PAD1_RIGHT   7
#define PAD1_SELECT  6

#define PAD2_UP      13
#define PAD2_DOWN    23
#define PAD2_LEFT    17
#define PAD2_RIGHT   19
#define PAD2_SELECT  18


uint8_t dbg_bank_num = 0;
uint8_t dbg_patch_num = 0;
uint8_t dbg_command = 0x04;		// FTP_COMMAND_EDITOR_MODE
uint8_t dbg_param = 0x00;
	
int sens_button_repeat = -1;
unsigned sens_button_repeat_time = 0;

int my_led_color[25] = {0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0, 0,0,0,0,0};



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
		last_vel[i] = 0;
		last_velocity[i] = 0;
		last_sens[i] = 0;
	}
}


void mySetLED(int i, int color)
{
	my_led_color[i] = color;
	setLED(i,color);
}
void myRestoreLED(int i)
{
	setLED(i,my_led_color[i]);
}




// virtual
void dlgFtpSens::begin()
{
	init();
	// initFTPifNeeded();
	expConfig::begin();	
	
	for (int i=0; i<25; i++)
		theButtons.setButtonEventMask(i, BUTTON_EVENT_CLICK);
	
	theButtons.setButtonEventMask(PAD1_UP,   	BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(PAD1_DOWN,   	BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(PAD1_LEFT,   	BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(PAD1_RIGHT,   BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(PAD1_SELECT,  BUTTON_EVENT_CLICK);

	mySetLED(PAD1_UP,      LED_BLUE);
	mySetLED(PAD1_DOWN,    LED_BLUE);
	mySetLED(PAD1_LEFT,    LED_BLUE);
	mySetLED(PAD1_RIGHT,   LED_BLUE);
	mySetLED(PAD1_SELECT,  LED_GREEN);

	theButtons.setButtonEventMask(PAD2_UP,   	BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(PAD2_DOWN,   	BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(PAD2_LEFT,   	BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(PAD2_RIGHT,   BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE);
	theButtons.setButtonEventMask(PAD2_SELECT,  BUTTON_EVENT_CLICK);
	
	mySetLED(PAD2_UP,      LED_BLUE);
	mySetLED(PAD2_DOWN,    LED_BLUE);
	mySetLED(PAD2_LEFT,    LED_BLUE);
	mySetLED(PAD2_RIGHT,   LED_BLUE);
	mySetLED(PAD2_SELECT,  LED_GREEN);
	
	mySetLED(24,LED_PURPLE);
	mySetLED(20,LED_GREEN);

	showLEDs();
}


// virtual
void dlgFtpSens::end()
{
	showTuningMessages = 1;
	showNoteInfoMessages = 1;
	showVolumeLevel = 1;
	showBatteryLevel = 1;
	showPerformanceCCs = 1;
}



//------------------------------------------------------------
// events
//------------------------------------------------------------

void myIncDec(int inc, uint8_t *val)
{
	int i = *val;
	i += inc;
	if (i > 127) i = 0;
	if (i < 0) i = 127;
	*val = i;
}






// virtual
void dlgFtpSens::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_COLS + col;
	if (event == BUTTON_EVENT_PRESS)
	{
		sens_button_repeat = num;
		sens_button_repeat_time = millis();
		navPad(num);
	}
	else if (event == BUTTON_EVENT_RELEASE)
	{
		sens_button_repeat_time = 0;
		myRestoreLED(num);
	}
	else
	{
		if (num == PAD1_SELECT)
		{
			display(0,"getting patch(%d,%d)",dbg_bank_num,dbg_patch_num);
			uint8_t  ftpRequestPatch[]	= { 0xF0, 0x00, 0x01, 0x6E, 0x01, FTP_CODE_READ_PATCH, dbg_bank_num, dbg_patch_num, 0xf7 };
			midi1.sendSysEx(sizeof(ftpRequestPatch),ftpRequestPatch,true); 	
			myRestoreLED(num);
		}
		else if (num == PAD2_SELECT)
		{
			display(0,"sending command(0x%02x=%s)  param(%02x)",dbg_command,getFTPCommandName(dbg_command),dbg_param);
			sendFTPCommandAndValue(dbg_command,dbg_param);
			myRestoreLED(num);
		}
		else if (num == 24)
		{
			display(0,"trying to cold set dynamics sensitivity",0);
			sendFTPCommandAndValue(FTP_CMD_SPLIT_NUMBER,0x01);
			sendFTPCommandAndValue(FTP_CMD_DYNAMICS_SENSITIVITY,0x0A);
			myRestoreLED(num);
		}
		else if (num == 20)
		{
			if (showTuningMessages)
			{
				setLED(num,LED_RED);
				showTuningMessages = 0;
				showNoteInfoMessages = 0;
				showVolumeLevel = 0;
				showBatteryLevel = 0;
				showPerformanceCCs = 0;
			}
			else
			{
				setLED(num,LED_GREEN);
				showTuningMessages = 1;
				showNoteInfoMessages = 1;
				showVolumeLevel = 1;
				showBatteryLevel = 1;
				showPerformanceCCs = 1;
			}
		}
	}
	showLEDs();
}
	
	
void dlgFtpSens::navPad(int num)
{
	if (num == PAD1_UP || num == PAD1_DOWN)
	{
		myIncDec(num==PAD1_UP ? 1 : -1, &dbg_patch_num);
		display(0,"setting patch_num to %02x",dbg_patch_num);
	}
	else if (num == PAD1_LEFT || num == PAD1_RIGHT)
	{
		myIncDec(num==PAD1_RIGHT ? 1 : -1, &dbg_bank_num);
		display(0,"setting bank_num to %02x",dbg_bank_num);
	}
	else if (num == PAD2_UP || num == PAD2_DOWN)
	{
		myIncDec(num==PAD2_UP ? 1 : -1, &dbg_command);
		display(0,"sending dbg command to 0x%02x=%s",dbg_command,getFTPCommandName(dbg_command));
		
	}
	else if (num == PAD2_LEFT || num == PAD2_RIGHT)
	{
		myIncDec(num==PAD2_RIGHT ? 1 : -1, &dbg_param);
		display(0,"setting dbg_param to %02x",dbg_param);
	}
}


// virtual
void dlgFtpSens::timer_handler()
{
    static elapsedMillis timer2 = 0;
    
    if (sens_button_repeat_time)
    {
        int dif = millis() - sens_button_repeat_time;
        if (dif > 350)
        {
            // starts repeating after 350ms
            // starts at 10 per second and accelerates to 200 per second over one seconds

            dif -= 350;
            if (dif > 1500) dif = 1500;
            unsigned interval = 1500 - dif;
            interval = 5 + (interval / 8);
        
            if (timer2 > interval)
            {
				navPad(sens_button_repeat);
                timer2 = 0;
            }
        }
    }    
}	

	
//------------------------------------------------------------
// updateUI (draw)
//------------------------------------------------------------


void dlgFtpSens::vel2ToInts(int *vel, int *velocity)
	// move the vel2 and velocity values from notes to local variable
	// and only change vel by 1 in the process
{
	for (int i=0; i<NUM_STRINGS; i++)
	{
		vel[i] = 0;		// zero indicator
		velocity[i] = 0;	// on or off
	}

	__disable_irq();
	note_t *note = first_note;
	while (note)
	{
		int i = note->string;
		int v = note->vel2;				// compressed velocity
		velocity[i] = note->vel;		// full velocity
		vel[i] = v;
		note = note->next;
	}
	__enable_irq();

}



#define SENS_TOP     			50
#define SENS_LEFT    			80
#define SENS_DIVS       		(2 * 15)
#define SENS_BOX_WIDTH  		9
#define SENS_BOX_X_OFFSET 		10		// one blank col of pixels between
#define SENS_BOX_HEIGHT         30		// two blank rows of pixels between
#define SENS_ROW_Y_OFFSET       34

#define SENS_START_GREEN        7
#define SENS_START_RED			(30-6)

#define SENS_HEIGHT				(SENS_ROW_Y_OFFSET * 6)
#define SENS_WIDTH				(SENS_BOX_X_OFFSET * SENS_DIVS)
#define SENS_BOTTOM  			(SENS_TOP  + SENS_HEIGHT - 1)
#define SENS_RIGHT   			(SENS_LEFT + SENS_WIDTH - 1)

#define SENS_COLOR_RED          TFT_RGB_COLOR(0xff,0,0)
#define SENS_COLOR_GREEN        TFT_RGB_COLOR(0,0xff,0)
#define SENS_COLOR_YELLOW       TFT_RGB_COLOR(0xff,0xff,0)
#define SENS_COLOR_DARK_RED     TFT_RGB_COLOR(0x40,0,0)
#define SENS_COLOR_DARK_GREEN   TFT_RGB_COLOR(0,0x40,0)
#define SENS_COLOR_DARK_YELLOW  TFT_RGB_COLOR(0x40,0x40,0)

#define SENS_MIDI_VEL_WIDTH     4
#define SENS_COLOR_MIDI_VEL     TFT_RGB_COLOR(0xff,0xff,0xff)


void dlgFtpSens::drawBox(int string, int box32, int vel16)
{
	bool on = (box32/2) < vel16;
	int color =
		box32 >= SENS_START_RED ?
			(on ? SENS_COLOR_RED : SENS_COLOR_DARK_RED) :
		box32 >= SENS_START_GREEN ?
			(on ? SENS_COLOR_GREEN : SENS_COLOR_DARK_GREEN) :
			(on ? SENS_COLOR_YELLOW : SENS_COLOR_DARK_YELLOW);
	mylcd.Fill_Rect(
		SENS_LEFT + box32 * SENS_BOX_X_OFFSET,
		SENS_TOP + string * SENS_ROW_Y_OFFSET,
		SENS_BOX_WIDTH,
		SENS_BOX_HEIGHT,
		color);
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
	
	int vel[6];
	int velocity[6];
	vel2ToInts(vel,velocity);
	for (int i=0; i<6; i++)
	{
		if (full_draw || last_vel[i] != vel[i])
		{
			if (last_velocity[i] && last_velocity[i] != velocity[i])
			{
				float pct = ((float)last_velocity[i]) / 127.0;
				int x = (((float)SENS_WIDTH-SENS_MIDI_VEL_WIDTH) * pct);
				mylcd.Fill_Rect(
					SENS_LEFT + x,
					SENS_TOP + i * SENS_ROW_Y_OFFSET - 1, // one pixel above
					SENS_MIDI_VEL_WIDTH,
					SENS_BOX_HEIGHT+2,					  // one pixel below
					0);
			}
			last_velocity[i] = velocity[i];
			
			last_vel[i] = vel[i];
			for (int j=0; j<SENS_DIVS; j++)
				drawBox(i,j,vel[i]);
				
			if (velocity[i])
			{
				float pct = ((float)velocity[i]) / 127.0;
				int x = (((float)SENS_WIDTH-SENS_MIDI_VEL_WIDTH) * pct);
				mylcd.Fill_Rect(
					SENS_LEFT + x,
					SENS_TOP + i * SENS_ROW_Y_OFFSET - 1, // one pixel above
					SENS_MIDI_VEL_WIDTH,
					SENS_BOX_HEIGHT+2,					  // one pixel below
					SENS_COLOR_MIDI_VEL);
			}
				
		}
	}
}

