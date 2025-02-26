//---------------------------------------------------
// expSystem.cpp
//---------------------------------------------------
// There are exactly two current "rigs" .. the rig_looper,
// and the config_system, on top of which "modal windows"
// may be opened.


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
#include "fileSystem.h"
#include "configSystem.h"
#include "rigLooper.h"


#define dbg_exp   0
	// 1 still shows midi messages
	// 0 shows SERIAL_DEVICE issues

#define BATTERY_CHECK_TIME  			30000
#define MIDI_ACTIVITY_TIMEOUT 			90


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
// I have more or less determined that the timers don't start again
//    until the handler has returned.
// At some point the timers use so much resources that the rest of
//    the system can become non functional.  The following values
//    work.

#define EXP_TIMER_INTERVAL 5000
    // 5000 us = 5 ms == 200 times per second
#define EXP_TIMER_PRIORITY  240                     // lowest priority
    // compared to default priority of 128


//--------------------------------
// global variables
//--------------------------------

expSystem theSystem;
	// global static object

int_rect tft_rect(0,0,479,319);				// full screen
int_rect title_rect(0,0,479,35);			// not including line
int_rect full_client_rect(0,37,479,319);	// under line to bottom of screen
int_rect pedal_rect(0,235,479,319);			// 89 high, starting at 230
int_rect client_rect(0,37,479,235);			// under line to above pedals

#define SYNTH_RECT_HEIGHT 70
#define SONG_STATE_WIDTH  100
#define SONG_MSG1_WIDTH   120

int_rect synth_rect(
	client_rect.xs,
	client_rect.ys,
	client_rect.xe,
	client_rect.ys + SYNTH_RECT_HEIGHT-1);

int_rect song_title_rect(
	client_rect.xs,
	synth_rect.ye,
	client_rect.xe - SONG_STATE_WIDTH,
	synth_rect.ye + 35);

int_rect song_state_rect(
	song_title_rect.xe,
	synth_rect.ye,
	client_rect.xe,
	song_title_rect.ye);

int_rect song_msg_rect[2];
	// assigned in begin()



//----------------------------------------
// expWindow (base class)
//----------------------------------------


// virtual
void expWindow::endModal(uint32_t param)
{
    // called by modal windows when they end themselves
	theSystem.endModal(this,param);
}



//----------------------------------------
// expSystem
//----------------------------------------


expSystem::expSystem()
{
	m_cur_rig = 0;
	m_num_modals = 0;
	last_battery_level = 0;

	draw_pedals = 1;
	draw_title = 1;
	m_title = 0;

	song_msg_rect[0].assign(
		client_rect.xs,
		song_title_rect.ye + 10,
		client_rect.xs + SONG_MSG1_WIDTH - 1,
		client_rect.ye);

	song_msg_rect[1].assign(
		song_msg_rect[0].xe,
		song_title_rect.ye + 10,
		client_rect.xe,
		client_rect.ye);

}


void expSystem::setTitle(const char *title)
{
	m_title = title;
	draw_title = 1;
}


void expSystem::begin()
{
	display(dbg_exp,"expSystem::begin()",0);

	song_msg_rect[0].assign(
		client_rect.xs,
		song_title_rect.ye + 10,
		client_rect.xs + SONG_MSG1_WIDTH - 1,
		client_rect.ye);

	song_msg_rect[1].assign(
		song_msg_rect[0].xe,
		song_title_rect.ye + 10,
		client_rect.xe,
		client_rect.ye);

	for (int i=0; i<NUM_PORTS; i++)
	{
		midi_activity[i] = millis();
		last_midi_activity[i] = 0;
	}

    theButtons.init();
	thePedals.init();
	initRotary();

	// activate the looper "rig"
	
    activateRig(&rig_looper);

	// start the timers

    m_timer.priority(EXP_TIMER_PRIORITY);
    m_timer.begin(timer_handler,EXP_TIMER_INTERVAL);

    m_critical_timer.priority(EXP_CRITICAL_TIMER_PRIORITY);
    m_critical_timer.begin(critical_timer_handler,EXP_CRITICAL_TIMER_INTERVAL);

	display(dbg_exp,"returning from expSystem::begin()",0);
}


//-------------------------------------------------
// Rig management
//-------------------------------------------------

void expSystem::activateRig(expWindow *the_rig)
	// switches between "rig_looper" and "config_system"
	// which are special expWindows
{
	display(dbg_exp,"activateRig(%s)",the_rig->name());
    if (m_cur_rig)
		m_cur_rig->end();			// deactivate previous rig

	m_cur_rig = the_rig;
	startWindow(m_cur_rig,false);	// start the new rig

	theButtons.getButton(0,THE_SYSTEM_BUTTON)->addLongClickHandler();
	    // add the system long click handler
}


