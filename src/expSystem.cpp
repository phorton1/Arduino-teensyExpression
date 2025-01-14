
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
#include "rigTest.h"
#include "rigMidiHost.h"


#define dbg_exp   0
	// 1 still shows midi messages
	// 0 shows SERIAL_DEVICE issues

#define GET_TEMPO_FROM_CLOCK           	0
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
const char *rig_names[MAX_EXP_RIGS];

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

    addRig(new configSystem());
    addRig(new rigLooper());
    // addRig(new rigNew());
    // addRig(new rigOld());
	addRig(new rigTest());
    addRig(new rigMidiHost());

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
        // show the first window

	if (!initFileSystem())	// fileSystem::init())
	{
        mylcd.setTextColor(TFT_YELLOW);
        mylcd.println("");
        mylcd.println("expSystem: COULD NOT START FILE SYSTEM!!");
		delay(10000);
	}

	display(dbg_exp,"returning from expSystem::begin()",0);
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


void expSystem::activateRig(int new_rig_num)
{
	display(dbg_exp,"activateRig(%d)",new_rig_num);

    if (new_rig_num >= m_num_rigs)
    {
        my_error("attempt to activate illegal rig number %d",new_rig_num);
        return;
    }

    // deactivate previous rig

    if (m_cur_rig_num >= 0)
    {
        getCurRig()->end();
        m_prev_rig_num = m_cur_rig_num;
    }

	// start the new rig

    m_cur_rig_num = new_rig_num;
	startWindow(getCurRig(),false);

    // add the system long click handler

	theButtons.getButton(0,THE_SYSTEM_BUTTON)->addLongClickHandler();
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
		getCurRig()->end();

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

	// old->onEndModal(old,param);
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
		getCurRig();

	startWindow(new_win,true);
	if (!m_num_modals && m_cur_rig_num)
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
		getCurRig()->onRotaryEvent(num,value);
}



void expSystem::buttonEvent(int row, int col, int event)
{

	int num = row * NUM_BUTTON_COLS + col;

	// modal windows get the event directly

	if (m_num_modals)
	{
		getTopModalWindow()->onButtonEvent(row,col,event);
	}

    // intercept long click on THE_SYSTEM_BUTTON
	// from rigs to go to the configSystem ...

	else if (num == THE_SYSTEM_BUTTON &&
 			 m_cur_rig_num &&
			 event == BUTTON_EVENT_LONG_CLICK)
	{
		setLED(THE_SYSTEM_BUTTON,LED_PURPLE);
		activateRig(0);
	}

	// else let the current rig have it

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
		// This experimental code is very processor intensive to
		// get the MIDI tempo from incoming midi clock messages.
		// It is defined out in my current 'production' code.

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

	theSystem.handleSerialData();

    // process incoming and outgoing midi events

    dequeueProcess();

	// call window handler

	if (theSystem.m_num_modals)
		theSystem.getTopModalWindow()->timer_handler();
	else
	    theSystem.getCurRig()->timer_handler();
}



