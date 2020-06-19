#include <myDebug.h>
#include "midiHostConfig.h"
#include "defines.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"

// This file brings up the question of possibly implementing configurations
// that are NOT user selectable, like the FTP tuner/sensitivity "dialog"
// instead of using the systemConfig - preferences approach.
//
// Which then harkens to a base class that has the arrow keys, and
// a general "back" functionality that can return to a given configuration,
// (i.e. a stack of active configurations).
//
// I also need to look at Scrolling option lists in systemConfig, the
// touchScreen, and the possibility of gestures (i.e. swipe right) to
// alliviate the need to put a "back" button on every configuration screen
// if I am going to use the touch.
//
// I almost hate to digress to the touchscreen calibration (as well as the
// pedal calibration and curves), because it would be more fun to see the
// FTP thing light up.
//
// But there's a ton of work to get this right.  I am more of the assumption
// than ever that the FTP config and foot pedals are more or less "global",
// though there *might* be 'sets' of pedal configs for different end-user
// configurations, I think the FTP stuff is to unweildy to have multiple
// isntances floating around.  And I have not seriously approached, at all,
// the use of the rotary controls.
//
// Sheesh, there's a tone of work to do.


#define SHOW_MIDI_EVENTS   1


#if WITH_MIDI_HOST
	#define MIDI_PASS_THRU  1
	#include "myMidiHost.h"
#endif


#define BUTTON_ONE   20
#define BUTTON_TWO   21


//------------------------------------------------------------
// life cycle
//------------------------------------------------------------

midiHostConfig::midiHostConfig() 
{
    draw_needed = 0;
    redraw_needed = 0;
}


// virtual
void midiHostConfig::begin()
{
	expConfig::begin();	
    draw_needed = 1;

    theButtons.setButtonEventMask(BUTTON_ONE, BUTTON_EVENT_CLICK);
    theButtons.setButtonEventMask(BUTTON_TWO, BUTTON_EVENT_CLICK);
	setLED(BUTTON_ONE,LED_BLUE);
	setLED(BUTTON_TWO,LED_BLUE);
	
	showLEDs();
}




//------------------------------------------------------------
// events
//------------------------------------------------------------


// virtual
void midiHostConfig::onButtonEvent(int row, int col, int event)
{
	int num = row * NUM_BUTTON_COLS + col;

	if (num == BUTTON_ONE)		// sysex test
	{
		// SYSEX DEVICE ID REQUEST
		// Send:  F0 7E 7F 06 01 F7
		// Expected:           F0 7E 10 06 02 00 01 6E 00 01 00 01 02 21 01 00 F7
		// Got (same for both dongles) from HOST :
		//		SysEx Message: F0 7E 00 06 02 00 01 6E 00 01 00 02 01 55 01 00 F7  (end)
		//		SysEx Message: F0 7E 10
		//                     F0 7E 10 06 02 00 06 02 00 01 6E 00 01 6E 00 01 00 01 01 00 01 02 21 01 02 21 01 00 F7  (end)
		//		SysEx Message: 00 F7  (end)
		// Got from iPad:      F0 7E 10 06 02 00 01 6E 00 01 00 01 02 21 01 00 F7
		//      == expected


		#define REQUEST_LEN  6
		uint8_t data[REQUEST_LEN] = {0xF0, 0x7E, 0x7F, 0x06, 0x01, 0xF7};

		#if WITH_MIDI_HOST
			if (midi_host_on)
			{
				display(0,"sending request(%d) to MIDI_HOST",REQUEST_LEN);
				midi1.sendSysEx(REQUEST_LEN,data,true);
			}
			else
		#endif
		{
			display(0,"sending request(%d) as midi device",REQUEST_LEN);
			usbMIDI.sendSysEx(REQUEST_LEN,data,true); 	
		}

		setLED(num,LED_BLUE);
		showLEDs();
	}
	
	else if (num == BUTTON_TWO)		// trying to figure out senstitivy
	{
		display(0,"Sending control change B7 1F 3C as device",0);

		#if 0
			uint32_t msg = 0x3c1Fb70b;
			// doesn't pay any attention to MIDI_NUM_CABLES
			usb_midi_write_packed(msg);		// write as device
			usb_midi_flush_output();
		#else
			// Does pay attention to cables
			usbMIDI.sendControlChange(0x1F, 0x3C, 8, 0);
				// should send B7 1F 3C
		#endif

		setLED(num,LED_BLUE);
		showLEDs();
	}
}


//------------------------------------------------------------
// updateUI (draw)
//------------------------------------------------------------

// virtual
void midiHostConfig::updateUI()	// draw
{
	if (draw_needed)
	{
		#if 0
			mylcd.Fill_Screen(0);
			mylcd.setFont(Arial_16_Bold);
			mylcd.Set_Text_Cursor(10,10);
			mylcd.Set_Text_colour(TFT_YELLOW);
			mylcd.print(getCurConfig()->name());
			mylcd.Set_Draw_color(TFT_YELLOW);
			mylcd.Draw_Line(0,36,TFT_WIDTH,36);
		#endif

		draw_needed = false;
		mylcd.setFont(Arial_20);
		mylcd.Set_Text_Cursor(20,50);
		mylcd.Set_Text_colour(TFT_WHITE);
		
		#if WITH_MIDI_HOST
			mylcd.print("MIDI_HOST ");
			mylcd.println(midi_host_on ? "ON" : " is OFF!! it must be on!!");
		#else
			mylcd.println("WITH_MIDI_HOST is not defined!!");
		#endif
	}
}




