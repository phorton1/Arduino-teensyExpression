//-----------------------------------------------
// configSystem.cpp
//-----------------------------------------------
// A "rig" (special expWindow) that handles configuration of the pedal.
//
// Configuration value changes have a one-to-one mapping directly to prefs.
// For most changes the system can handle them being changed asynchronously.
// However, certain changes, like changing the FTP port or SPOOF_FTP setting,
// would best be handled synchronously at the end, when the changes are accepted.
//
// As it is, right now, turning on FTP host mode is dangerous and crashes the system.


#include <myDebug.h>
#include "configSystem.h"
#include "prefs.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"
#include "pedals.h"
#include "rotary.h"
#include "expSystem.h"
#include "configOptions.h"
#include "expDialogs.h"
#include "rigLooper.h"

#define dbg_csys  0

#define BUTTON_BRIGHTNESS_DOWN  0
#define BUTTON_BRIGHTNESS_UP    1


configOption *rootOption = 0;
configOption *cur_menu = 0;
configOption *cur_option = 0;
configOption *display_menu = 0;
configOption *display_option = 0;
configOption *optBrightness = 0;

// terminal node modal dialog functions
// will be called with the menu number

#include "winFtpTuner.h"
#include "winFtpSensitivity.h"
#include "winConfigPedal.h"

void startFtpTuner(int i)
	{ theSystem.startModal(new winFtpTuner()); }
void startFtpSensitivity(int i)
	{ theSystem.startModal(new winFtpSensitivity()); }
void configPedal(int i)
	{ theSystem.startModal(new winConfigPedal(i)); }

ENABLED_CONFIG(ftpPortOption,!getPref8(PREF_SPOOF_FTP));

configSystem config_system;
	// global static object

//---------------------------------
// pseudo static initialization
//---------------------------------

