
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

#define BUTTON_BRIGHTNESS_DOWN  0
#define BUTTON_BRIGHTNESS_UP    1
#define BUTTON_EXIT_CANCEL      3
#define BUTTON_EXIT_DONE        4

#define ROW_CONFIGS             1
#define FIRST_PATCH_BUTTON     (ROW_CONFIGS * NUM_BUTTON_COLS)
#define MAX_SHOWN_PATCHES		5

#define GROUP_PATCH_NUMS  		1

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
	{ theSystem.startModal(theSystem.getFtpTuner()); }
void startFtpSensitivity(int i)
	{ theSystem.startModal(theSystem.getFtpSensitivity()); }
void configPedal(int i)
	{ theSystem.startModal(new winConfigPedal(i)); }

ENABLED_CONFIG(ftpPortOption,!getPref8(PREF_SPOOF_FTP));



//---------------------------------
// pseudo static initialization
//---------------------------------

void createOptions()
{
	if (rootOption == 0)
	{
		rootOption = new configOption();

		optBrightness = new configOption(
			rootOption,
			"Brightness",
			0,
			PREF_BRIGHTNESS,
			setLEDBrightness);

		new configOption(rootOption,"Patch",		OPTION_TYPE_CONFIG_NUM,		PREF_PATCH_NUM);

//      FTP
//          Spoof FTP       ON/OFF
//          FTP Port        None, Host, Remote
//          Tuner           -> modal window
//          Sensitivity     -> modal window

		configOption *optFTP = new configOption(rootOption,"FTP");
		new configOption(optFTP,"Spoof FTP",	OPTION_TYPE_NEEDS_REBOOT,	PREF_SPOOF_FTP);
		new ftpPortOption(optFTP,"FTP Port",	0,							PREF_FTP_PORT);
		new configOption(optFTP,"FTP Tuner",	0,							PREF_NONE,		startFtpTuner);
		new configOption(optFTP,"FTP Sensitivity",0,						PREF_NONE,		startFtpSensitivity);

		configOption *pedals = new configOption(rootOption,"Pedals");
		new configOption(pedals,"Configure Pedal1 (Synth)",	0,PREF_NONE,configPedal);
		new configOption(pedals,"Configure Pedal2 (Loop)",	0,PREF_NONE,configPedal);
		new configOption(pedals,"Configure Pedal3 (Wah)",	0,PREF_NONE,configPedal);
		new configOption(pedals,"Configure Pedal4 (Guitar)",	0,PREF_NONE,configPedal);

		configOption *system = new configOption(rootOption,"System");
		new configOption(system,"Calibrate Touch");

		new configOption(rootOption,"Debug Port",	OPTION_TYPE_NEEDS_REBOOT,	PREF_DEBUG_PORT);
		new configOption(rootOption,"Factory Reset",OPTION_TYPE_FACTORY_RESET);

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
{
    display(0,"configSystem::begin(%d)",warm);
    expWindow::begin(warm);
	createOptions();
	display(0,"options created",0);

	if (!warm)
	{
		rootOption->init();
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
    theButtons.setButtonType(BUTTON_EXIT_DONE,       BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK, LED_PURPLE);
    theButtons.setButtonType(BUTTON_EXIT_CANCEL,     BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK, LED_ORANGE);

	theButtons.setButtonType(BUTTON_MOVE_UP,   	BUTTON_EVENT_PRESS);
	theButtons.setButtonType(BUTTON_MOVE_DOWN,	BUTTON_EVENT_PRESS);
	theButtons.setButtonType(BUTTON_MOVE_LEFT,	BUTTON_EVENT_PRESS);
	theButtons.setButtonType(BUTTON_MOVE_RIGHT,	BUTTON_EVENT_PRESS);
	theButtons.setButtonType(BUTTON_SELECT,	    BUTTON_EVENT_CLICK, 	LED_GREEN);

    // setup the patch_num button row

    int num_show = theSystem.getNumPatches()-1;
    if (num_show >= MAX_SHOWN_PATCHES) num_show = MAX_SHOWN_PATCHES;
    for (int i=0; i<num_show; i++)
        theButtons.setButtonType(FIRST_PATCH_BUTTON+i,BUTTON_TYPE_RADIO(GROUP_PATCH_NUMS));

	int patch_num = getPref8(PREF_PATCH_NUM);
    if (patch_num && patch_num<=MAX_SHOWN_PATCHES)
        theButtons.select(FIRST_PATCH_BUTTON+patch_num-1,1);

    // finished
    // do not call draw() here!
    // only draw on the main thread ..

    showLEDs();

	display(0,"configSystem::begin() finished",0);

}   // configSystem::begin



void reboot(int num)
	// general purpose static reboot method
{
    if (dbgSerial == &Serial)
        Serial.end();
    else
        Serial3.end();

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
		for (int i=0; i<NUM_EEPROM_USED; i++)
			setPref8(i,255);
		save_global_prefs();
		reboot(THE_SYSTEM_BUTTON);
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

			// re-init things that might have changed
            // setLEDBrightness(optBrightness.orig_value);
			// value on config options is superflous ?!?

            setLEDBrightness(getPref8(PREF_BRIGHTNESS));

			// whereas there may be other ways to change the patch number
			// so it *might* not have been saved since last time ...

            theSystem.activatePatch(theSystem.getPrevConfigNum());

        }
    }
    else if (num == BUTTON_EXIT_DONE)
    {
        if (event == BUTTON_EVENT_LONG_CLICK)
        {
			bool reboot_needed = configOption::reboot_needed();
			pref_changed8(PREF_SPOOF_FTP);
			save_global_prefs();
            if (reboot_needed)
            {
                reboot(num);
            }
        }

        theSystem.activatePatch(getPref8(PREF_PATCH_NUM));
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

				// highlight the patch quick key

				if (cur_option->type & OPTION_TYPE_CONFIG_NUM)
				{
					int value = getPref8(PREF_PATCH_NUM);
					if (value && value <= MAX_SHOWN_PATCHES)
						theButtons.select(FIRST_PATCH_BUTTON+value-1,1);
					else
						theButtons.clearRadioGroup(GROUP_PATCH_NUMS);
					showLEDs();
				}

			}
			else if (cur_option->m_setter_fxn)
			{
				// display(0,"calling dialog function on %d. %s",cur_option->getNum(), cur_option->getTitle());
				(cur_option->m_setter_fxn)(cur_option->getNum());
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
		}

		else if (row == ROW_CONFIGS)
		{
			setPref8(PREF_PATCH_NUM,col + 1);
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
		//display(0,"m_scroll_top=%d option_num=%d",m_scroll_top,option_num);
		int scroll_top = m_scroll_top;
		if (option_num < scroll_top)
			scroll_top = option_num;
		else if (option_num >= scroll_top + OPTIONS_PER_PAGE)
			scroll_top = option_num - OPTIONS_PER_PAGE + 1;
		if (scroll_top != m_scroll_top)
		{
			mylcd.Fill_Rect(0,TOP_OFFSET,TFT_WIDTH,TFT_HEIGHT-TOP_OFFSET,0);
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

        mylcd.Fill_Screen(0);

        if (cur_option->pParent == rootOption)
            theSystem.setTitle(theSystem.getCurPatch()->name());
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
					mylcd.Fill_Rect(0,y,TFT_WIDTH,LINE_HEIGHT-HIGHLIGHT_OFFSET,color);

				uint16_t fc = enabled ? TFT_YELLOW : TFT_DARKGREY;

				mylcd.Set_Text_colour(fc);
				mylcd.Set_Text_Cursor(LEFT_OFFSET,y + TEXT_OFFSET);
				mylcd.print(num+1,DEC);
				mylcd.print(". ");
				mylcd.print(opt->getTitle());
			}

			if (opt->m_pref_num >= 0 && (
				draw_all || draw_selected || draw_value))
			{
				uint16_t fc = enabled ? TFT_WHITE : TFT_DARKGREY;

				mylcd.printf_justified(
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
