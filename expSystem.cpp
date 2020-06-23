
#include <EEPROM.h>
#include <myDebug.h>
#include "expSystem.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"
#include "pedals.h"
#include "rotary.h"
#include "ftp.h"
#include "ftp_defs.h"
#include "myMidiHost.h"
#include "midiQueue.h"

#include "configSystem.h"
#include "patchOldRig.h"
#include "patchTest.h"
#include "patchMidiHost.h"
#include "winFtpTuner.h"
#include "winFtpSensitivity.h"


#define SHOW_SENT_MIDI  1

#define BATTERY_CHECK_TIME  30000


// Fishman TriplePlay MIDI HOST Spoof Notes
//
// This version WORKS as a midi host to the FTP dongle, appears in
// windows as a "Fishman TriplePlay" with similarly named
// midi ports, and successfully runs within the Windows FTP Editor,
// based on an EEPROM/configuration setting "SPOOF_FTP"
//
// REQUIRES setting MIDI4+SERIAL in Arduino IDE, as I did not want
// to muck around with Paul's midi.h file where it checks MIDI_NUM_CABLES
// inline, and IT is compiled with the original usb_desc.h, and it
// will not work properly as just a MIDI device (which uses SEREMU).
//
// Also note that the COM port changes from 3 to 11 when you change
// the SPOOF_FTP setting.
//
// As it stands right now, I am using a modified version of Paul's
// USBHost_t36.h file that exposes variables on it's MIDIDevice class,
// and makes a couple of functions (the usb IRQ handlers) virutal,
// so that I can inherit from it and implement the myMidiHost object,
// which tightly sends the hosted midi messges directly to the
// teensyDuino USBMidi, via the low level 'C' calls underlying
// it's hardwired "usbMIDI" class.
//
// HOST    myMidiHost : public USBHost_t36.h MIDIDevice
//
//      myMidiHost
//      Variable Name:  midi1 
//      Derivees from USBHost_t36.h MIDIDevice
//      Which has been modified to expose protected vars
//         and make a method rx_data() virtual
//      Spoof requires setting MIDI4+SERIAL in Arduino IDE
//      Hooks rx_data(), which is the host usb IRQ handler, to
//           directly call the low level 'C' routines
//           usb_midi_write_packed(msg) and usb_midi_flush_output()
//           upon every received packet.
//
// DEVICE (teensyDuino "self") usbMidi
//      Variable Name: usbMIDI (hardwired)
//      available based on USB Setting in Arduino IDE
//      I get it's messages based on calls to low calls to
//         low levl usb_midi_read_message() 'C' function
//         in the critical_timer_handler() implementation
//      which is where they get written TO the hosted device (FTP)
//         via the exposed USBHost_t36 MIDIDevice myMidiHost
//         midi_host.write_packed(msg) method
//
// IT WAS IMPORTANT AND HARD TO FIND THAT I HAD TO LOWER THE PRIORITY
// OF THE critical_timer to let the host usb IRQs have priority.
//
// THE SYSTEM IS NOT SYMETTRIC.  We read from the host based on direct
// usb IRQ's, but we read from the device based on a timer loop and the
// usb_midi_read_message() function.
//
// The IRQ is enqueing the 32bit messages (and I also modified USBHost_t36.h
// to increase the midi rx buffer size from 80 to 2048), which are currently,
// and messily, then dequeud in the "critical_timer_handler()" method, then
// printed to buffered text, and finally displayed in the updateUI() method
// called from loop().   That whole thing could be cleaned up to work with
// a single queue of 32 bit words, and to decode and show the queued messages
// separately for display.
//


//-------------------------------------
// critical timer loop
//-------------------------------------
// The critical_timer_handler() is ONLY used to dequeue DEVICE
// (teensyDuino) usb midi events and send them as rapidly as
// possible to the Hosted device and enqueue them for further
// processing in the normal processing loop.

#define EXP_CRITICAL_TIMER_INTERVAL 1000
#define EXP_CRITICAL_TIMER_PRIORITY  192
    // Available priorities:
    // Cortex-M4: 0,16,32,48,64,80,96,112,128,144,160,176,192,208,224,240
    // Cortex-M0: 0,64,128,192


//----------------------------------
// normal timer loop
//----------------------------------
// The "normal" timer loop does the bulk of the work in the system.
// It is used to
//
//      (a) check the BUTTONS, PEDALS, and ROTARY states and
//          generate events based on their changes.
//      (b) process incoming or outgoing MIDI as necessary
//          to generate program related events based on them.
//      (c) re-enqueue the incoming and outgoing (processed) MIDI
//          messags for display.
//      (d) used variously by other objects to implement key
//          repeats, etc.