void createOptions()
{
	rootOption = new configOption();

	optBrightness = new configOption(
		rootOption,
		"Brightness",
		0,
		PREF_BRIGHTNESS,
		setLEDBrightness);

	configOption *optFTP = new configOption(rootOption,"FTP");
	new configOption (optFTP,"Spoof FTP",	   OPTION_TYPE_NEEDS_REBOOT, PREF_SPOOF_FTP);
	new ftpPortOption(optFTP,"FTP Port",	   0,						 PREF_FTP_PORT);
	new configOption (optFTP,"FTP Tuner",	   0,						 PREF_NONE,		startFtpTuner);
	new configOption (optFTP,"FTP Sensitivity",0,						 PREF_NONE,		startFtpSensitivity);

	// pedals

	configOption *pedals = new configOption(rootOption,"Pedals");
	new configOption(pedals,"Configure Pedal1 (Synth)",	0,PREF_NONE,configPedal);
	new configOption(pedals,"Configure Pedal2 (Loop)",	0,PREF_NONE,configPedal);
	new configOption(pedals,"Configure Pedal3 (Wah)",	0,PREF_NONE,configPedal);
	new configOption(pedals,"Configure Pedal4 (Guitar)",0,PREF_NONE,configPedal);

	// midi monitor

	configOption *monitor = new configOption(rootOption,"Midi Monitor", 0,  PREF_MIDI_MONITOR);
	new configOption(monitor,"Midi Monitor", 0, PREF_MIDI_MONITOR);

	configOption *mon_ports = new configOption(monitor,"Ports");
	new configOption(mon_ports,"Duino Input 0",  0, PREF_MONITOR_DUINO_INPUT0);
	new configOption(mon_ports,"Duino Input 1",  0, PREF_MONITOR_DUINO_INPUT1);
	new configOption(mon_ports,"Duino Output 0", 0, PREF_MONITOR_DUINO_OUTPUT0);
	new configOption(mon_ports,"Duino Output 1", 0, PREF_MONITOR_DUINO_OUTPUT1);
	new configOption(mon_ports,"Host Input 0",   0, PREF_MONITOR_HOST_INPUT0);
	new configOption(mon_ports,"Host Input 1",   0, PREF_MONITOR_HOST_INPUT1);
	new configOption(mon_ports,"Host Output 0",  0, PREF_MONITOR_HOST_OUTPUT0);
	new configOption(mon_ports,"Host Output 1",  0, PREF_MONITOR_HOST_OUTPUT1);

	configOption *mon_channels = new configOption(monitor,"Channels");
	new configOption(mon_channels,"Midi Channel 1",  0, PREF_MONITOR_CHANNEL1 + 0);
	new configOption(mon_channels,"Midi Channel 2",  0, PREF_MONITOR_CHANNEL1 + 1);
	new configOption(mon_channels,"Midi Channel 3",  0, PREF_MONITOR_CHANNEL1 + 2);
	new configOption(mon_channels,"Midi Channel 4",  0, PREF_MONITOR_CHANNEL1 + 3);
	new configOption(mon_channels,"Midi Channel 5",  0, PREF_MONITOR_CHANNEL1 + 4);
	new configOption(mon_channels,"Midi Channel 6",  0, PREF_MONITOR_CHANNEL1 + 5);
	new configOption(mon_channels,"Midi Channel 7",  0, PREF_MONITOR_CHANNEL1 + 6);
	new configOption(mon_channels,"Midi Channel 8",  0, PREF_MONITOR_CHANNEL1 + 7);
	new configOption(mon_channels,"Midi Channel 9",  0, PREF_MONITOR_CHANNEL1 + 8);
	new configOption(mon_channels,"Midi Channel 10", 0, PREF_MONITOR_CHANNEL1 + 9);
	new configOption(mon_channels,"Midi Channel 11", 0, PREF_MONITOR_CHANNEL1 + 10);
	new configOption(mon_channels,"Midi Channel 12", 0, PREF_MONITOR_CHANNEL1 + 11);
	new configOption(mon_channels,"Midi Channel 13", 0, PREF_MONITOR_CHANNEL1 + 12);
	new configOption(mon_channels,"Midi Channel 14", 0, PREF_MONITOR_CHANNEL1 + 13);
	new configOption(mon_channels,"Midi Channel 15", 0, PREF_MONITOR_CHANNEL1 + 14);
	new configOption(mon_channels,"Midi Channel 16", 0, PREF_MONITOR_CHANNEL1 + 15);

	configOption *msg_types = new configOption(monitor,"Message Types");
	configOption *ftp_specific = new configOption(msg_types,"FTP Specific");

	new configOption(msg_types,"Sysex",			  0,  PREF_MONITOR_SYSEX);
	new configOption(msg_types,"Active Sense",	  0,  PREF_MONITOR_ACTIVESENSE);
	new configOption(msg_types,"Note On",		  0,  PREF_MONITOR_NOTE_ON);
	new configOption(msg_types,"Note Off",		  0,  PREF_MONITOR_NOTE_OFF);
	new configOption(msg_types,"Velocity ",		  0,  PREF_MONITOR_VELOCITY);
	new configOption(msg_types,"Program Chg",	  0,  PREF_MONITOR_PROGRAM_CHG);
	new configOption(msg_types,"Aftertouch",	  0,  PREF_MONITOR_AFTERTOUCH);
	new configOption(msg_types,"Pitch Bend",	  0,  PREF_MONITOR_PITCHBEND);
	new configOption(msg_types,"Other CCs", 	  0,  PREF_MONITOR_CCS);
	new configOption(msg_types,"Everything Else", 0,  PREF_MONITOR_EVERYTHING_ELSE);

	new configOption(ftp_specific,"ParsePatches", 	  0, PREF_MONITOR_PARSE_FTP_PATCHES);
	new configOption(ftp_specific,"Note Info", 		  0, PREF_MONITOR_FTP_NOTE_INFO);
	new configOption(ftp_specific,"Tuning Msgs",	  0, PREF_MONITOR_FTP_TUNING_MSGS);
	new configOption(ftp_specific,"Commands", 		  0, PREF_MONITOR_FTP_COMMANDS);
	new configOption(ftp_specific,"Values", 		  0, PREF_MONITOR_FTP_VALUES);
	new configOption(ftp_specific,"Poly Mode", 		  0, PREF_MONITOR_FTP_POLY_MODE);
	new configOption(ftp_specific,"Bend Mode", 		  0, PREF_MONITOR_FTP_BEND_MODE);
	new configOption(ftp_specific,"Volume", 		  0, PREF_MONITOR_FTP_VOLUME);
	new configOption(ftp_specific,"Battery", 		  0, PREF_MONITOR_FTP_BATTERY);
	new configOption(ftp_specific,"Sensitivity",	  0, PREF_MONITOR_FTP_SENSITIVITY);
	new configOption(ftp_specific,"Known Commands",   0, PREF_MONITOR_KNOWN_FTP_COMMANDS);
	new configOption(ftp_specific,"Unknown Commands", 0, PREF_MONITOR_UNKNOWN_FTP_COMMANDS);

	// performance filter

	configOption *filter = new configOption(rootOption,"Perf Filter", 0, PREF_PERF_FILTER);
	new configOption(filter,"Perf Filter",  0, PREF_PERF_FILTER);
	new configOption(filter,"Filter Bends", 0, PREF_PERF_FILTER_BENDS);
	new configOption(filter,"Monitor Perf", 0, PREF_MONITOR_PERFORMANCE);

	// all other preferences

	configOption *system = new configOption(rootOption,"System " TEENSY_EXPRESSION_VERSION);
	new configOption(system,"Debug Port",	OPTION_TYPE_NEEDS_REBOOT,	PREF_DEBUG_PORT);
	new configOption(system,"File Sys Port",	OPTION_TYPE_NEEDS_REBOOT,	PREF_FILE_SYSTEM_PORT);
	new configOption(system,"Calibrate Touch");

	new configOption(rootOption,"Factory Reset",OPTION_TYPE_FACTORY_RESET);
}


