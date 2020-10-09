
#include <myDebug.h>
#include "expSystem.h"
#include "prefs.h"
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
#include "rigOld.h"
#include "rigNew.h"
#include "rigTest.h"
#include "rigMidiHost.h"

#define WITH_FILE_SYSTEM    1

#if WITH_FILE_SYSTEM
    #include "fileSystem.h"
#endif

#define dbg_exp   1
	// 1 still shows midi messages
	// 0 shows Serial3 issues

#define GET_TEMPO_FROM_CLOCK           	0
#define BATTERY_CHECK_TIME  			30000
#define MIDI_ACTIVITY_TIMEOUT 			90

// moved to ftp.cpp::initQueryFTP()
// #define BATTERY_CHECK_TIME  			30000


// Fishman TriplePlay MIDI HOST Spoof Notes
//
// This version WORKS as a midi host to the FTP dongle, appears in
// windows as a "Fishman TriplePlay" with similarly named
// midi ports, and successfully runs within the Windows FTP Editor,
// based on an pref setting "SPOOF_FTP"
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
//      Variable Name:  midi_host
//      Derives from USBHost_t36.h MIDIDevice
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
const char *rig_names[MAX_EXP_RIGS];

//----------------------------------------
// expWindow (base class)
//----------------------------------------


// virtual
void expWindow::begin(bool warm)
    // derived classes should call base class method FIRST
    // base class clears all button registrations.
{
    theButtons.clear();
}


// virtual
void expWindow::endModal(uint32_t param)
	// currently
{
    // called by modal windows when they end themselves
	theSystem.endModal(this,param);
}



//----------------------------------------
// expSystem
//----------------------------------------


expSystem::expSystem()
{
	m_tempo = 0;

    m_num_rigs = 0;
    m_cur_rig_num = -1;
    m_prev_rig_num = 0;

	m_num_modals = 0;

	last_battery_level = 0;

	// moved
	// battery_time = BATTERY_CHECK_TIME;

	draw_pedals = 1;
	draw_title = 1;
	m_title = 0;

    for (int i=0; i<MAX_EXP_RIGS; i++)
        m_rigs[i] = 0;
}


void expSystem::setTitle(const char *title)
{
	m_title = title;
	draw_title = 1;
}


void expSystem::begin()
{
    addRig(new configSystem());
    addRig(new rigNew());
    addRig(new rigOld());
    addRig(new rigTest());
    addRig(new rigMidiHost());

	for (int i=0; i<NUM_PORTS; i++)
	{
		midi_activity[i] = millis();
		last_midi_activity[i] = 0;
	}
	for (int i=0; i<m_num_rigs; i++)
		rig_names[i] = m_rigs[i]->short_name();

	setPrefMax(PREF_RIG_NUM,m_num_rigs-1);
	setPrefStrings(PREF_RIG_NUM,rig_names);

    theButtons.init();
	thePedals.init();
	initRotary();

    // set the brightness from prefs

    setLEDBrightness(getPref8(PREF_BRIGHTNESS));

    // get rig_num from prefs and activate it

    int rig_num = getPref8(PREF_RIG_NUM);
    if (rig_num >= m_num_rigs)
        rig_num = m_num_rigs - 1;

    // rig_num = 0;
        // override prefs setting
        // for working on a particular rig

    m_timer.priority(EXP_TIMER_PRIORITY);
    m_timer.begin(timer_handler,EXP_TIMER_INTERVAL);

    m_critical_timer.priority(EXP_CRITICAL_TIMER_PRIORITY);
    m_critical_timer.begin(critical_timer_handler,EXP_CRITICAL_TIMER_INTERVAL);
        // start the timer

    activateRig(rig_num);
        // show the first windw

	fileSystem::init();
}


//-------------------------------------------------
// Rig management
//-------------------------------------------------

void expSystem::addRig(expWindow *pRig)
{
    if (m_num_rigs >= MAX_EXP_RIGS + 1)
    {
        my_error("TOO MANY RIGS! %d",m_num_rigs+1);
        return;
    }
    m_rigs[m_num_rigs++] = pRig;
}