#define EXP_TIMER_INTERVAL 5000
    // 5000 us = 5 ms == 200 times per second
#define EXP_TIMER_PRIORITY  240                     // lowest priority
    // compared to default priority of 128
    
// 1. I have more or less determined that the timers don't start again
//    until the handler has returned.
// 2. At some point the timers use so much resources that the rest of
//    the system is non functional

expSystem theSystem;


//----------------------------------------
// expWindow (base class)
//----------------------------------------


expWindow::expWindow()
{}


// virtual
void expWindow::begin()
    // derived classes should call base class method FIRST
    // base class clears all button registrations.
{
    theButtons.clear();
}



//----------------------------------------
// expSystem
//----------------------------------------


expSystem::expSystem()
{
    m_num_patches = 0;
    m_cur_patch_num = -1;
    m_prev_patch_num = 0;
	
	last_battery_level = 0;
	battery_time = BATTERY_CHECK_TIME;
	draw_needed	= 1;
	m_title = 0;

    for (int i=0; i<MAX_EXP_PATCHES; i++)
        m_patches[i] = 0;

}

void expSystem::setTitle(const char *title)
{
	m_title = title;
	draw_needed = 1;
}


void expSystem::begin()
{
    addPatch(new configSystem());
    addPatch(new patchOldRig());
    addPatch(new patchTest());
    addPatch(new patchMidiHost());
    addPatch(new winFtpTuner());
    addPatch(new winFtpSensitivity());
			
    theButtons.init();
    
    // get the brightness from EEPROM
    
    int brightness = EEPROM.read(EEPROM_BRIGHTNESS);
    if (brightness == 255)
        brightness = DEFAULT_BRIGHTNESS;
    setLEDBrightness(brightness);        

    // get config_num from EEPROM and activate it
    
    int patch_num = EEPROM.read(EEPROM_PATCH_NUM);
    if (patch_num == 255)
        patch_num = DEFAULT_CONFIG_NUM;
    if (patch_num == 255)
        patch_num = DEFAULT_CONFIG_NUM;
    if (patch_num >= m_num_patches)
        patch_num = m_num_patches - 1;

    patch_num = 0;
        // override EEPROM setting 
        // for working on a particular patch
    
    m_timer.priority(EXP_TIMER_PRIORITY);
    m_timer.begin(timer_handler,EXP_TIMER_INTERVAL);
    
    m_critical_timer.priority(EXP_CRITICAL_TIMER_PRIORITY);
    m_critical_timer.begin(critical_timer_handler,EXP_CRITICAL_TIMER_INTERVAL);
        // start the timer

    activatePatch(patch_num);
        // show the first windw
    
}


//-------------------------------------------------
// Config management
//-------------------------------------------------
    
void expSystem::addPatch(expWindow *pConfig)
{
    if (m_num_patches >= MAX_EXP_PATCHES + 1)
    {
        my_error("TOO MANY PATCHES! %d",m_num_patches+1);
        return;
    }
    m_patches[m_num_patches++] = pConfig;
}


void expSystem::activatePatch(int i)
{
    if (m_cur_patch_num >= m_num_patches)
    {
        my_error("attempt to activate illegal patch number %d",i);
        return;
    }
    
    // deactivate previous patch
    
    if (m_cur_patch_num >= 0)
    {
        getCurPatch()->end();
        m_prev_patch_num = m_cur_patch_num;
    }
    
    m_cur_patch_num = i;
    
    // clear the TFT and show the patch (window) title
    
    if (m_cur_patch_num)
    {
		draw_needed = 1;
        mylcd.Fill_Screen(0);
		setTitle(getCurPatch()->name());
    }
    
    // start the patch (window) running
    
    getCurPatch()->begin();
    
    // add the system long click handler
    
    theButtons.getButton(0,4)->m_event_mask |= BUTTON_EVENT_LONG_CLICK;
}    


//-----------------------------------------
// events
//-----------------------------------------


void expSystem::pedalEvent(int num, int value)
{
    if (!getCurPatch()->onPedalEvent(num,value))
	{
		expressionPedal *pedal = thePedals.getPedal(num);
		if (pedal->getCCChannel() && pedal->getCCNum())
		usbMIDI.sendControlChange(
			pedal->getCCNum(),
			value,
			pedal->getCCChannel());
		#if SHOW_SENT_MIDI
			display(0,"pedal(%d,%s) sent MIDI CC(%d,%d,%d)",
				num,
				pedal->getCCNum(),
				value,
				pedal->getCCChannel(),
				value);
		#endif
	}
}