//----------------------------------------
// window management
//----------------------------------------

void expSystem::startWindow(expWindow *win, bool warm)
{
	display(dbg_exp,"startWindow(%d,%s)",warm,win->name());

	theButtons.clear();
	mylcd.fillScreen(0);
	if (!(win->m_flags & WIN_FLAG_OWNER_TITLE))
		setTitle(win->name());
	else
		draw_title = 1;
	if (win->m_flags & WIN_FLAG_SHOW_PEDALS)
		draw_pedals = 1;
	win->begin(warm);
}


void expSystem::startModal(expWindow *win)
{
	display(dbg_exp,"startModal(%s)",win->name());

	if (m_num_modals >= MAX_MODAL_STACK)
	{
		my_error("NUMBER OF MODAL WINDOWS EXCEEDED",m_num_modals);
		return;
	}
	if (!m_num_modals)
		m_cur_rig->end();
	m_modal_stack[m_num_modals++] = win;
	startWindow(win,false);
}



void expSystem::swapModal(expWindow *win, uint32_t param)
{
	display(dbg_exp,"swapModal(%s,0x%08x)",win->name(),param);

	if (!m_num_modals)
	{
		startModal(win);
		return;
	}
	expWindow *old = getTopModalWindow();
	old->end();
	m_modal_stack[m_num_modals-1] = win;
	startWindow(win,true);
	if (old->m_flags & WIN_FLAG_DELETE_ON_END)
		delete old;
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
	display(dbg_exp,"expSystem::endModal(%s,0x%08x)",win->name(),param);

	m_num_modals--;
	expWindow *new_win = m_num_modals ?
		getTopModalWindow() :
		m_cur_rig;
	startWindow(new_win,true);
	if (!m_num_modals)
	{
		// reset the system button handler
		theButtons.getButton(0,THE_SYSTEM_BUTTON)->addLongClickHandler();
		draw_pedals = 1;
	}
	new_win->onEndModal(win,param);
	if (win->m_flags & WIN_FLAG_DELETE_ON_END)
		delete win;
}



//-----------------------------------------
// events
//-----------------------------------------
// Pedal behavior is orchestrated in pedals.cpp

void expSystem::rotaryEvent(int num, int value)
{
	if (m_num_modals)
		getTopModalWindow()->onRotaryEvent(num,value);
	else
		m_cur_rig->onRotaryEvent(num,value);
}



void expSystem::buttonEvent(int row, int col, int event)
	// modal windows get the event directly
	// whereas we check for the config mode from rig_looper
{
	int num = row * NUM_BUTTON_COLS + col;
	if (m_num_modals)
	{
		getTopModalWindow()->onButtonEvent(row,col,event);
	}

	// start the config_system ...
	else if (num == THE_SYSTEM_BUTTON &&
 			 m_cur_rig == &rig_looper &&
			 event == BUTTON_EVENT_LONG_CLICK)
	{
		setLED(THE_SYSTEM_BUTTON,LED_PURPLE);
		activateRig(&config_system);
	}
	else
	{
		m_cur_rig->onButtonEvent(row,col,event);
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

		// we only write through to the midi host if we are spoofing

	    bool is_spoof = getPref8(PREF_SPOOF_FTP);
		if (is_spoof)
		{
	        midi_host.write_packed(msg);
			theSystem.midiActivity(pindex | INDEX_MASK_HOST | INDEX_MASK_OUTPUT);
				// add the host and output bits to map it to port 6 or 7
				// PORT_INDEX_HOST_OUTPUT0 or  PORT_INDEX_HOST_OUTPUT1
		}

        // enqueue it for processing (display from device)
		// if we are monitoring the input port, or it is the remote FTP

		if (getPref8(PREF_MONITOR_PORT0 + pindex) || (  		// if monitoring the port, OR
			(getPref8(PREF_FTP_PORT) == FTP_PORT_REMOTE) &&     // if this is the PREF_FTP_PORT==2==Remote, AND
			INDEX_CABLE(pindex)))                       		// cable=1
		{
	        enqueueProcess(msg);
		}
    }
}


// static
void expSystem::timer_handler()
{
	// basics

    theButtons.task();
	thePedals.task();
	pollRotary();

	// check SERIAL_DEVICE for incoming midi or file commands

	handleSerialData();
		// in common fileSerial.cpp file

    // process incoming and outgoing midi events

    dequeueProcess();

	// call window handler

	if (theSystem.m_num_modals)
		theSystem.getTopModalWindow()->timer_handler();
	else
	    theSystem.m_cur_rig->timer_handler();
}


