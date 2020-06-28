#include <myDebug.h>
#include "winFtpTuner.h"
#include "defines.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"
#include "ftp.h"
#include "ftp_defs.h"
#include "winFtpSensitivity.h"
#include "winFtpSettings.h"

#define BUTTON_SENSITIVITY   0


//------------------------------------------------------------
// life cycle
//------------------------------------------------------------

winFtpTuner::winFtpTuner()
{
	init();
}

void winFtpTuner::init()
{
	draw_needed = 1;
	last_tuner_note = -1;
	last_tuner_value = 0;
	for (int i=0; i<NUM_STRINGS; i++)
		last_string_pressed[i] = -1;
}


// virtual
void winFtpTuner::begin(bool warm)
{
    sendFTPCommandAndValue(FTP_CMD_EDITOR_MODE, 0x02);
		// I think this should be called FTP_COMMAND_TUNER
		// as the only value that seems to work is the 2/0 bit

	init();
	expWindow::begin(warm);
	theButtons.setButtonType(THE_SYSTEM_BUTTON,BUTTON_EVENT_CLICK,LED_GREEN);
	theButtons.setButtonType(BUTTON_SENSITIVITY,BUTTON_EVENT_CLICK,LED_BLUE);
	showLEDs();
}


// virtual
void winFtpTuner::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_ROWS + col;
	if (num == THE_SYSTEM_BUTTON)
		endModal(0);
	else if (num == BUTTON_SENSITIVITY)
	{
		// not so easy to swap modal windows
		// and since we're in a button event,
		// updateUI() gets called between these
		// two calls.

		theSystem.swapModal(theSystem.getFtpSettings(),0);
	}
}




//------------------------------------------------------------
// updateUI (draw)
//------------------------------------------------------------
// fretboard

#define FRETBOARD_X     	10
#define FRETBOARD_Y     	210
#define FRETBOARD_WIDTH 	460
#define FRETBOARD_HEIGHT    100
#define BRIDGE_WIDTH        10
#define FRET_WIDTH          2
#define STRING_WIDTH        1
#define CIRCLE_DIAMETER     10

#define PRESSED_COLOR       TFT_YELLOW		// TFT_RED
#define FRET_COLOR 			TFT_RGB_COLOR(0x30,0x30,0x30)	// TFT_DARKGREY
#define FRETBOARD_COLOR 	TFT_RGB_COLOR(0x00,0x00,0x30)	// TFT_NAVY
#define STRING_COLOR 		TFT_RGB_COLOR(0x60,0x60,0x60)	// TFT_LIGHTGREY

#define NUM_INTERVALS       14
#define INTERVAL_WIDTH      ((FRETBOARD_WIDTH - 2*BRIDGE_WIDTH) / NUM_INTERVALS)
#define STRING_SPACING      (FRETBOARD_HEIGHT / NUM_STRINGS)

// tuner

#define TUNER_NOTE_X   		50
#define TUNER_NOTE_Y   		90
#define TUNER_NOTE_WIDTH    100
#define TUNER_NOTE_HEIGHT   60

#define TUNER_FRAME_X       160
#define TUNER_FRAME_Y       60
#define TUNER_FRAME_WIDTH   300
#define TUNER_FRAME_HEIGHT  120
#define TUNER_FRAME_MARGIN  10
#define TUNER_RANGE         (TUNER_FRAME_WIDTH - 2*TUNER_FRAME_MARGIN)
#define TUNER_MID_X   		(TUNER_FRAME_X + TUNER_FRAME_MARGIN + TUNER_RANGE/2)
#define TUNER_MID_Y   		(TUNER_FRAME_Y + TUNER_FRAME_MARGIN + (TUNER_FRAME_HEIGHT-TUNER_FRAME_MARGIN*2)/2)
#define TUNER_VALUE_X(v)	(TUNER_FRAME_X + TUNER_FRAME_MARGIN + ((float)(((float)(v) + 64)/128.0) * TUNER_RANGE))

#define TUNER_DISABLED_COLOR  TFT_RGB_COLOR(0x50,0x50,0x50)


//#define TUNER_BOX_OFFSET    ((TUNER_FRAME_WIDTH-TUNER_FRAME_MARGIN*2)/5)
//#define TUNER_BOX_WIDTH     (TUNER_BOX_OFFSET - TUNER_FRAME_MARGIN)
//#define TUNER_BOX_HEIGHT    (TUNER_FRAME_HEIGHT-2*TUNER_FRAME_MARGIN)
//#define UNUSED_BOX_COLOR    TFT_BLUE




void winFtpTuner::fretsToInts(int *ints)
{
	for (int i=0; i<NUM_STRINGS; i++)
	{
		ints[i] = -1;
	}

	__disable_irq();
	note_t *note = first_note;
	while (note)
	{
		if (note->string != -1 && note->fret != -1)
		{
			int i = note->fret;
			ints[note->string] = i > NUM_INTERVALS ? NUM_INTERVALS + 1 : i;
				// show everything else higher on the right
		}
		note = note->next;
	}
	__enable_irq();
}