void configSystem::clearOptionStates(configOption *option)
{
	if (option == rootOption)
		display(0,"clearOptionStates",0);
	option->clearSelected();
	option->clearDisplayValue();
	configOption *child = option->pFirstChild;
	while (child)
	{
		clearOptionStates(child);
		child = child->pNextOption;
	}
}


//---------------------------------
// ctor and functions
//---------------------------------

configSystem::configSystem()
	: expWindow(WIN_FLAG_OWNER_TITLE)
{
}



// virtual
void configSystem::begin(bool warm)
	// called "cold" when coming from rig_looper,
	// "warm" when returning from child/dialog
{
    display(0,"configSystem::begin(%d)",warm);
    expWindow::begin(warm);

	if (!rootOption)
	{
		m_dirty = 0;
		createOptions();
		rootOption->init();
		display(0,"options created",0);
	}

	if (!warm)
	{
		clearOptionStates(rootOption);
		cur_menu = rootOption->pFirstChild;
		cur_option = rootOption->pFirstChild;
		cur_option->selected = 1;
		m_scroll_top = 0;
	}

	display_menu = 0;
	display_option = 0;
	m_last_display_option = 0;

    // setup buttons and leds

    theButtons.setButtonType(BUTTON_BRIGHTNESS_DOWN, BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT, LED_RED);
    theButtons.setButtonType(BUTTON_BRIGHTNESS_UP,   BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT, LED_GREEN);

	int done_color = m_dirty ? configOption::reboot_needed() ?
		LED_RED : LED_PURPLE : LED_CYAN;

    theButtons.setButtonType(BUTTON_EXIT_DONE,       BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK, done_color);
    theButtons.setButtonType(BUTTON_EXIT_CANCEL,     BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK, m_dirty?LED_ORANGE:LED_YELLOW);

	theButtons.setButtonType(BUTTON_MOVE_UP,   	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT);
	theButtons.setButtonType(BUTTON_MOVE_DOWN,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT);
	theButtons.setButtonType(BUTTON_MOVE_LEFT,	BUTTON_EVENT_PRESS);
	theButtons.setButtonType(BUTTON_MOVE_RIGHT,	BUTTON_EVENT_PRESS);
	theButtons.setButtonType(BUTTON_SELECT,	    BUTTON_EVENT_CLICK, 	LED_GREEN);

    showLEDs();

	display(0,"configSystem::begin() finished",0);

}   // configSystem::begin



void reboot(int num)
	// general purpose static reboot method
{
    if (dbgSerial == &Serial)
        Serial.end();
    else
        SERIAL_DEVICE.end();

    for (int i=0; i<21; i++)
    {
        setLED(num,i & 1 ? LED_RED : 0);
        showLEDs();
        delay(80);
    }
    // REBOOT
    #define RESTART_ADDR 0xE000ED0C
    #define READ_RESTART() (*(volatile uint32_t *)RESTART_ADDR)
    #define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))
    WRITE_RESTART(0x5FA0004);
}