void handleCommonMidiSerial(uint8_t *midi_buf)
	// externed in common fileSystem.h
{
	theSystem.m_cur_rig->onSerialMidiEvent(midi_buf[2],midi_buf[3]);
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


void expSystem::updateUI()
{
	expWindow *win = m_num_modals ?
		getTopModalWindow() :
		m_cur_rig;

	// query the FTP battery level on a timer,
	// but only from the rig_looper,
	// so we don't start handling FTP port changes
	// until they are accepted from config_system

	if (win == &rig_looper)
		initQueryFTP();


	//----------------------------------
	// PEDALS
	//----------------------------------
	// draw the pedal frame and titles if needed

	if (win->m_flags & WIN_FLAG_SHOW_PEDALS)
	{
		bool draw_pedal_values = false;
		int pedal_width = pedal_rect.width() / NUM_PEDALS;

		if (draw_pedals)
		{
			draw_pedals = false;
			draw_pedal_values = true;

			mylcd.fillRect(
				pedal_rect.xs,
				pedal_rect.ys,
				pedal_rect.width(),
				PEDAL_TEXT_AREA_HEIGHT,
				TFT_YELLOW);

			mylcd.setFont(Arial_18_Bold);   // Arial_16);
			mylcd.setTextColor(0);
			mylcd.setDrawColor(TFT_YELLOW);

			for (int i=0; i<NUM_PEDALS; i++)
			{
				mylcd.printfJustified(
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
					mylcd.drawLine(
						i*pedal_width,
						pedal_rect.ys + PEDAL_TEXT_AREA_HEIGHT,
						i*pedal_width,
						pedal_rect.ye);
			}
		}

		// draw the pedal numbers if they've changed

		for (int i=0; i<NUM_PEDALS; i++)
		{
			expressionPedal *pedal = thePedals.getPedal(i);
			if (draw_pedal_values || pedal->displayValueChanged())
			{
				pedal->clearDisplayValueChanged();
				int v = pedal->getDisplayValue();

				mylcd.setFont(Arial_40_Bold);   // Arial_40);
				mylcd.setTextColor(TFT_WHITE);

				mylcd.printfJustified(
					12+i*pedal_width,
					pedal_rect.ys + PEDAL_TEXT_AREA_HEIGHT + 7,
					100,
					pedal_rect.height() - PEDAL_TEXT_AREA_HEIGHT + 20,
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
	// Top Title Frame
	//---------------------------

	bool draw_title_frame = draw_title;

	if (draw_title)
	{
		draw_title = false;
		fillRect(title_rect,TFT_BLACK);

		// title text

		draw_title = false;
        mylcd.setFont(Arial_16_Bold);
        mylcd.setCursor(5,10);
        mylcd.setTextColor(TFT_YELLOW);
        mylcd.print(m_title);

		// horizontal line

        mylcd.setDrawColor(TFT_YELLOW);
	    mylcd.drawLine(0,36,mylcd.width()-1,36);

		// midi indicator frames

        mylcd.setDrawColor(TFT_WHITE);
		for (int i=0; i<NUM_PORTS; i++)
		{
			mylcd.fillCircle(
				INDICATOR_X + (i/2)*INDICATOR_PAIR_SPACING + (i&1)*INDICATOR_SPACING,
				INDICATOR_Y,
				INDICATOR_RADIUS + 1);
		}
	}

	//----------------------------------------
	// battery indicator frame and value
	//----------------------------------------

	if (draw_title_frame ||
		last_battery_level != ftp_battery_level)
	{
		//  battery indicator frame

		int battery_frame_color = ftp_battery_level == -1 ?
			TFT_DARKGREY :
			TFT_YELLOW;

		mylcd.fillRect(
			BATTERY_X,
			BATTERY_Y,
			BATTERY_WIDTH,
			BATTERY_HEIGHT,
			battery_frame_color);
		mylcd.fillRect(
			BATTERY_X + BATTERY_WIDTH -1,
			BATTERY_Y + (BATTERY_HEIGHT/4),
			4,
			(BATTERY_HEIGHT/2),
			battery_frame_color);
		mylcd.fillRect(
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

		mylcd.fillRect(
			BATTERY_X + BATTERY_FRAME,
			BATTERY_Y + BATTERY_FRAME,
			left_int,
			BATTERY_HEIGHT - 2*BATTERY_FRAME,
			color);

		if (right_int > 0)
			mylcd.fillRect(
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

				mylcd.setDrawColor(color);
				mylcd.fillCircle(
					INDICATOR_X + cable_pair*INDICATOR_PAIR_SPACING + out*INDICATOR_SPACING,
					INDICATOR_Y,
					INDICATOR_RADIUS);
			}
		}
	}

	// call the current window's updateUI method

	if (win)
		win->updateUI();

}	// expSystem::updateUI()




#if !MIDI_ACTIVITY_INLINE
	void expSystem::midiActivity(int port_num)
	{
		midi_activity[port_num] = millis();
		display(dbg_exp,"midiActivity(%d)",port_num);
	}
#endif