// OLD
//	//--------------------------------------------------------
//	// Serial Port Handler
//	//--------------------------------------------------------
//	// Polls Serial and SERIAL_DEVICE for data.
//	// Handles incoming serial midi that starts with 0x0B and/or
//	// fileSystem command lines that start with "file_command:.*"
//	// Note that this implementation does not care about setting
//	// of PREF_FILE_SYSTEM_PORT ... it will accept file commands
//	// from either port.
//	//
//	// When a serial byte is received, this routine assumes a full
//	// packet is following (4 bytes for midi, or <cr-lf> for text)
//	// and reads the whole packet with blocking and a timeout
//
//	#define MAX_BASE64_BUF  10240
//		// agreed upon in console.pm
//	#define MAX_SERIAL_TEXT_LINE (MAX_BASE64_BUF+32)
//		// allow for "file_command:BASE64 " (13 + 6 + 1)
//
//
//	#define SERIAL_TIMEOUT  200		   // ms
//	char static_serial_buffer[MAX_SERIAL_TEXT_LINE+1];
//
//	volatile int fu = 0;
//
//
//	void expSystem::handleSerialData()
//	{
//		// The main USB Serial is only expected to contain lines of text
//		// SERIAL_DEVICE may contain either text or serial midi data
//
//		int buf_ptr = 0;
//		bool is_midi = false;
//		bool started = false;
//		bool finished = false;
//		elapsedMillis line_timeout = 0;
//		bool from_serial3 = 0;
//
//		if (Serial.available())
//		{
//			started = true;
//			while (!finished && buf_ptr<MAX_SERIAL_TEXT_LINE && line_timeout<SERIAL_TIMEOUT)
//			{
//				if (Serial.available())
//				{
//					int c = Serial.read();
//					if (c == 0x0A)				// LF comes last
//					{
//						static_serial_buffer[buf_ptr++] = 0;
//						finished = 1;
//
//					}
//					else if (c != 0x0D)			// skip CR
//					{
//						static_serial_buffer[buf_ptr++] = c;
//						line_timeout = 0;
//					}
//				}
//			}
//		}
//		else if (SERIAL_DEVICE.available())
//		{
//			started = true;
//			from_serial3 = 1;
//
//			int c = SERIAL_DEVICE.read();
//			if (c == 0x0B)
//			{
//				is_midi = 1;
//				static_serial_buffer[buf_ptr++] = c;
//				for (int i=0; i<3; i++)
//				{
//					while (!SERIAL_DEVICE.available()) {fu++;}
//					c = SERIAL_DEVICE.read();
//					static_serial_buffer[buf_ptr++] = c;
//				}
//				finished = true;
//			}
//			else
//			{
//				line_timeout = 0;
//				while (!finished && buf_ptr<MAX_SERIAL_TEXT_LINE)
//				{
//					if (c == 0x0A)			// LF comesl last
//					{
//						static_serial_buffer[buf_ptr++] = 0;
//						finished = 1;
//					}
//					else
//					{
//						if (c != 0x0D)			// skip CR
//						{
//							static_serial_buffer[buf_ptr++] = c;
//							line_timeout = 0;
//
//						}
//						while (!SERIAL_DEVICE.available())
//						{
//							if (line_timeout>=SERIAL_TIMEOUT)
//								break;
//						}
//	 					c = SERIAL_DEVICE.read();
//					}
//				}
//			}
//		}	// SERIAL_DEVICE.available()
//
//
//	 	if (started && !finished)
//		{
//			my_error("Could not finish serial input from_serial3(%d) is_midi(%d) buf_ptr(%d) %s",
//				from_serial3,
//				is_midi,
//				buf_ptr,
//				line_timeout>SERIAL_TIMEOUT ? "TIMEOUT" : "");
//			display_bytes(0,"BUF",(uint8_t*)static_serial_buffer,buf_ptr);
//		}
//		else if (finished && is_midi)
//		{
//			display_bytes(dbg_exp-1,"expSystem recv serial midi: ",(uint8_t*)static_serial_buffer,4);
//			theSystem.getCurRig()->onSerialMidiEvent(static_serial_buffer[2],static_serial_buffer[3]);
//		}
//		else if (finished)
//		{
//			if (!strncmp(static_serial_buffer,"file_command:",13))
//			{
//				char *p_command = &static_serial_buffer[13];
//				char *p_param = p_command;
//				while (*p_param && *p_param != ' ') p_param++;
//				if (*p_param == ' ') *p_param++ = 0;
//				fileSystem::handleFileCommand(p_command,p_param);
//			}
//			else
//			{
//				static_serial_buffer[buf_ptr+1] = 0;
//				my_error("expSystem got unexpected serial data from_serial3(%d) is_midi(%d) buf_ptr(%d) %s",
//					from_serial3,
//					is_midi,
//					buf_ptr,
//					line_timeout>SERIAL_TIMEOUT ? "TIMEOUT" : "");
//				display_bytes(0,"BUF",(uint8_t*)static_serial_buffer,buf_ptr);
//			}
//		}
//	}
//


