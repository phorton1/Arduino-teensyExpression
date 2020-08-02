#include <myDebug.h>
#include "winFtpSensitivity.h"
#include "defines.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"
#include "ftp.h"
#include "ftp_defs.h"
#include "myMidiHost.h"


#define KEYPAD_UP      7
#define KEYPAD_DOWN    17
#define KEYPAD_LEFT    11
#define KEYPAD_RIGHT   13
#define KEYPAD_SELECT  12

#define ITEM_DYNAMIC_RANGE     6
#define ITEM_DYNAMIC_OFFSET    7
#define ITEM_TOUCH_SENSITIVITY 8



//------------------------------------------------------------
// life cycle
//------------------------------------------------------------

winFtpSensitivity::winFtpSensitivity()
{
	init();
	ftp_dynamic_range = 20;
	ftp_dynamic_offset = 10;
	ftp_touch_sensitivity = 4;
}


void winFtpSensitivity::init()
{
	draw_needed = 1;
	for (int i=0; i<NUM_STRINGS; i++)
	{
		last_vel[i] = 0;
		last_velocity[i] = 0;
	}
	for (int i=0; i<NUM_SENSITIVITY_ITEMS; i++)
		last_value[i] = 0;

	selected_item = 0;
	last_selected_item = 0;
}




// virtual
void winFtpSensitivity::begin(bool warm)
{
	// update the string sensitivity values

    for (int i=0; i<NUM_STRINGS; i++)
    {
	    sendFTPCommandAndValue(FTP_CMD_GET_SENSITIVITY, i);
    }

	sendFTPCommandAndValue(FTP_CMD_SPLIT_NUMBER,0x01);
	sendFTPCommandAndValue(FTP_CMD_DYNAMICS_SENSITIVITY,ftp_dynamic_range);
	sendFTPCommandAndValue(FTP_CMD_DYNAMICS_OFFSET,ftp_dynamic_offset);
	sendFTPCommandAndValue(FTP_CMD_TOUCH_SENSITIVITY,ftp_touch_sensitivity);

	// normal initialization

	init();
	expWindow::begin(warm);

	theButtons.setButtonType(KEYPAD_UP,   	BUTTON_TYPE_CLICK);
	theButtons.setButtonType(KEYPAD_DOWN,	BUTTON_TYPE_CLICK);
	theButtons.setButtonType(KEYPAD_LEFT,	BUTTON_TYPE_CLICK);
	theButtons.setButtonType(KEYPAD_RIGHT,	BUTTON_TYPE_CLICK);
	theButtons.setButtonType(KEYPAD_SELECT,	BUTTON_TYPE_CLICK, 	LED_GREEN);

	theButtons.setButtonType(THE_SYSTEM_BUTTON,	BUTTON_TYPE_CLICK, 	LED_GREEN);

	// theButtons.setButtonType(20,			BUTTON_TYPE_TOGGLE, LED_CYAN, LED_ORANGE);
	theButtons.setButtonType(24,			BUTTON_TYPE_CLICK,	LED_PURPLE);

	if (!ftp_poly_mode)
		theButtons.select(20,1);

	showLEDs();
}


//------------------------------------------------------------
// events
//------------------------------------------------------------