void expSystem::activateRig(int i)
{
    if (m_cur_rig_num >= m_num_rigs)
    {
        my_error("attempt to activate illegal rig number %d",i);
        return;
    }

    // deactivate previous rig

    if (m_cur_rig_num >= 0)
    {
        getCurRig()->end();
        m_prev_rig_num = m_cur_rig_num;
    }

    m_cur_rig_num = i;
	expWindow *cur_rig = getCurRig();

    // clear the TFT and show the rig (window) title

    if (m_cur_rig_num)
    {
        mylcd.Fill_Screen(0);
		if (cur_rig->m_flags & WIN_FLAG_SHOW_PEDALS)
			draw_pedals = 1;
		if (!(cur_rig->m_flags & WIN_FLAG_OWNER_TITLE))
			setTitle(cur_rig->name());
    	else
			draw_title = 1;
	}

    // start the rig (window) running

    cur_rig->begin(false);

    // add the system long click handler

	theButtons.getButton(0,THE_SYSTEM_BUTTON)->m_event_mask |= BUTTON_EVENT_LONG_CLICK;
}


//----------------------------------------
// modal windows
//----------------------------------------

void expSystem::startModal(expWindow *win)
{
	display(dbg_exp,"startModal(%s)",win->name());

	if (m_num_modals >= MAX_MODAL_STACK)
	{
		my_error("NUMBER OF MODAL WINDOWS EXCEEDED",m_num_modals);
		return;
	}

	if (!m_num_modals)
		getCurRig()->end();

	// ok, so the modal windows should start with a clean slate of
	// no buttons, but how does the client restore them?
	// by changing all the calls to expWindow::begin() to have
	// a "warm" option that means they were called coming down
	// the stack

	m_modal_stack[m_num_modals++] = win;

	theButtons.clear();
	mylcd.Fill_Screen(0);
	if (!(win->m_flags & WIN_FLAG_OWNER_TITLE))
		setTitle(win->name());
	else
		draw_title = 1;
	if (win->m_flags & WIN_FLAG_SHOW_PEDALS)
		draw_pedals = 1;

	win->begin(false);
}



void expSystem::swapModal(expWindow *win, uint32_t param)
{
	if (!m_num_modals)
	{
		startModal(win);
		return;
	}

	// ok, so the modal windows should start with a clean slate of
	// no buttons, but how does the client restore them?
	// by changing all the calls to expWindow::begin() to have
	// a "warm" option that means they were called coming down
	// the stack

	expWindow *old = getTopModalWindow();
	old->end();
	win->begin(1);
	m_modal_stack[m_num_modals-1] = win;
	m_num_modals++;
	endModal(old,param);
}




expWindow *expSystem::getTopModalWindow()
{
	if (m_num_modals)
		return m_modal_stack[m_num_modals-1];
	return 0;
}


void expSystem::endModal(expWindow *win, uint32_t param)
	// currently always acts on top of stack,
	// api allows for closing a window in the middle,
	// though it will not work at this time.
{
	m_num_modals--;
	expWindow *new_win = m_num_modals ?
		getTopModalWindow() :
		getCurRig();

	mylcd.Fill_Screen(0);
	if (!(new_win->m_flags & WIN_FLAG_OWNER_TITLE))
		setTitle(new_win->name());
	else
		draw_title = 1;
	new_win->begin(true);

	if (!m_num_modals && m_cur_rig_num)
	{
		// returning to a rig window
		// reset the system button handler
		theButtons.getButton(0,THE_SYSTEM_BUTTON)->m_event_mask |= BUTTON_EVENT_LONG_CLICK;
	}

	new_win->onEndModal(win,param);
	if (win->m_flags & WIN_FLAG_DELETE_ON_END)
		delete win;
}


//-----------------------------------------
// events
//-----------------------------------------
// prh - 2020-08-13:  Getting rid of "pedal events"
// Pedal behavior henceforth orchestrated from pedals.cpp

// void expSystem::pedalEvent(int num, int value)
// {
//     if (!getCurRig()->onPedalEvent(num,value))
// 	{
// 		expressionPedal *pedal = thePedals.getPedal(num);
// 		if (pedal->getCCChannel() && pedal->getCCNum())
//             mySendDeviceControlChange(
// 				pedal->getCCNum(),
// 				value,
// 				pedal->getCCChannel());
// 	}
// }
//