//--------------------------------------------------------
// Command Parse
//--------------------------------------------------------
// File text start with file_command or file_message,
// followed by the req_num, size, and data
//
// file_command|file_message	\t req_num \t size \t data


#define SERIAL_TIMEOUT  200

#define FILE_COMMAND_SIG		"file_command"
#define FILE_MESSAGE_SIG		"file_message"

#define FILE_COMMAND_SIG_LEN	12
#define MAX_NUM_LENGTH			10

#define MIN_COMMAND_BUF_LEN		3   	// OK
#define MAX_COMMAND_BUF_LEN		15000   // BASE64 size offset ENCODED_CONTENT
	// probably have to tune this last one


typedef struct
{
	int state;
		// 0 = looking for SIG
		// 1 = got SIG, parsing req_num
		// 2 = got req_num parsing size
		// 3 = got size, created buf, adding bytes
	int off;
		// offset within the item being parsed, depending on state

	uint32_t timeout;

	// parsed contents

	int type;
		// 0 == file_command
		// 1 == file_message

	char req_num[MAX_NUM_LENGTH+1];
	char size[MAX_NUM_LENGTH+1];

	int  len;   // length of allocated buffer which is size + 1
	char *buf;

} parseCommand_t;


static parseCommand_t parse_command[2];


static void initParseCommand(parseCommand_t *pcmd)
{
	if (pcmd->buf)
		free(pcmd->buf);
	memset(pcmd,0,sizeof(parseCommand_t));
}




// PRH PRH
#define dbg_file_command -2
#define dbg_raw_midi 0

