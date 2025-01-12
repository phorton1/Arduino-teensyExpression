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

#define NUM_TEXT_LINES     15
#define TEXT_LINE_HEIGHT   18

#define KEYPAD_UP      7
#define KEYPAD_DOWN    17
#define KEYPAD_LEFT    11
#define KEYPAD_RIGHT   13
#define KEYPAD_SELECT  12
#define KEYPAD_CANCEL  24

char winSelectSong::selected_name[80] = {0};

#define MAX_SONG_LINES  5000
int line_map[MAX_SONG_LINES];

//------------------------------------------------------------
// life cycle
//------------------------------------------------------------

winSelectSong::winSelectSong(const char *sel_name) :
	expWindow(WIN_FLAG_DELETE_ON_END)
{
	init();
	if (sel_name && *sel_name)
		strcpy(selected_name,sel_name);	// 80 char max!!
}


void winSelectSong::init()
{
	draw_needed = 1;
	show_song_text = 0;
	num_text_lines = 0;
	top_text_line = 0;
	last_top_line = -1;


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
	theButtons.setButtonType(KEYPAD_LEFT,	BUTTON_TYPE_CLICK);
	theButtons.setButtonType(KEYPAD_RIGHT,	BUTTON_TYPE_CLICK);
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
		if (show_song_text)
		{
			top_text_line += (num == KEYPAD_DOWN)? 1 : -1;
			if (top_text_line + NUM_TEXT_LINES > num_text_lines)
				top_text_line = num_text_lines - NUM_TEXT_LINES;
			if (top_text_line < 0)  top_text_line = 0;
		}
		else if (num_songs)
		{
			selected_song += (num == KEYPAD_DOWN)? 1 : -1;
			if (selected_song < 0) selected_song = 0;	// num_songs-1;
			if (selected_song >= num_songs) selected_song = num_songs-1;	// 0;
		}
	}
	else if (num == KEYPAD_RIGHT)
	{
		if (num_songs)
		{
			last_top_line = -1;
			top_text_line = 0;
			num_text_lines = 0;

			const char *sname = songParser::getSongName(selected_song);
			show_song_text = songParser::openSongFile(sname);

			if (show_song_text)
			{
				int text_len = songParser::textLen();
				show_song_text[text_len] = 0;
				theSystem.setTitle(sname);
				for (int i=0; i<text_len; i++)
				{
					if (show_song_text[i] == 13)
					{
						if (num_text_lines < MAX_SONG_LINES-1)
						{
							num_text_lines++;
							line_map[num_text_lines] = i+2;		// also skip the lf
						}
						show_song_text[i] = 0;
					}
				}
			}
		}
	}
	else if (num == KEYPAD_LEFT)
	{
		draw_needed = 1;
		show_song_text = 0;
		theSystem.setTitle("Select Song");
	}

	else if (num == KEYPAD_CANCEL)
	{
		songParser::releaseSongNames();
		// selected_name[0] = 0;
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

	if (show_song_text && last_top_line != top_text_line)
	{
		last_top_line = top_text_line;
		fillRect(full_client_rect,TFT_BLACK);

		int i = 0;
		mylcd.setDefaultFont();
		mylcd.setTextSize(2);

		mylcd.setTextColor(TFT_WHITE);

		while (i<NUM_TEXT_LINES && last_top_line+i < num_text_lines)
		{
			char *line = &show_song_text[line_map[last_top_line+i]];
			mylcd.drawString(line,0,client_rect.ys + 5 + i * TEXT_LINE_HEIGHT);
			i++;
		}
		return;
	}

	// normal draw

	bool full_draw = draw_needed;
	draw_needed = 0;

	int top = top_song;
	int selected = selected_song;

	if (full_draw)
	{
		fillRect(full_client_rect,TFT_BLACK);
	}

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

			mylcd.setTextColor(fc);
			mylcd.fillRect(20,y,client_rect.width()-40,LINE_HEIGHT,bc);
			mylcd.drawString(songParser::getSongName(i),25,y+5);
		}
		y += LINE_HEIGHT;
	}

	last_selected_song = selected;
}