void expSystem::rotaryEvent(int num, int value)
{
	if (m_num_modals)
		getTopModalWindow()->onRotaryEvent(num,value);
	else
		getCurRig()->onRotaryEvent(num,value);
}



void expSystem::buttonEvent(int row, int col, int event)
{
    // handle THE_SYSTEM_BUTTON

	int num = row * NUM_BUTTON_COLS + col;

	if (m_num_modals)
		getTopModalWindow()->onButtonEvent(row,col,event);

	// handle the system button globally

	else if (num == THE_SYSTEM_BUTTON &&
 			 m_cur_rig_num &&
			 event == BUTTON_EVENT_LONG_CLICK)
	{
		setLED(THE_SYSTEM_BUTTON,LED_PURPLE);
		activateRig(0);
	}

	// else let the window have it

	else
	{
		getCurRig()->onButtonEvent(row,col,event);
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
		int pindex = (msg >> 4) & PORT_INDEX_MASK;
		theSystem.midiActivity(pindex);
			// the message comes in on port index 0 or 1
			// PORT_INDEX_DUINO_INPUT0 or PORT_INDEX_DUINO_INPUT1

		// MIDI CLOCK MESSAGES
		#if GET_TEMPO_FROM_CLOCK
			if (((msg >> 8) & 0xff) == 0xF8)
			{
				static int beat_counter = 0;
				static elapsedMillis bpm_millis = 0;
				if (beat_counter == 24)  // every 24 messages = 1 beat
				{
					float millis = bpm_millis;
					float bpm = 60000 / millis  + 0.5;
						// I *think* this is rock solid with Quantiloop
						// without rounding, if it's truncated to 1 decimal place
					theSystem.m_tempo = bpm;
						// I am going to use the nearest integer value
						// so if I change the tempo once, I can only
						// approximately return to the original tempo
						// which is the case anyways cuz of audio_bus's
						// implementation ...
					bpm_millis = 0;
					beat_counter = 0;
				}
				beat_counter++;
			}
		#endif 	// GET_TEMPO_FROM_CLOCK


		// we only write it to the midi host if we are spoofing

	    bool is_spoof = getPref8(PREF_SPOOF_FTP);
		if (is_spoof)
		{
	        midi_host.write_packed(msg);
			theSystem.midiActivity(pindex | INDEX_MASK_HOST | INDEX_MASK_OUTPUT);
				// add the host and output bits to map it to port 6 or 7
				// PORT_INDEX_HOST_OUTPUT0 or  PORT_INDEX_HOST_OUTPUT1
		}

        // enqueue it for processing (display from device)
		// if we are monitoring the input port, or its the remote FTP


		if (getPref8(PREF_MONITOR_PORT0 + pindex) || (  		// if monitoring the port, OR
			(getPref8(PREF_FTP_PORT) == FTP_PORT_REMOTE) &&     // if this is the PREF_FTP_PORT==2==Remote, AND
			INDEX_CABLE(pindex)))                       		// cable=1
		{
	        enqueueProcess(msg);
		}
    }
}



volatile int fu = 0;

// static
void expSystem::timer_handler()
{
	// basics

    theButtons.task();
	thePedals.task();
	pollRotary();

	// check Serial3 for incoming midi or file commands

	theSystem.handleSerialData();

    // process incoming and outgoing midi events

    dequeueProcess();

	// call window handler
	// for time being, modal windows absorb everything
	// except for the above call to dequeueProcess()

	if (theSystem.m_num_modals)
		theSystem.getTopModalWindow()->timer_handler();
	else
	    theSystem.getCurRig()->timer_handler();
}




//--------------------------------------------------------
// Serial Port Handler
//--------------------------------------------------------
// Polls Serial and Serial3 for data.
// Data on Serial3 *may* be midi data (4 byte packet starting with 0x0b)
// Data on either *may* be "file_command:.*"
// Note that this implementation does not care about setting
// of PREF_FILE_SYSTEM_PORT ... it will accept file commands
// from either port.
//
// When a serial byte is received, this routine assumes a full
// packet is following (4 bytes for midi, or <cr-lf> for text)
// and reads the whole packet with blocking and a timeout