static void handleChar(bool is_serial, char c)
{
	// prh excluded from copy from TE2
	// if (!ACTIVE_FILE_SYS_DEVICE)
	// {
	// 	my_error("No ACTIVE_FILE_SYS_DEVICE in handleChar()",0);
	// 	return;
	// }

	parseCommand_t *pcmd = &parse_command[is_serial];

	display(dbg_file_command+2,"state(%d) off(%d) char=%c 0x%02x",
		pcmd->state,
		pcmd->off,
		c>=' '?c:'.',c);

	bool ok = 1;
	bool done = 0;

	if (pcmd->state == 0)									// parsing signature
	{
		char expected = pcmd->type ?
			FILE_MESSAGE_SIG[pcmd->off]:
			FILE_COMMAND_SIG[pcmd->off];

		if (c == '\t' && pcmd->off == FILE_COMMAND_SIG_LEN)	// got the signature
		{
			pcmd->off = 0;		// reset for next state
			pcmd->state++;       // advance to next state
			display_level(dbg_file_command+1,2,"handleChar() got %s",
				pcmd->type ? "file_message" : "file_command");
		}
		else if (c == expected)
		{
			pcmd->off++;
		}
		else if (!pcmd->type && pcmd->off == 5 && c == 'm')
		{
			pcmd->type = 1;
			pcmd->off++;
		}
		else
		{
			ok = 0;
			if (pcmd->off)
				my_error("handleChar() type(%d) off(%d) illegal char in SIG '%c'=0x%02x",
					pcmd->type,
					pcmd->off,
					c>=' '?c:'.',c);
		}
	}
	else if (pcmd->state == 1)						// parsing req_num
	{
		if (c == '\t')								// got length terminator
		{
			pcmd->req_num[pcmd->off] = 0;
			display(dbg_file_command+1,"handleChar(%s) got req_num(%s)",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num);

			pcmd->off = 0;		// reset for next state
			pcmd->state++;		// next state
		}
		else if (pcmd->off >= MAX_NUM_LENGTH)		// too big
		{
			ok = 0;
			my_error("handleChar(%s) off(%d) req_num overflow",
				pcmd->type?"file_message":"file_command",
				pcmd->off);
		}
		else if (c >= '0' && c <= '9')
		{
			pcmd->req_num[pcmd->off++] = c;
		}
		else
		{
			ok = 0;
			my_error("handleChar(%s) off(%d) illegal char in req_num '%c'=0x%02x",
				pcmd->type?"file_message":"file_command",
				pcmd->off,
				c>=' '?c:'.',c);
		}
	}
	else if (pcmd->state == 2)						// parsing size
	{
		if (c == '\t')								// got length terminator
		{
			pcmd->size[pcmd->off] = 0;
			display_level(dbg_file_command+1,2,"handleChar(%s,%s) got size(%s)",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num,
				pcmd->size);

			pcmd->len = atoi(pcmd->size);
			if (pcmd->len < MIN_COMMAND_BUF_LEN ||
				pcmd->len > MAX_COMMAND_BUF_LEN)
			{
				ok = 0;
				my_error("handleChar(%s,%s) len(%d) must be between %d and %d",
					pcmd->type?"file_message":"file_command",
					pcmd->req_num,
					pcmd->len,
					MIN_COMMAND_BUF_LEN,
					MAX_COMMAND_BUF_LEN);
			}
			else
			{
				pcmd->buf = (char *) malloc(pcmd->len + 1);		// allocate buffer
				if (!pcmd->buf)
				{
					ok = 0;
					my_error("handleChar(%s,%s) unable to allocate buffer of len(%d)",
						pcmd->type?"file_message":"file_command",
						pcmd->req_num,
						pcmd->len);
				}
				else
				{
					pcmd->off = 0;		// reset for next state
					pcmd->state++;		// next state
				}
			}
		}
		else if (pcmd->off >= MAX_NUM_LENGTH)		// too big
		{
			ok = 0;
			my_error("handleChar(%s,%s) off(%d) size overflow",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num,
				pcmd->off);
		}
		else if (c >= '0' && c <= '9')
		{
			pcmd->size[pcmd->off++] = c;
		}
		else
		{
			ok = 0;
			my_error("handleChar(%s,%s) off(%d) illegal char in size '%c'=0x%02x",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num,
				pcmd->off,
				c>=' '?c:'.',c);
		}
	}
	else if (pcmd->state == 3)			// adding characters to the file buffer
	{
		if (pcmd->off > pcmd->len)
		{
			ok = 0;
			my_error("handleChar(%s,%s) buffer overflow at off(%d)",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num,
				pcmd->off);
		}
		else if (c == '\n')
		{
			if (pcmd->off == pcmd->len)
			{
				done = 1;
				pcmd->buf[pcmd->off] = 0;		// terminate the buffer
			}
			else
			{
				ok = 0;
				my_error("handleChar(%s,%s) length mismatch off(%d) != len(%d)",
					pcmd->type?"file_message":"file_command",
					pcmd->req_num,
					pcmd->off,
					pcmd->len);
			}
		}
		else		// add character to file_command cuffer
		{
			pcmd->buf[pcmd->off++] = c;
		}
	}

	if (done)	// start the command or send the message
	{
		pcmd->timeout = 0;
		display(dbg_file_command,"handleChar() %s %s(%s) len=%d",
				pcmd->type?"queing":"starting",
				pcmd->type?"file_message":"file_command",
				pcmd->req_num,
				pcmd->len);
		int req_num = atoi(pcmd->req_num);
		if (pcmd->type)
		{
			if (!1)	// #PRH hasFileSystem())
			{
				warning(dbg_file_command,"handleChar() noFileSytem file_message(%d) dropping buffer of len(%d)",
					req_num,
					pcmd->len);
				free(pcmd->buf);
			}
			else if (!getCommand(req_num))
			{
				warning(dbg_file_command,"handleChar() find queue for file_message(%d) dropping buffer of len(%d)",
					req_num,
					pcmd->len);
				free(pcmd->buf);
			}
			else
			{
				if (!addCommandQueue(req_num,pcmd->buf))
				{
					warning(dbg_file_command,"handleChar() could not queue file_message(%d) dropping buffer of len(%d)",
						req_num,
						pcmd->len);
					free(pcmd->buf);
				}
			}
		}
		else if (!startCommand(req_num,pcmd->buf))
		{
			warning(dbg_file_command,"handleChar() could not start file_command(%d) dropping buffer of len(%d)",
				req_num,
				pcmd->len);
			free(pcmd->buf);
		}

		pcmd->buf = 0;
	}

	// !ok or done - init for new parse

	if (done || !ok)
		initParseCommand(pcmd);
	else if (ok)
		pcmd->timeout = millis();

}	// handleChar