void winFtpTuner::drawCircle(int string, int fret, bool pressed)
{
	if (fret > NUM_INTERVALS + 1)
		return;

	int center_x = fret == 0 ?
		BRIDGE_WIDTH/2 :
		BRIDGE_WIDTH + (INTERVAL_WIDTH/2) + (fret-1)*INTERVAL_WIDTH;

	if (fret > NUM_INTERVALS)
		center_x -= INTERVAL_WIDTH/2 - BRIDGE_WIDTH;

	int center_y = (STRING_SPACING/2) + STRING_SPACING * string;
	center_x += FRETBOARD_X;
	center_y += FRETBOARD_Y;

	// display(0,"drawCircle(%d,%d,%d)",string,fret,pressed);
	mylcd.Set_Draw_color(pressed ? PRESSED_COLOR : ((fret == 0 || fret > NUM_INTERVALS) ? FRET_COLOR : FRETBOARD_COLOR));
	mylcd.Fill_Circle(center_x,center_y,CIRCLE_DIAMETER / 2);

	if (!pressed)
	{
		mylcd.Fill_Rect(
			center_x - CIRCLE_DIAMETER/2,
			center_y,
			CIRCLE_DIAMETER + 1,
			STRING_WIDTH,
			STRING_COLOR);
	}
}


void winFtpTuner::drawTunerPointer(int tuner_x, int color)
{
	mylcd.Set_Draw_color(color);

	#define TRIANGLE_WIDTH 4

	// mylcd.Draw_Line(
	// 	tuner_x,
	// 	TUNER_FRAME_Y + TUNER_FRAME_MARGIN,
	// 	TUNER_MID_X,
	// 	TUNER_FRAME_Y + TUNER_FRAME_HEIGHT - 1 - TUNER_FRAME_MARGIN);

	int mid_x = tuner_x + (TUNER_MID_X - tuner_x) / 2;

	mylcd.Fill_Triangle(
		tuner_x,
		TUNER_FRAME_Y + TUNER_FRAME_MARGIN,
		mid_x - TRIANGLE_WIDTH,
		TUNER_MID_Y,
		mid_x + TRIANGLE_WIDTH,
		TUNER_MID_Y);
	mylcd.Fill_Triangle(
		TUNER_MID_X,
		TUNER_FRAME_Y + TUNER_FRAME_HEIGHT - TUNER_FRAME_MARGIN,
		mid_x - TRIANGLE_WIDTH,
		TUNER_MID_Y,
		mid_x + TRIANGLE_WIDTH,
		TUNER_MID_Y);


	#if 0
		mylcd.Draw_Line(
			tuner_x-1,
			TUNER_FRAME_Y + TUNER_FRAME_MARGIN * 2,
			TUNER_MID_X-1,
			TUNER_FRAME_Y + TUNER_FRAME_HEIGHT - 1 - TUNER_FRAME_MARGIN);
		mylcd.Draw_Line(
			tuner_x+1,
			TUNER_FRAME_Y + TUNER_FRAME_MARGIN * 2,
			TUNER_MID_X+1,
			TUNER_FRAME_Y + TUNER_FRAME_HEIGHT - 1 - TUNER_FRAME_MARGIN);
	#endif
}