// virtual
void configSystem::onEndModal(expWindow *win, uint32_t param)
	// called when modal dialogs are ended
{
	if (param && win->getId() == OPTION_TYPE_FACTORY_RESET)
	{
		clear_prefs();
		reboot(THE_SYSTEM_BUTTON);
	}
}


void configSystem::checkDirty()
{
	display(dbg_csys+1,"checkDirty()",0);
	bool dirty = prefs_changed();
	display(dbg_csys+1,"m_dirty=%d dirty=%d",m_dirty,dirty);
	if (m_dirty != dirty)
	{
		m_dirty = dirty;
		display(dbg_csys,"confg_sys.m_dirty changed to %d",dirty);

		int done_color = m_dirty ? configOption::reboot_needed() ?
			LED_RED : LED_PURPLE : LED_CYAN;
		theButtons.setButtonColor(BUTTON_EXIT_DONE,done_color);
		theButtons.setButtonColor(BUTTON_EXIT_CANCEL,m_dirty?LED_ORANGE:LED_YELLOW);
	}
}



//---------------------------------------------
// BUTTONS
//---------------------------------------------

// virtual
void configSystem::onButtonEvent(int row, int col, int event)
{
    // display(0,"configSystem(%d,%d) event(%s)",row,col,buttonArray::buttonEventName(event));

    int num = row * NUM_BUTTON_COLS + col;

    if (num == BUTTON_MOVE_UP)
    {
        if (cur_option->pPrevOption)
        {
            cur_option->selected = 0;
            cur_option = cur_option->pPrevOption;
            cur_option->selected = 1;
        }
    }
    else if (num == BUTTON_MOVE_DOWN)
    {
        if (cur_option->pNextOption)
        {
            cur_option->selected = 0;
            cur_option = cur_option->pNextOption;
            cur_option->selected = 1;
        }
    }
    else if (num == BUTTON_MOVE_LEFT)
    {
        configOption *option = cur_option->pParent;
        if (option != rootOption)
        {
            cur_option->selected = 0;
            cur_option->display_selected = -1;

            display_option = 0;
            cur_menu = option->pParent->pFirstChild;
            cur_option = option;
        }
    }

    // exit / cancel

    else if (num == BUTTON_EXIT_CANCEL)  // abort - don't bother with colors
    {
        if (event == BUTTON_EVENT_LONG_CLICK)
        {
            reboot(num);
        }
        else
        {
			restore_prefs();
			checkDirty();

			// re-init things that might have changed
            // setLEDBrightness(optBrightness.orig_value);
			// value on config options is superflous ?!?

            setLEDBrightness(getPref8(PREF_BRIGHTNESS));

			// whereas there may be other ways to change the rig number
			// so it *might* not have been saved since last time ...

            theSystem.activateRig(&rig_looper);
        }
    }
    else if (num == BUTTON_EXIT_DONE)
    {
        if (event == BUTTON_EVENT_LONG_CLICK)
        {
			delay(200);	// delay needed to let pixels finish redrawing
			bool reboot_needed = configOption::reboot_needed();
			pref_changed8(PREF_SPOOF_FTP);
			save_global_prefs();
			checkDirty();

            if (reboot_needed)
            {
                reboot(num);
            }
        }

        theSystem.activateRig(&rig_looper);
    }

	// do something

    else if (cur_option->isEnabled())
	{
		if (num == BUTTON_SELECT)
		{
			// normal behavior - go into child menu,
			// increment the value, call a dialog window,
			// or do some special function

			if (cur_option->pFirstChild)
			{
				display_option = 0;
				cur_menu = cur_option->pFirstChild;
				cur_option = cur_option->pFirstChild;
				cur_option->selected = 1;
			}
			else if (cur_option->hasValue())
			{
				cur_option->incValue(1);
				checkDirty();
			}
			else if (cur_option->m_setter_fxn)
			{
				// display(0,"calling dialog function on %d. %s",cur_option->getNum(), cur_option->getTitle());
				(cur_option->m_setter_fxn)(cur_option->getNum());
				checkDirty();
			}
			else if (cur_option->type & OPTION_TYPE_FACTORY_RESET)
			{
				theSystem.startModal(new yesNoDialog(
					OPTION_TYPE_FACTORY_RESET,
					"Confirm Factory Reset",
					"Are you sure you want to do a\nfactory reset?"));
			}
		}

		else if (num == BUTTON_BRIGHTNESS_UP ||
				 num == BUTTON_BRIGHTNESS_DOWN)
		{
			int inc = num == BUTTON_BRIGHTNESS_UP ? 5 : -5;
			optBrightness->setValue(getPref8(PREF_BRIGHTNESS) + inc);
			checkDirty();
		}

	}	// enabled
}


