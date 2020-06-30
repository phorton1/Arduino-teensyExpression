#include <myDebug.h>
#include "patchMidiHost.h"
#include "defines.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"
#include "ftp.h"
#include "ftp_defs.h"
#include "myMidiHost.h"
#include "midiQueue.h"
#include "oldRig_defs.h"

#define TEST_TEMPO  1


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


//------------------------------------------------------------
// life cycle
//------------------------------------------------------------

patchMidiHost::patchMidiHost()
{
	dbg_bank_num = 0;
	dbg_patch_num = 0;
	dbg_command = 0x04;		// FTP_COMMAND_EDITOR_MODE
	dbg_param = 0x00;
	init();
}


void patchMidiHost::init()
{
	draw_needed = 1;
	for (int i=0; i<NUM_STRINGS; i++)
	{
		last_vel[i] = 0;
		last_velocity[i] = 0;
		last_sens[i] = 0;
	}
}



// virtual
void patchMidiHost::begin(bool warm)
{
	init();
	// initFTPifNeeded();
	expWindow::begin(warm);

	theButtons.setButtonType(PAD1_UP,   	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT );
	theButtons.setButtonType(PAD1_DOWN,		BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT );
	theButtons.setButtonType(PAD1_LEFT,		BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT );
	theButtons.setButtonType(PAD1_RIGHT,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT );
	theButtons.setButtonType(PAD1_SELECT,	BUTTON_EVENT_CLICK,  LED_GREEN);

	theButtons.setButtonType(PAD2_UP,   	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT );
	theButtons.setButtonType(PAD2_DOWN,		BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT );
	theButtons.setButtonType(PAD2_LEFT,		BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT );
	theButtons.setButtonType(PAD2_RIGHT,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT );
	theButtons.setButtonType(PAD2_SELECT,	BUTTON_EVENT_CLICK,  LED_GREEN);

	theButtons.setButtonType(20,			BUTTON_TYPE_TOGGLE, LED_GREEN, LED_ORANGE);
	theButtons.setButtonType(24,			BUTTON_TYPE_CLICK,	LED_PURPLE);

	showLEDs();
}


// virtual
void patchMidiHost::end()
{
}



//------------------------------------------------------------
// events
//------------------------------------------------------------

void patchMidiHost::myIncDec(int inc, uint8_t *val)
{
	int i = *val;
	i += inc;
	if (i > 127) i = 0;
	if (i < 0) i = 127;
	*val = i;
}






// virtual
void patchMidiHost::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_COLS + col;
	if (num == PAD1_UP || num == PAD1_DOWN)
	{
		myIncDec(num==PAD1_UP ? 1 : -1, &dbg_patch_num);
		display(0,"setting patch_num to %02x",dbg_patch_num);
	}
	else if (num == PAD1_LEFT || num == PAD1_RIGHT)
	{
		myIncDec(num==PAD1_RIGHT ? 1 : -1, &dbg_bank_num);
		if (dbg_bank_num > 3)
		{
			display(0,"bank_num(%02x) will send 0x0C control message to midi channel %d",dbg_bank_num,dbg_bank_num-2);
		}
		else
		{
			display(0,"setting bank_num to %02x",dbg_bank_num);
		}
	}
	else if (num == PAD2_UP || num == PAD2_DOWN)
	{
		myIncDec(num==PAD2_UP ? 1 : -1, &dbg_command);
		const char *name = getFTPCommandName(dbg_command);
		if (!name) name = "unknown";
		display(0,"setting dbg command to 0x%02x=%s",dbg_command,name);

	}
	else if (num == PAD2_LEFT || num == PAD2_RIGHT)
	{
		myIncDec(num==PAD2_RIGHT ? 1 : -1, &dbg_param);
		display(0,"setting dbg_param to %02x",dbg_param);
	}
	else if (num == PAD1_SELECT)
	{
		if (dbg_bank_num > 3)
		{
			display(0,"Sending C%d  patch change message to patch %d on channel %d",
				dbg_bank_num-3,
				dbg_patch_num,
				dbg_bank_num-2);
			mySendDeviceProgramChange(dbg_patch_num,dbg_bank_num-2);
		}
		else
		{
			display(0,"getting patch(%d,%d)",dbg_bank_num,dbg_patch_num);
			uint8_t  ftpRequestPatch[]	= { 0xF0, 0x00, 0x01, 0x6E, 0x01, FTP_CODE_READ_PATCH, dbg_bank_num, dbg_patch_num, 0xf7 };
			mySendFtpSysex(sizeof(ftpRequestPatch),ftpRequestPatch);
			// midi_host.sendSysEx(sizeof(ftpRequestPatch),ftpRequestPatch,true);
		}
	}
	else if (num == PAD2_SELECT)
	{
		#if TEST_TEMPO

			// first of all, sending CC zero sets to 60pm
			//   - and quantiloop often uses tempos below that
			// looks like an exponential operation

			// 60, 62, 65, 67, 69, 71, 74,
			// 350

			// 290/128  = 2.265625 per increment
			// 60 + (2.226) * param

			#define FACTOR (290.0/127.0)
			float calc_temp = dbg_param;
			calc_temp = 60.0 + (calc_temp * FACTOR) + 0.5;

			display(0,"Sending CC(%d) For Tempo %d",dbg_param,((int)calc_temp));
			mySendDeviceControlChange(NEW_AUDIOBUS_CC_TEMPO, dbg_param, NEW_SELECT_RIG_CHANNEL);

		#else

			const char *name = getFTPCommandName(dbg_command);
			if (!name) name = "unknown";
			display(0,"sending command(0x%02x=%s)  param(%02x)",dbg_command,name,dbg_param);
			sendFTPCommandAndValue(dbg_command,dbg_param);

		#endif
	}
	else if (num == 24)
	{
		display(0,"trying to cold set dynamics sensitivity",0);
		sendFTPCommandAndValue(FTP_CMD_SPLIT_NUMBER,0x01);
		sendFTPCommandAndValue(FTP_CMD_DYNAMICS_SENSITIVITY,0x0A);
	}
	else if (num == 20)
	{
		#if 1

			// SWAP APPS on IPAD
			static int dbg_prog_num = 0;
			// for now I just wanna swap between audiobus and sampletank
			// dbg_prog_num = (dbg_prog_num + 1) % 4;

			dbg_prog_num = dbg_prog_num ? 0 : 3;

			mySendMidiMessage(0x09, NEW_SELECT_RIG_CHANNEL, dbg_prog_num + 1, 0x7f);
			mySendMidiMessage(0x08, NEW_SELECT_RIG_CHANNEL, dbg_prog_num + 1, 0);
				// send a NOTE_ON message (with 7f velocity) to channel 9 with the
				// note 1==Audiobus, 2=Tonestack, 3=SampleTank, 4=Quantiloop, followed
				// by a NOTE_OFF

		#elif 0

			arrayedButton *pb = theButtons.getButton(row,col);
			bool poly_mode = !pb->isSelected();
			sendFTPCommandAndValue(FTP_CMD_POLY_MODE,poly_mode);

		#else

			// display(0,"lcd ID=%08x",mylcd.Read_ID());
			mylcd.dim();
		#endif
	}
}



//------------------------------------------------------------
// updateUI (draw)
//------------------------------------------------------------


void patchMidiHost::vel2ToInts(int *vel, int *velocity)
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


void patchMidiHost::drawBox(int string, int box32, int vel16)
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
void patchMidiHost::updateUI()	// draw
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