void expSystem::rotaryEvent(int num, int value)
{
    getCurPatch()->onRotaryEvent(num,value);
}



void expSystem::buttonEvent(int row, int col, int event)
{
    // handle changes to configSystem
    
    if (row == 0 &&
        col == 4 &&
        m_cur_patch_num &&
        event == BUTTON_EVENT_LONG_CLICK)
    {
        setLED(0,4,LED_PURPLE);
        activatePatch(0);
    }
    else
    {
        getCurPatch()->onButtonEvent(row,col,event);
    }
}






//-----------------------------------------
// timer handlers
//-----------------------------------------


// static
void expSystem::critical_timer_handler()
{
    uint32_t msg = usb_midi_read_message();  // read from device
    
    if (msg)
    {
		// write it to the midi host

        midi_host.write_packed(msg);
        
        // enqueue it for processing (display from device)
        
        enqueueProcess(msg);
    }
}




// static
void expSystem::timer_handler()
{
	// basics
	
    theButtons.task();
	thePedals.task();
    pollRotary();

    // process incoming and outgoing midi events
    
    dequeueProcess();
    
	// call window handler
	
    theSystem.getCurPatch()->timer_handler();
}





//------------------------------
// updateUI
//-------------------------------
// battery indicator

#define BATTERY_X       426
#define BATTERY_Y       12
#define BATTERY_FRAME   2
#define BATTERY_WIDTH   36
#define BATTERY_HEIGHT  16



void expSystem::updateUI()
{
	if (battery_time > BATTERY_CHECK_TIME)
	{
	    sendFTPCommandAndValue(FTP_CMD_BATTERY_LEVEL, 0);
		battery_time = 0;
	}
	
	bool full_draw = 0;
	if (draw_needed)
	{
		draw_needed = 0;
		full_draw = 1;
		
		// title
		
        mylcd.setFont(Arial_16_Bold);
        mylcd.Set_Text_Cursor(10,10);
        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.print(m_title);
        mylcd.Set_Draw_color(TFT_YELLOW);
	    mylcd.Draw_Line(0,36,mylcd.Get_Display_Width()-1,36);

		//----------------------------------
		//  battery indicator
		//----------------------------------
		
		mylcd.Fill_Rect(
			BATTERY_X,
			BATTERY_Y,
			BATTERY_WIDTH,
			BATTERY_HEIGHT,
			TFT_DARKGREY);
		mylcd.Fill_Rect(
			BATTERY_X + BATTERY_WIDTH -1,
			BATTERY_Y + (BATTERY_HEIGHT/4),
			4,
			(BATTERY_HEIGHT/2),
			TFT_DARKGREY);
		mylcd.Fill_Rect(
			BATTERY_X + BATTERY_FRAME,
			BATTERY_Y + BATTERY_FRAME,
			BATTERY_WIDTH - 2*BATTERY_FRAME,
			BATTERY_HEIGHT - 2*BATTERY_FRAME,
			TFT_BLACK);
	}
	
	//------------------------------
	// battery indicator value
	//------------------------------
	
	if (full_draw ||
		last_battery_level != ftp_battery_level)
	{
		float pct = ftp_battery_level == -1 ? 1.0 : (((float)ftp_battery_level)-0x40) / 0x24;
		int color = ftp_battery_level == -1 ? TFT_LIGHTGREY : (pct <= .15 ? TFT_RED : TFT_DARKGREEN);
		if (pct > 1) pct = 1.0;
		
		display(0,"battery_level=%d   pct=%0.2f",ftp_battery_level,pct);
		
		float left_width = pct * ((float) BATTERY_WIDTH - 2*BATTERY_FRAME);
		float right_width = (1-pct) * ((float) BATTERY_WIDTH - 2*BATTERY_FRAME);
		int left_int = left_width;
		int right_int = right_width;
		
		mylcd.Fill_Rect(
			BATTERY_X + BATTERY_FRAME,
			BATTERY_Y + BATTERY_FRAME,
			left_int,
			BATTERY_HEIGHT - 2*BATTERY_FRAME,
			color);

		if (right_int > 0)
			mylcd.Fill_Rect(
				BATTERY_X + BATTERY_FRAME + left_int,
				BATTERY_Y + BATTERY_FRAME,
				right_int,
				BATTERY_HEIGHT - 2*BATTERY_FRAME,
				TFT_BLACK);
		
		last_battery_level = ftp_battery_level;
	}
		
	
	// current "process" function called from timer_handler()
	// dequeueProcess();
	
    getCurPatch()->updateUI();
}





            

    