// virtual
void winFtpTuner::updateUI()	// draw
{
	bool full_draw = 0;
	if (draw_needed)
	{
		full_draw = 1;
		draw_needed = 0;

		//----------------------------------
		// fretboard
		//----------------------------------

		mylcd.Fill_Rect(		// board
			FRETBOARD_X,
			FRETBOARD_Y,
			FRETBOARD_WIDTH,
			FRETBOARD_HEIGHT,
			FRETBOARD_COLOR);
		mylcd.Fill_Rect(		// left bridge
			FRETBOARD_X,
			FRETBOARD_Y,
			BRIDGE_WIDTH,
			FRETBOARD_HEIGHT,
			FRET_COLOR);
		mylcd.Fill_Rect(		// right bridge
			FRETBOARD_X + FRETBOARD_WIDTH - 1 - BRIDGE_WIDTH,
			FRETBOARD_Y,
			BRIDGE_WIDTH,
			FRETBOARD_HEIGHT,
			FRET_COLOR);

		// frets

		int x = FRETBOARD_X + BRIDGE_WIDTH + INTERVAL_WIDTH - FRET_WIDTH/2;
		for (int i=0; i<NUM_INTERVALS-1; i++)
		{
			mylcd.Fill_Rect(
				x,
				FRETBOARD_Y,
				FRET_WIDTH,
				FRETBOARD_HEIGHT,
				FRET_COLOR);
			x += INTERVAL_WIDTH;
		}

		// strings

		int y = FRETBOARD_Y + (STRING_SPACING/2);
		for (int i=0; i<NUM_STRINGS; i++)
		{
			mylcd.Fill_Rect(
				FRETBOARD_X,
				y,
				FRETBOARD_WIDTH,
				STRING_WIDTH,
				STRING_COLOR);
			y += STRING_SPACING;
		}

		// tuner frame

		mylcd.Set_Draw_color(TFT_WHITE);
		mylcd.drawBorder(
			TUNER_FRAME_X,
			TUNER_FRAME_Y,
			TUNER_FRAME_WIDTH,
			TUNER_FRAME_HEIGHT,
			2,
			TFT_WHITE);
		// zero tick
		mylcd.Draw_Line(
			TUNER_MID_X,
			TUNER_FRAME_Y,
			TUNER_MID_X,
			TUNER_FRAME_Y + TUNER_FRAME_MARGIN - 1);

		#define NUM_TICKS_PER_SIDE  5
		#define TICK_SPACE   (((TUNER_FRAME_WIDTH-TUNER_FRAME_MARGIN*2)/2) / NUM_TICKS_PER_SIDE)
		for (int i=0; i<NUM_TICKS_PER_SIDE; i++)
		{
			mylcd.Draw_Line(
				TUNER_MID_X - TICK_SPACE*i,
				TUNER_FRAME_Y,
				TUNER_MID_X - TICK_SPACE*i,
				TUNER_FRAME_Y + TUNER_FRAME_MARGIN - 1);
			mylcd.Draw_Line(
				TUNER_MID_X + TICK_SPACE*i,
				TUNER_FRAME_Y,
				TUNER_MID_X + TICK_SPACE*i,
				TUNER_FRAME_Y + TUNER_FRAME_MARGIN - 1);
		}
	}

	//------------------------------
	// fretboard pressed notes
	//------------------------------

	int pressed[6];
	fretsToInts(pressed);
	for (int i=0; i<NUM_STRINGS; i++)
	{
		if (full_draw || pressed[i] != last_string_pressed[i])
		{
			if (last_string_pressed[i] != -1)
				drawCircle(i,last_string_pressed[i],false);
			if (pressed[i] != -1)
				drawCircle(i,pressed[i],true);
			last_string_pressed[i] = pressed[i];
		}
	}


	//------------------------------
	// tuner
	//------------------------------
	// note

	int t_note = tuning_note ? tuning_note->val : -1;
	int t_value = tuning_note ? tuning_note->tuning : 0;
	int l_note = last_tuner_note;

	if (full_draw || t_note != last_tuner_note)
	{
		if (last_tuner_note != -1)
			mylcd.Fill_Rect(TUNER_NOTE_X,TUNER_NOTE_Y,TUNER_NOTE_WIDTH,TUNER_NOTE_HEIGHT,0);
		if (t_note != -1)
		{
	        mylcd.setFont(Arial_48);
			mylcd.Set_Text_Cursor(TUNER_NOTE_X + 5,TUNER_NOTE_Y + 5);
			mylcd.Set_Text_colour(TFT_WHITE);
            mylcd.print(noteName(t_note));
		}
		last_tuner_note = t_note;
	}

	// tuner value (slow movement by only moving one per loop)

	if (t_value > last_tuner_value)
		t_value = last_tuner_value + 1;
	if (t_value < last_tuner_value)
		t_value = last_tuner_value - 1;

	if (full_draw || t_note != l_note || t_value != last_tuner_value)
	{
		int tuner_value_x = TUNER_VALUE_X(t_value);
		int last_tuner_value_x = TUNER_VALUE_X(last_tuner_value);
		last_tuner_value = t_value;

		drawTunerPointer(last_tuner_value_x,0);
		int color = tuning_note ? (abs(t_value)<=2) ? TFT_GREEN : TFT_WHITE : TUNER_DISABLED_COLOR;
		drawTunerPointer(tuner_value_x,color);
		last_tuner_value_x = tuner_value_x;

		// TUNER LEDS

		#define TUNER_LED_BASE 16

		float pct;
		#define BRIGHT_PCT(p)   (((unsigned)(255.0*(p))) & 0xff)

		pct = (1-((float)-t_value)/32.0);	// pct good
		setLED(TUNER_LED_BASE + 0,
			tuning_note ?
				(t_value <= -2 && t_value >= -32) ?
					LED_RGB(BRIGHT_PCT(1-pct),BRIGHT_PCT(pct),0) :
				(t_value < -32) ?
					LED_RGB(0xff,0,0) :
					LED_RGB(0,0,0xff) :
				LED_RGB(0,0,0xff));

		// pct = ((float)abs(t_value))/4.0;
		setLED(TUNER_LED_BASE + 1,
			tuning_note ? (t_value >= -4 && t_value <= 2) ?
			LED_RGB(0,0xff,0) : // BRIGHT_PCT(1.0-pct),0) :
			LED_RGB(0,0,0xff) :
			LED_RGB(0,0,0xff));

		pct = (1-((float)t_value)/32.0);	// pct good
		setLED(TUNER_LED_BASE + 2,
			tuning_note ?
				(t_value >= 2 && t_value <= 32) ?
					LED_RGB(BRIGHT_PCT(1-pct),BRIGHT_PCT(pct),0) :
				(t_value > 32) ?
					LED_RGB(0xff,0,0) :
					LED_RGB(0,0,0xff) :
				LED_RGB(0,0,0xff));

		showLEDs();
	}

}