#define MAX_BASE64_BUF  10240
	// agreed upon in console.pm
#define MAX_SERIAL_TEXT_LINE (MAX_BASE64_BUF+32)
	// allow for "file_command:BASE64 " (13 + 6 + 1)


#define SERIAL_TIMEOUT  200		   // ms
char static_serial_buffer[MAX_SERIAL_TEXT_LINE+1];

void expSystem::handleSerialData()
{
	// The main USB Serial is only expected to contain lines of text
	// Serial3 may contain either.

	int buf_ptr = 0;
	bool is_midi = false;
	bool started = false;
	bool finished = false;
	elapsedMillis line_timeout = 0;
	bool from_serial3 = 0;

	if (Serial.available())
	{
		started = true;
		while (!finished && buf_ptr<MAX_SERIAL_TEXT_LINE && line_timeout<SERIAL_TIMEOUT)
		{
			if (Serial.available())
			{
				int c = Serial.read();
				if (c == 0x0A)				// LF comes last
				{
					static_serial_buffer[buf_ptr++] = 0;
					finished = 1;

				}
				else if (c != 0x0D)			// skip CR
				{
					static_serial_buffer[buf_ptr++] = c;
					line_timeout = 0;
				}
			}
		}
	}
	else if (Serial3.available())
	{
		started = true;
		from_serial3 = 1;

		int c = Serial3.read();
		if (c == 0x0B)
			is_midi = 1;
		static_serial_buffer[buf_ptr++] = c;

		line_timeout = 0;
		while (!finished && buf_ptr<MAX_SERIAL_TEXT_LINE && line_timeout<SERIAL_TIMEOUT)
		{
			if (Serial3.available())
			{
				int c = Serial.read();
				if (is_midi)
				{
					static_serial_buffer[buf_ptr++] = c;
					line_timeout = 0;
					if (buf_ptr == 4)
						finished = 1;
				}
				else if (c == 0x0A)			// LF comesl last
				{
					static_serial_buffer[buf_ptr++] = 0;
					finished = 1;
				}
				else if (c != 0x0D)			// skip CR
				{
					static_serial_buffer[buf_ptr++] = c;
					line_timeout = 0;
				}
			}
		}
	}


	if (started && !finished)
	{
		my_error("Could not finish serial input from_serial3(%d) is_midi(%d) buf_ptr(%d) %s",
			from_serial3,
			is_midi,
			buf_ptr,
			line_timeout>SERIAL_TIMEOUT ? "TIMEOUT" : "");
		display_bytes(0,"BUF",(uint8_t*)static_serial_buffer,buf_ptr);
	}
	else if (finished && is_midi)
	{
		display_bytes(dbg_exp-1,"expSystem recv serial midi: ",(uint8_t*)static_serial_buffer,4);
		theSystem.getCurRig()->onSerialMidiEvent(static_serial_buffer[2],static_serial_buffer[3]);
	}
	else if (finished)
	{
		if (!strncmp(static_serial_buffer,"file_command:",13))
		{
			char *p_command = &static_serial_buffer[13];
			char *p_param = p_command;
			while (*p_param && *p_param != ' ') p_param++;
			if (*p_param == ' ') *p_param++ = 0;
			fileSystem::handleFileCommand(p_command,p_param);
		}
		else
		{
			static_serial_buffer[buf_ptr+1] = 0;
			my_error("expSystem got unexpected serial data from_serial3(%d) is_midi(%d) buf_ptr(%d) %s",
				from_serial3,
				is_midi,
				buf_ptr,
				line_timeout>SERIAL_TIMEOUT ? "TIMEOUT" : "");
			display_bytes(0,"BUF",(uint8_t*)static_serial_buffer,buf_ptr);
		}
	}
}




//------------------------------
// updateUI
//-------------------------------
// battery indicator

#define BATTERY_X       435
#define BATTERY_Y       12
#define BATTERY_FRAME   2
#define BATTERY_WIDTH   36
#define BATTERY_HEIGHT  16