//---------------------------------------------
// DRAW
//---------------------------------------------

#define LINE_HEIGHT     45
#define TOP_OFFSET      50
#define TEXT_OFFSET     10
#define HIGHLIGHT_OFFSET 3

#define LEFT_OFFSET     20
#define RIGHT_OFFSET    20

#define NUMBER_WIDTH    120
#define MID_OFFSET      (TFT_WIDTH/2)


#define OPTIONS_PER_PAGE   6


void configSystem::updateUI()
{
    bool draw_all = false;

	if (cur_option != m_last_display_option)
	{
		m_last_display_option = cur_option;
		int  option_num = cur_option->getNum();
		// display(0,"m_scroll_top=%d option_num=%d",m_scroll_top,option_num);
		int scroll_top = m_scroll_top;
		if (option_num < scroll_top)
			scroll_top = option_num;
		else if (option_num >= scroll_top + OPTIONS_PER_PAGE)
			scroll_top = option_num - OPTIONS_PER_PAGE + 1;
		if (scroll_top != m_scroll_top)
		{
			mylcd.fillRect(0,TOP_OFFSET,TFT_WIDTH,TFT_HEIGHT-TOP_OFFSET,0);
			m_scroll_top = scroll_top;
			draw_all = true;
			//display(0,"new m_scroll_top=%d",m_scroll_top);
		}
	}

    // title

    if (display_menu != cur_menu)
    {
        display_menu = cur_menu;
        draw_all = true;

        mylcd.fillScreen(0);

        if (cur_option->pParent == rootOption)
            theSystem.setTitle(name());
        else
        {
            configOption *opt = cur_option->pParent;
            theSystem.setTitle(opt->getTitle());
        }
    }

    int y = TOP_OFFSET;
    configOption *opt = cur_menu;
    mylcd.setFont(Arial_20);

    while (opt)
    {
		int num = opt->getNum();
		if (num >= m_scroll_top && num < m_scroll_top + OPTIONS_PER_PAGE)
		{
			bool draw_value = opt->needsValueDisplay();
			bool enabled = opt->isEnabled();

			bool draw_selected =
				opt->display_selected != opt->selected ||
				opt->display_enabled != enabled;

			opt->display_enabled = enabled;
			opt->display_selected = opt->selected;
			opt->clearDisplayValue();

			if (draw_all || draw_selected)
			{
				int color = TFT_BLACK;
				if (opt->selected)
					color = TFT_BLUE;

				// don't need to draw black on a full redraw

				if (color != TFT_BLACK || !draw_all)
					mylcd.fillRect(0,y,TFT_WIDTH,LINE_HEIGHT-HIGHLIGHT_OFFSET,color);

				uint16_t fc = enabled ? TFT_YELLOW : TFT_DARKGREY;

				mylcd.setTextColor(fc);
				mylcd.setCursor(LEFT_OFFSET,y + TEXT_OFFSET);
				mylcd.print(num+1,DEC);
				mylcd.print(". ");
				mylcd.print(opt->getTitle());
			}

			if (opt->m_pref_num >= 0 && (
				draw_all || draw_selected || draw_value))
			{
				uint16_t fc = enabled ? TFT_WHITE : TFT_DARKGREY;

				mylcd.printfJustified(
					MID_OFFSET,
					y + TEXT_OFFSET,
					MID_OFFSET - RIGHT_OFFSET,
					LINE_HEIGHT - TEXT_OFFSET - HIGHLIGHT_OFFSET,
					LCD_JUST_RIGHT,
					fc,
					opt->selected ? TFT_BLUE : TFT_BLACK,
					"%s",
					opt->getValueString());
			}
	        y += LINE_HEIGHT;
		}

        opt = opt->pNextOption;
    }

}	// updateUI()
