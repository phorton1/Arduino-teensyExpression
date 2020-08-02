#include <myDebug.h>
#include "winFtpSettings.h"
#include "defines.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"
#include "ftp.h"
#include "ftp_defs.h"
#include "myMidiHost.h"
#include "winFtpSensitivity.h"


#define KEYPAD_UP      7
#define KEYPAD_DOWN    17
#define KEYPAD_LEFT    11
#define KEYPAD_RIGHT   13
#define KEYPAD_SELECT  12
#define BUTTON_TUNER   0



int winFtpSettings::ftp_settings[FTP_NUM_SETTINGS] = {
	1,	// poly_mode
	0,  // bend_mode
	0,	// layer_type
};



//------------------------------------------------------------
// life cycle
//------------------------------------------------------------


winFtpSettings::winFtpSettings()
{
}


// virtual
void winFtpSettings::begin(bool warm)
{
	draw_needed = 1;
	selected_item = 0;
	last_selected_item = -1;
	for (int i=0; i<FTP_NUM_SETTINGS; i++)
		last_value[i] = -1;

	// sendFTPCommandAndValue(FTP_CMD_POLY_MODE,ftp_poly_mode);

	expWindow::begin(warm);

	theButtons.setButtonType(KEYPAD_UP,   	BUTTON_TYPE_CLICK);
	theButtons.setButtonType(KEYPAD_DOWN,	BUTTON_TYPE_CLICK);
	theButtons.setButtonType(KEYPAD_LEFT,	BUTTON_TYPE_CLICK);
	theButtons.setButtonType(KEYPAD_RIGHT,	BUTTON_TYPE_CLICK);
	theButtons.setButtonType(KEYPAD_SELECT,	BUTTON_TYPE_CLICK, 	LED_GREEN);

	theButtons.setButtonType(BUTTON_TUNER,		BUTTON_TYPE_CLICK);
	theButtons.setButtonType(THE_SYSTEM_BUTTON,	BUTTON_TYPE_CLICK, 	LED_GREEN);

	showLEDs();
}


//------------------------------------------------------------
// events
//------------------------------------------------------------


// virtual
void winFtpSettings::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_COLS + col;

	if (num == KEYPAD_UP || num == KEYPAD_DOWN)
	{
		selected_item += (num == KEYPAD_DOWN)? 1 : -1;
		if (selected_item < 0) selected_item = FTP_NUM_SETTINGS-1;
		if (selected_item >= FTP_NUM_SETTINGS) selected_item = 0;
	}
	else if (num == KEYPAD_LEFT || num == KEYPAD_RIGHT || num==KEYPAD_SELECT)
	{
		int inc = (num == KEYPAD_LEFT) ? -1 : 1;
		int i = ftp_settings[selected_item];

		if (selected_item == FTP_SETTING_POLY_MODE)
		{
			sendFTPCommandAndValue(FTP_CMD_POLY_MODE,ftp_poly_mode?0:1);
				// wait for response to repaint
		}
		if (selected_item == FTP_SETTING_BEND_MODE)
		{
			int i = (ftp_bend_mode + inc) % 4;
			sendFTPCommandAndValue(FTP_CMD_SPLIT_NUMBER,0x01);
			sendFTPCommandAndValue(FTP_CMD_PITCHBEND_MODE,i);
				// wait for response to repaint
		}
		else if (selected_item == FTP_SETTING_PERF_LAYER_TYPE)
		{
			ftp_settings[selected_item] = (i + inc) % PERF_NUM_LAYER_TYPES;
		}
		else
		{
			ftp_settings[selected_item] = i ? 0 : 1;
		}
	}
	else if (num == THE_SYSTEM_BUTTON)
	{
		endModal(237);
	}
	else if (num == BUTTON_TUNER)
	{
		theSystem.swapModal(theSystem.getFtpSensitivity(),0);
	}
}



//------------------------------------------------------------
// updateUI (draw)
//------------------------------------------------------------

#define TOP_Y     	   60
#define LINE_HEIGHT    35
#define LEFT_X    	   20
#define LEFT_WIDTH     120
#define RIGHT_X        360
#define RIGHT_WIDTH    90


const char *getItemName(int i)
{
	if (i==FTP_SETTING_POLY_MODE             ) return "Poly Mode";
	if (i==FTP_SETTING_BEND_MODE             ) return "Bend Mode";
	if (i==FTP_SETTING_PERF_LAYER_TYPE       ) return "Layer Type";
	return "unknown item";
}

const char *getLayerTypeName(int i)
{
	if (i == PERF_LAYER_TYPE_NONE) return "None";
	if (i == PERF_LAYER_TYPE_1_5 ) return "1x5";
	if (i == PERF_LAYER_TYPE_2_4 ) return "2x4";
	if (i == PERF_LAYER_TYPE_3_3 ) return "3x3";
	if (i == PERF_LAYER_TYPE_4_2 ) return "4x2";
	if (i == PERF_LAYER_TYPE_5_1 ) return "5x1";
	return "unknown type";
}


const char *getBendModeName(int i)
{
	if (i == 1) return "Smooth";
	if (i == 2) return "Stepped";
	if (i == 3) return "Trigger";
	return "Auto";
}



const char *getItemValueString(int item, int val)
{
	if (item == FTP_SETTING_PERF_LAYER_TYPE)
		return getLayerTypeName(val);
	else if (item == FTP_SETTING_BEND_MODE)
		return getBendModeName(val);
	if (val) return "On";
	return "Off";
}



// virtual
void winFtpSettings::updateUI()	// draw
{
	__disable_irq();
	ftp_settings[0] = ftp_poly_mode;
	ftp_settings[1] = ftp_bend_mode;
	bool full_draw = draw_needed;
	int sel_item = selected_item;
	int last_sel = last_selected_item;
	draw_needed = 0;
	last_selected_item = selected_item;
	__enable_irq();

	bool selection_changed = last_sel != sel_item;

	for (int i=0; i<FTP_NUM_SETTINGS; i++)
	{
		if (full_draw)
		{
			mylcd.setFont(Arial_18_Bold);
			mylcd.Set_Text_colour(TFT_YELLOW);
			mylcd.Set_Text_Cursor(LEFT_X, TOP_Y + i*LINE_HEIGHT + 5);
			mylcd.print(getItemName(i));
		}

		int value = ftp_settings[i];

        if (full_draw ||
			last_value[i] != value ||
			(selection_changed && (
				sel_item == i ||
				last_sel == i	)))
		{
			last_value[i] = value;
			int color = i == sel_item ? TFT_BLUE : 0;
			mylcd.setFont(Arial_18);
			mylcd.Fill_Rect(
				RIGHT_X,
				TOP_Y + i*LINE_HEIGHT,
				RIGHT_WIDTH,
				LINE_HEIGHT-5,
				color);
			mylcd.printf_justified(
				RIGHT_X + 5,
				TOP_Y + i*LINE_HEIGHT + 5,
				RIGHT_WIDTH-10,
				LINE_HEIGHT,
				LCD_JUST_RIGHT,
				TFT_WHITE,
				0,
				false,
				"%s",
				getItemValueString(i,value));
		}
	}

}