//--------------------------------------------------------
// Serial Port Handler
//--------------------------------------------------------
// Polls Serial and SERIAL_DEVICE for data.
//
// At this time TE2 only allows Serial Midi over the SERIAL
// device or file_commands over either USB or SERIAL.
//
// Midi Packets start with 0x0B which should never be in plain text.
//
// File_commands start with file_command \t length \t reqnum \t data
// When a file_command is received, this routine assumes a full
// packet is following (4 bytes for midi, or <cr-lf> for text)
// and reads the whole packet with  a timeout


void expSystem::handleSerialData()
{
	// The main USB Serial is only expected to contain lines of text
	// SERIAL_DEVICE may contain either text or serial midi data

	for (int i=0; i<2; i++)
	{
		parseCommand_t *pcmd = &parse_command[i];
		if (pcmd->timeout && millis() - pcmd->timeout > SERIAL_TIMEOUT)
		{
			my_error("%s command timeout",i ? "SERIAL" : "USB");
			initParseCommand(pcmd);
		}
	}

	while (Serial.available())
	{
		int c = Serial.read();
		handleChar(0,c);
	}

	// only serialMidi sends 0x0B!!
	// and they can be sent in the middle of regular lines of text

	while (SERIAL_DEVICE.available())
	{
		int c = SERIAL_DEVICE.read();
		if (c == 0x0B)
		{
			uint8_t midi_buf[4];
			midi_buf[0] = c;

			int i = 1;
			uint32_t midi_timeout = millis();
			while (i<4 && millis() - midi_timeout < SERIAL_TIMEOUT)
			{
				if (SERIAL_DEVICE.available())
				{
					midi_buf[i++] = SERIAL_DEVICE.read();
					midi_timeout = millis();
				}
			}

			if (i < 4)
				my_error("serial midi timeout(%d)",i);
			else
			{
				if (dbg_raw_midi <= 0)
					display_level(dbg_raw_midi,0,"serial: 0x%02x%02x%02x%02x",
						midi_buf[3],
						midi_buf[2],
						midi_buf[1],
						midi_buf[0]);

			//	display_bytes(dbg_exp-1,"expSystem recv serial midi: ",(uint8_t*)static_serial_buffer,4);
			theSystem.getCurRig()->onSerialMidiEvent(midi_buf[2],midi_buf[3]);
				// prh - from TE2: enqueueMidi(false, MIDI_PORT_SERIAL,midi_buf);
			}
		}
		else
		{
			handleChar(1,c);
		}
	}	// SERIAL_DEVICE.available();
}	// handleSerialData()


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
	initQueryFTP();
		// query the FTP battery level on a timer

	expWindow *win = m_num_modals ?
		getTopModalWindow() :
		getCurRig();

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

	// tempo

	#if GET_TEMPO_FROM_CLOCK
		static int last_tempo = 0;
		if (m_tempo != last_tempo)
		{
			last_tempo = m_tempo;
			mylcd.setFont(Arial_14_Bold);
			mylcd.setTextColor(TFT_WHITE);
			mylcd.printfJustified(
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

	// call the current window's updateUI method

	win->updateUI();

}	// expSystem::updateUI()




#if !MIDI_ACTIVITY_INLINE
	void expSystem::midiActivity(int port_num)
	{
		midi_activity[port_num] = millis();
		display(dbg_exp,"midiActivity(%d)",port_num);
	}
#endif