// virtual
void winFtpSensitivity::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_COLS + col;

	if (num == KEYPAD_UP || num == KEYPAD_DOWN)
	{
		selected_item += (num == KEYPAD_DOWN)? 1 : -1;
		if (selected_item < 0) selected_item = NUM_SENSITIVITY_ITEMS-1;
		if (selected_item >= NUM_SENSITIVITY_ITEMS) selected_item = 0;
	}
	else if (num == KEYPAD_LEFT || num == KEYPAD_RIGHT)
	{
		if (selected_item == ITEM_DYNAMIC_RANGE)
		{
			ftp_dynamic_range += (num == KEYPAD_RIGHT) ? 1 : -1;
			if (ftp_dynamic_range < 10) ftp_dynamic_range = 10;
			if (ftp_dynamic_range > 20) ftp_dynamic_range = 20;
			sendFTPCommandAndValue(FTP_CMD_SPLIT_NUMBER,0x01);
			sendFTPCommandAndValue(FTP_CMD_DYNAMICS_SENSITIVITY,ftp_dynamic_range);
		}
		else if (selected_item == ITEM_DYNAMIC_OFFSET)
		{
			ftp_dynamic_offset += (num == KEYPAD_RIGHT) ? 1 : -1;
			if (ftp_dynamic_offset < 0)  ftp_dynamic_offset = 0;
			if (ftp_dynamic_offset > 20) ftp_dynamic_offset = 20;
			sendFTPCommandAndValue(FTP_CMD_SPLIT_NUMBER,0x01);
			sendFTPCommandAndValue(FTP_CMD_DYNAMICS_OFFSET,ftp_dynamic_offset);
		}
		else if (selected_item == ITEM_TOUCH_SENSITIVITY)
		{
			ftp_touch_sensitivity += (num == KEYPAD_RIGHT) ? 1 : -1;
			if (ftp_touch_sensitivity < 0) ftp_touch_sensitivity = 0;
			if (ftp_touch_sensitivity > 9) ftp_touch_sensitivity = 9;
			sendFTPCommandAndValue(FTP_CMD_TOUCH_SENSITIVITY,ftp_touch_sensitivity);
		}
		else
		{
			int value = ftp_sensitivity[selected_item];
			if (value != -1)
			{
				value += (num == KEYPAD_RIGHT) ? 1 : -1;
				if (value < 0)  value = 0;
				if (value > 15) value = 15;
				sendFTPCommandAndValue(FTP_CMD_SET_SENSITIVITY,selected_item<<4 | value);
			}
		}
	}
	else if (num == THE_SYSTEM_BUTTON ||
			 num == KEYPAD_SELECT)
	{
		endModal(237);
	}


	// else if (num == 20)
	// {
	// 	arrayedButton *pb = theButtons.getButton(row,col);
	// 	ftp_poly_mode = !pb->isSelected();
	// 	sendFTPCommandAndValue(FTP_CMD_POLY_MODE,ftp_poly_mode);
	// }
	else if (num == 24)
	{
		#if 1
			for (int i=0; i<NUM_STRINGS; i++)
			{
				sendFTPCommandAndValue(FTP_CMD_GET_SENSITIVITY, i);
			}
		#else
			sendFTPCommandAndValue(FTP_CMD_SPLIT_NUMBER,0x01);
			sendFTPCommandAndValue(FTP_CMD_DYNAMICS_SENSITIVITY,ftp_dynamic_range);
			sendFTPCommandAndValue(FTP_CMD_DYNAMICS_OFFSET,ftp_dynamic_offset);
		#endif
	}
}



//------------------------------------------------------------
// updateUI (draw)
//------------------------------------------------------------


void winFtpSensitivity::vel2ToInts(int *vel, int *velocity)
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
#define SENS_BOX_HEIGHT         31		// two blank rows of pixels between
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

#define NUMBER_X  				(SENS_RIGHT + 10)
#define NUMBER_WIDTH  			40


void winFtpSensitivity::drawBox(int string, int box32, int vel16)
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
void winFtpSensitivity::updateUI()	// draw
{
	bool full_draw = draw_needed;
	draw_needed = 0;

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

	if (full_draw)
	{
		mylcd.setFont(Arial_16);
        mylcd.Set_Text_colour(TFT_YELLOW);
	    mylcd.Set_Text_Cursor(SENS_LEFT + 190,SENS_BOTTOM + 8);
		mylcd.print("Dyn Range");
	    mylcd.Set_Text_Cursor(SENS_LEFT + 190,SENS_BOTTOM + 3 + SENS_ROW_Y_OFFSET);
		mylcd.print("Dyn Offset");

	    mylcd.Set_Text_Cursor(SENS_LEFT,SENS_BOTTOM + 8);
		mylcd.print("Touch Sens");
	}

	bool selection_changed = last_selected_item != selected_item;

	for (int i=0; i<NUM_SENSITIVITY_ITEMS; i++)
	{
		int value =
			i == ITEM_DYNAMIC_RANGE ? ftp_dynamic_range :
			i == ITEM_DYNAMIC_OFFSET ? ftp_dynamic_offset :
			i == ITEM_TOUCH_SENSITIVITY ? ftp_touch_sensitivity :
			ftp_sensitivity[i] + 1;


        if (full_draw ||
			last_value[i] != value ||
			(selection_changed && (
				selected_item == i ||
				last_selected_item == i	)))
		{
			last_value[i] = value;
			int color = i == selected_item ? TFT_BLUE : 0;

			int x = i >= ITEM_TOUCH_SENSITIVITY ?
				SENS_LEFT + 120 :
				NUMBER_X;
			int y = i >= ITEM_TOUCH_SENSITIVITY ?
				SENS_TOP + (i-2) * SENS_ROW_Y_OFFSET - 3 :
				SENS_TOP + i * SENS_ROW_Y_OFFSET - 3;

			mylcd.setFont(Arial_16_Bold);
			mylcd.Fill_Rect(
				x,
				y,
				NUMBER_WIDTH,
				SENS_BOX_HEIGHT,
				color);
			mylcd.printf_justified(
				x + 5,
				y + 7,
				NUMBER_WIDTH-10,
				SENS_BOX_HEIGHT-6,
				LCD_JUST_RIGHT,
				TFT_WHITE,
				color,
				false,
				"%-2d",
				last_value[i]);
		}
	}

	last_selected_item = selected_item;
}