#define INDICATOR_Y      	 	20
#define INDICATOR_RADIUS  	  	5
#define INDICATOR_X 			280
#define INDICATOR_PAIR_SPACING  40
#define INDICATOR_SPACING    	15

#define PEDAL_TEXT_AREA_HEIGHT  30


int_rect tft_rect(0,0,479,319);				// full screen
int_rect title_rect(0,0,479,35);			// not including line
int_rect full_client_rect(0,37,479,319);	// under line to bottom of screen
int_rect pedal_rect(0,230,479,319);			// 89 high, starting at 230
int_rect client_rect(0,37,479,229);			// under line to above pedals



void expSystem::updateUI()
{
	initQueryFTP();
		// query the FTP battery level on a timer

	expWindow *win = m_num_modals ?
		getTopModalWindow() :
		getCurRig();

	//----------------------------------
	// PEDALS
	//----------------------------------
	// draw the pedal frame if needed

	if (win->m_flags & WIN_FLAG_SHOW_PEDALS)
	{
		bool draw_pedal_values = false;
		int pedal_width = pedal_rect.width() / NUM_PEDALS;

		if (draw_pedals)
		{
			draw_pedals = false;
			draw_pedal_values = true;

			mylcd.Fill_Rect(
				pedal_rect.xs,
				pedal_rect.ys,
				pedal_rect.width(),
				PEDAL_TEXT_AREA_HEIGHT,
				TFT_YELLOW);

			mylcd.setFont(Arial_18_Bold);   // Arial_16);
			mylcd.Set_Text_colour(0);
			mylcd.Set_Draw_color(TFT_YELLOW);

			for (int i=0; i<NUM_PEDALS; i++)
			{
				mylcd.printf_justified(
					i*pedal_width,
					pedal_rect.ys + 5,
					pedal_width,
					PEDAL_TEXT_AREA_HEIGHT,
					LCD_JUST_CENTER,
					TFT_BLACK,
					TFT_YELLOW,
					false,
					"%s",
					thePedals.getPedal(i)->getName());

				if (i && i<NUM_PEDALS)
					mylcd.Draw_Line(
						i*pedal_width,
						pedal_rect.ys + PEDAL_TEXT_AREA_HEIGHT,
						i*pedal_width,
						pedal_rect.ye);
			}
		}

		// and draw the pedal numbers if they've changed
		// 2020-10-08 - the exp system just draws the display values
		// and the songMachine is free to changes them, and call
		// pedal[i].setDisplayValue() AND
		// thePedals.pedalEvent(int num, int value) separately
		// to change the display and send out the CCs

		for (int i=0; i<NUM_PEDALS; i++)
		{
			expressionPedal *pedal = thePedals.getPedal(i);
			if (draw_pedal_values || pedal->displayValueChanged())
			{
				pedal->clearDisplayValueChanged();
				int v = pedal->getDisplayValue();

				mylcd.setFont(Arial_40_Bold);   // Arial_40);
				mylcd.Set_Text_colour(TFT_WHITE);

				mylcd.printf_justified(
					12+i*pedal_width,
					pedal_rect.ys + PEDAL_TEXT_AREA_HEIGHT + 14,
					100,
					pedal_rect.height() - PEDAL_TEXT_AREA_HEIGHT - 14,
					LCD_JUST_CENTER,
					TFT_WHITE,
					TFT_BLACK,
					true,
					"%d",
					v);
			}
		}
	}

	//---------------------------
	// Title and "frame"
	//---------------------------

	bool draw_title_frame = draw_title;

	if (draw_title)
	{
		draw_title = false;

		// title text

		draw_title = false;
        mylcd.setFont(Arial_16_Bold);
        mylcd.Set_Text_Cursor(5,10);
        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.print(m_title);

		// horizontal line

        mylcd.Set_Draw_color(TFT_YELLOW);
	    mylcd.Draw_Line(0,36,mylcd.Get_Display_Width()-1,36);

		// midi indicator frames

        mylcd.Set_Draw_color(TFT_WHITE);
		for (int i=0; i<NUM_PORTS; i++)
		{
			mylcd.Fill_Circle(
				INDICATOR_X + (i/2)*INDICATOR_PAIR_SPACING + (i&1)*INDICATOR_SPACING,
				INDICATOR_Y,
				INDICATOR_RADIUS + 1);
		}
	}

	//------------------------------
	// battery indicator frame and value
	//------------------------------

	if (draw_title_frame ||
		last_battery_level != ftp_battery_level)
	{
		//  battery indicator frame

		int battery_frame_color = ftp_battery_level == -1 ?
			TFT_DARKGREY :
			TFT_YELLOW;

		mylcd.Fill_Rect(
			BATTERY_X,
			BATTERY_Y,
			BATTERY_WIDTH,
			BATTERY_HEIGHT,
			battery_frame_color);
		mylcd.Fill_Rect(
			BATTERY_X + BATTERY_WIDTH -1,
			BATTERY_Y + (BATTERY_HEIGHT/4),
			4,
			(BATTERY_HEIGHT/2),
			battery_frame_color);
		mylcd.Fill_Rect(
			BATTERY_X + BATTERY_FRAME,
			BATTERY_Y + BATTERY_FRAME,
			BATTERY_WIDTH - 2*BATTERY_FRAME,
			BATTERY_HEIGHT - 2*BATTERY_FRAME,
			TFT_BLACK);

		float pct = ftp_battery_level == -1 ? 1.0 : (((float)ftp_battery_level)-0x40) / 0x24;
		int color = ftp_battery_level == -1 ? TFT_LIGHTGREY : (pct <= .15 ? TFT_RED : TFT_DARKGREEN);
		if (pct > 1) pct = 1.0;

		// display(0,"battery_level=%d   pct=%0.2f",ftp_battery_level,pct);

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


	// MIDI INDICATORS (always if changed)
	// remap from by output-cable  Di0,Di1,Do0,Do1,Hi0,Hi1,Ho0,Ho1
	// to by cable-output:         Di0,Do0, Di1,Do1,  Hi0,Ho0, Hi1,Ho1

	unsigned now = millis();
	for (int cable_pair=0; cable_pair<NUM_PORTS/2; cable_pair++)
	{
		for (int out=0; out<2; out++)
		{
			#define INDEX_MASK_HOST     0x04
			#define INDEX_MASK_OUTPUT   0x02
			#define INDEX_MASK_CABLE    0x01

			int i = ((cable_pair<<1)&INDEX_MASK_HOST) + (out*INDEX_MASK_OUTPUT) + (cable_pair&1);

			bool midi_on =
				now > midi_activity[i] &&
				now < midi_activity[i] + MIDI_ACTIVITY_TIMEOUT;

			if (draw_title_frame ||  midi_on != last_midi_activity[i])
			{
				last_midi_activity[i] = midi_on;
				int color = midi_on ?
					out ? TFT_RED : TFT_GREEN :
					0;

				mylcd.Set_Draw_color(color);
				mylcd.Fill_Circle(
					INDICATOR_X + cable_pair*INDICATOR_PAIR_SPACING + out*INDICATOR_SPACING,
					INDICATOR_Y,
					INDICATOR_RADIUS);
			}
		}
	}

	// tempo

	#if GET_TEMPO_FROM_CLOCK
		static int last_tempo = 0;
		if (m_tempo != last_tempo)
		{
			last_tempo = m_tempo;
			mylcd.setFont(Arial_14_Bold);
			mylcd.Set_Text_colour(TFT_WHITE);
			mylcd.printf_justified(
				10,
				200,
				50,
				30,
				LCD_JUST_CENTER,
				TFT_WHITE,
				TFT_BLACK,
				true,
				"%d",
				m_tempo);
			display(0,"m_tempo=%d",m_tempo);
		}
	#endif

    // getCurRig()->updateUI();   // ??
	win->updateUI();

}




#if !MIDI_ACTIVITY_INLINE
	void expSystem::midiActivity(int port_num)
	{
		midi_activity[port_num] = millis();
		display(dbg_exp,"midiActivity(%d)",port_num);
	}
#endif
