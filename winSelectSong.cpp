#include <myDebug.h>
#include "winSelectSong.h"
#include "defines.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"
#include "songParser.h"
#include "expSystem.h"


#define NUM_PER_WINDOW  8
#define LINE_HEIGHT     33


#define KEYPAD_UP      7
#define KEYPAD_DOWN    17
#define KEYPAD_LEFT    11
#define KEYPAD_RIGHT   13
#define KEYPAD_SELECT  12
#define KEYPAD_CANCEL  24

char winSelectSong::selected_name[80] = {0};


//------------------------------------------------------------
// life cycle
//------------------------------------------------------------

winSelectSong::winSelectSong(const char *sel_name) :
	expWindow(WIN_FLAG_DELETE_ON_END)
{
	init();
	selected_name[0] = 0;
	if (sel_name)
		strcpy(selected_name,sel_name);	// 80 char max!!
}


void winSelectSong::init()
{
	draw_needed = 1;
	top_song = 0;
	num_songs = 0;
	selected_song = -1;
	last_selected_song = -1;
}




// virtual
void winSelectSong::begin(bool warm)
{
	init();
	expWindow::begin(warm);

	theButtons.setButtonType(KEYPAD_UP,   	BUTTON_TYPE_CLICK | BUTTON_MASK_REPEAT);
	theButtons.setButtonType(KEYPAD_DOWN,	BUTTON_TYPE_CLICK | BUTTON_MASK_REPEAT);
	// theButtons.setButtonType(KEYPAD_LEFT,	BUTTON_TYPE_CLICK);
	// theButtons.setButtonType(KEYPAD_RIGHT,	BUTTON_TYPE_CLICK);
	theButtons.setButtonType(KEYPAD_SELECT,	BUTTON_TYPE_CLICK, 	LED_GREEN);

	theButtons.setButtonType(THE_SYSTEM_BUTTON,	BUTTON_TYPE_CLICK, 	LED_GREEN);
	theButtons.setButtonType(KEYPAD_CANCEL,	    BUTTON_TYPE_CLICK,	LED_PURPLE);

	num_songs = songParser::getSongNames();
	if (num_songs)
	{
		selected_song = 0;
		if (selected_name[0])
		{
			for (int i=0; i<num_songs; i++)
			{
				if (!strcmp(songParser::getSongName(i),selected_name))
				{
					selected_song = i;
					break;
				}
			}
		}
	}

	showLEDs();
}


//------------------------------------------------------------
// events
//------------------------------------------------------------


// virtual
void winSelectSong::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_COLS + col;

	if (num == KEYPAD_UP || num == KEYPAD_DOWN)
	{
		if (num_songs)
		{
			selected_song += (num == KEYPAD_DOWN)? 1 : -1;
			if (selected_song < 0) selected_song = 0;	// num_songs-1;
			if (selected_song >= num_songs) selected_song = num_songs-1;	// 0;
		}
	}
	else if (num == KEYPAD_CANCEL)
	{
		songParser::releaseSongNames();
		selected_name[0] = 0;
		endModal(0);
	}
	else if (num == KEYPAD_SELECT ||
			 num == THE_SYSTEM_BUTTON)
	{
		strcpy(selected_name,songParser::getSongName(selected_song));
		songParser::releaseSongNames();
		endModal(selected_song+1);
	}
}



//------------------------------------------------------------
// updateUI (draw)
//------------------------------------------------------------


// virtual
void winSelectSong::updateUI()	// draw
{
	int top = top_song;
	int selected = selected_song;
	bool full_draw = draw_needed;
	draw_needed = 0;


	// bring it into view if needed

	if (selected < top)
	{
		top = selected;
		full_draw = 1;
	}
	else if (selected >= top + NUM_PER_WINDOW)
	{
		top = selected - NUM_PER_WINDOW + 1;
		full_draw = 1;
	}

	if (draw_needed)
	{
		fillRect(client_rect,TFT_BLACK);
	}

	mylcd.setFont(Arial_20);

	int y = client_rect.ys + 5;
	bool sel_changed = last_selected_song != selected;
	for (int i=top; i<top+NUM_PER_WINDOW; i++)
	{
		if (full_draw ||
			(sel_changed &&
			(i == selected || i == last_selected_song)))
		{
			int fc = TFT_YELLOW;
			int bc = TFT_BLACK;
			if (i == selected)
			{
				fc = TFT_WHITE;
				bc = TFT_BLUE;
			}

			mylcd.Set_Text_colour(fc);
			mylcd.Fill_Rect(20,y,client_rect.width()-40,LINE_HEIGHT,bc);
			mylcd.Print_String(songParser::getSongName(i),25,y+5);
		}
		y += LINE_HEIGHT;
	}

	last_selected_song = selected;
}
