
#include <EEPROM.h>
#include <myDebug.h>
#include "configSystem.h"
#include "defines.h"
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

configOption     rootOption;
brightnessOption optBrightness(&rootOption);
patchNumOption  optPatchNum(&rootOption);
configOption     optPedals(&rootOption,"Pedals",OPTION_TYPE_MENU);
configOption     optSystem(&rootOption,"System",OPTION_TYPE_MENU);
spoofFTPOption   optSpoofFTP(&rootOption);

configOption     factorReset(&rootOption,"Factory Reset",  OPTION_TYPE_IMMEDIATE | OPTION_TYPE_FACTORY_RESET);
configOption     testOption2(&rootOption,"Test2",  OPTION_TYPE_TERMINAL);
configOption     testOption3(&rootOption,"Test3",  OPTION_TYPE_TERMINAL);
configOption     testOption4(&rootOption,"Test4",  OPTION_TYPE_TERMINAL);


configOption     optCalibPedals(&optPedals,"Calibrate Pedals",OPTION_TYPE_MENU);
configOption     optConfigPedals(&optPedals,"Configure Pedals",OPTION_TYPE_MENU);

configOption     optCalibPedal1(&optCalibPedals,"Calibrate Pedal1 (Synth)",  OPTION_TYPE_TERMINAL);
configOption     optCalibPedal2(&optCalibPedals,"Calibrate Pedal2 (Loop)",   OPTION_TYPE_TERMINAL);
configOption     optCalibPedal3(&optCalibPedals,"Calibrate Pedal3 (Wah)",    OPTION_TYPE_TERMINAL);
configOption     optCalibPedal4(&optCalibPedals,"Calibrate Pedal4 (Guitar)", OPTION_TYPE_TERMINAL);

configOption     optConfigPedal1(&optConfigPedals,"Configure Pedal1 (Synth)",  OPTION_TYPE_TERMINAL);
configOption     optConfigPedal2(&optConfigPedals,"Configure Pedal2 (Loop)",   OPTION_TYPE_TERMINAL);
configOption     optConfigPedal3(&optConfigPedals,"Configure Pedal3 (Wah)",    OPTION_TYPE_TERMINAL);
configOption     optConfigPedal4(&optConfigPedals,"Configure Pedal4 (Guitar)", OPTION_TYPE_TERMINAL);


configOption     optCalibrateTouch(&optSystem,"Calibrate Touch",OPTION_TYPE_TERMINAL);



configOption *cur_menu = 0;
configOption *cur_option = 0;
configOption *display_menu = 0;
configOption *display_option = 0;
bool in_terminal_mode = false;



void reboot(int num)
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



configSystem::configSystem()
	: expWindow(WIN_FLAG_OWNER_TITLE)
{
}


bool config_changed()
{
    return
        optBrightness.getValue() != optBrightness.getOrigValue() ||
        optPatchNum.getValue() != optPatchNum.getOrigValue() ||
		optSpoofFTP.getValue() != optSpoofFTP.getOrigValue();
}


void configSystem::notifyTerminalModeEnd()
    // called by terminal nodes to end their sessions
{
    in_terminal_mode = 0;
    display_menu = 0;
    display_option = 0;
    // reset up and down keys so they don't repeat
	theButtons.setButtonType(BUTTON_MOVE_UP,   	BUTTON_EVENT_PRESS);
	theButtons.setButtonType(BUTTON_MOVE_DOWN,	BUTTON_EVENT_PRESS);
}


// virtual
void configSystem::begin(bool warm)
{
    display(0,"configSystem::begin(%d)",warm);
    expWindow::begin(warm);

    // setup option terminal nodes
    // calls init() on entire tree

	if (!warm)
	{
		rootOption.init(this);

		// initialize globals

		cur_menu = rootOption.pFirstChild;
		cur_option = rootOption.pFirstChild;
		cur_option->selected = 1;
		in_terminal_mode = false;
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

    if (optPatchNum.value && optPatchNum.value<=MAX_SHOWN_PATCHES)
        theButtons.select(FIRST_PATCH_BUTTON+optPatchNum.value-1,1);

    // finished
    // do not call draw() here!
    // only draw on the main thread ..

    showLEDs();

}   // configSystem::begin




// virtual
void configSystem::onEndModal(expWindow *win, uint32_t param)
{
	if (param && win->getId() == OPTION_TYPE_FACTORY_RESET)
	{
		for (int i=0; i<NUM_EEPROM_USED; i++)
			EEPROM.write(i,255);
		reboot(BUTTON_SELECT);
	}
}




// virtual
void configSystem::onButtonEvent(int row, int col, int event)
{
    // display(0,"configSystem(%d,%d) event(%s)",row,col,buttonArray::buttonEventName(event));
    int num = row * NUM_BUTTON_COLS + col;

    if (in_terminal_mode)
    {
        cur_option->terminalNav(num);
        return;
    }

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
        if (option != &rootOption)
        {
            cur_option->selected = 0;
            cur_option->display_selected = -1;

            display_option = 0;
            cur_menu = option->pParent->pFirstChild;
            cur_option = option;
        }
    }
    else if (num == BUTTON_SELECT)
    {
        if (cur_option->pFirstChild)
        {
            display_option = 0;
            cur_menu = cur_option->pFirstChild;
            cur_option = cur_option->pFirstChild;
            cur_option->selected = 1;
        }
        else if (cur_option->type & OPTION_TYPE_IMMEDIATE)
        {
            cur_option->incValue(1);

            if (cur_option->type & OPTION_TYPE_CONFIG_NUM)
			{
                if (optPatchNum.value &&
					optPatchNum.value <= MAX_SHOWN_PATCHES)
				{
					theButtons.select(FIRST_PATCH_BUTTON+optPatchNum.value-1,1);
				}
				else
				{
					theButtons.clearRadioGroup(GROUP_PATCH_NUMS);
				}

                showLEDs();
            }
			else if (cur_option->type & OPTION_TYPE_FACTORY_RESET)
			{
				theSystem.startModal(new yesNoDialog(
					OPTION_TYPE_FACTORY_RESET,
					"Confirm Factory Reset",
					"Are you sure you want to do a\nfactory reset?"));
			}
        }
        else if (cur_option->type & OPTION_TYPE_TERMINAL)
        {
            // have the up and down keys start repeating for default terminal mode editors
            theButtons.setButtonType(BUTTON_MOVE_UP,   	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT );
            theButtons.setButtonType(BUTTON_MOVE_DOWN,	BUTTON_EVENT_PRESS | BUTTON_MASK_REPEAT );
            in_terminal_mode = cur_option->beginTerminalMode();
        }
    }

    else if (num == BUTTON_BRIGHTNESS_UP ||
             num == BUTTON_BRIGHTNESS_DOWN)
    {
        int inc = num == BUTTON_BRIGHTNESS_UP ? 5 : -5;
        optBrightness.setValue(optBrightness.value + inc);
}
    else if (row == ROW_CONFIGS)
    {
        optPatchNum.setValue(col + 1);
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
            setLEDBrightness(optBrightness.orig_value);
            theSystem.activatePatch(optPatchNum.orig_value);
        }
    }
    else if (num == BUTTON_EXIT_DONE)
    {
        if (event == BUTTON_EVENT_LONG_CLICK)
        {
            display(0,"write EEPROM bright=%d config=%d spoof_ftp=%d",
                optBrightness.value,
                optPatchNum.value,
				optSpoofFTP.value);

            EEPROM.write(PREF_BRIGHTNESS,optBrightness.value);
            EEPROM.write(PREF_PATCH_NUM,optPatchNum.value);
            EEPROM.write(PREF_SPOOF_FTP,optSpoofFTP.value);

            if (optSpoofFTP.value != optSpoofFTP.orig_value)
            {
                reboot(num);
            }
        }

        theSystem.activatePatch(optPatchNum.value);
    }

}







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
    if (in_terminal_mode)
    {
        cur_option->terminalDraw();
        return;
    }

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

        if (cur_option->pParent == &rootOption)
            theSystem.setTitle(theSystem.getCurPatch()->name());
        else
        {
            configOption *opt = cur_option->pParent;
            theSystem.setTitle(opt->title);
        }
    }

    mylcd.setFont(Arial_20);

    // int num = 0;
    int y = TOP_OFFSET;
    configOption *opt = cur_menu;

    while (opt)
    {
        // num++;

		int num = opt->getNum();
		if (num >= m_scroll_top && num < m_scroll_top + OPTIONS_PER_PAGE)
		{
			bool draw_selected = opt->display_selected != opt->selected;
			bool draw_value = opt->display_value != opt->value;
			opt->display_selected = opt->selected;
			opt->display_value = opt->value;

			if (draw_all || draw_selected)
			{
				int color = TFT_BLACK;
				if (opt->selected)
					color = TFT_BLUE;

				// don't need to draw black on a full redraw

				if (color != TFT_BLACK || !draw_all)
					mylcd.Fill_Rect(0,y,TFT_WIDTH,LINE_HEIGHT-HIGHLIGHT_OFFSET,color);

				mylcd.Set_Text_colour(TFT_YELLOW);
				mylcd.Set_Text_Cursor(LEFT_OFFSET,y + TEXT_OFFSET);
				mylcd.print(num+1,DEC);
				mylcd.print(". ");
				mylcd.print(opt->title);
			}

			if (opt->type & OPTION_TYPE_VALUE && (
				draw_all || draw_selected || draw_value))
			{
				mylcd.printf_justified(
					MID_OFFSET,
					y + TEXT_OFFSET,
					MID_OFFSET - RIGHT_OFFSET,
					LINE_HEIGHT - TEXT_OFFSET - HIGHLIGHT_OFFSET,
					LCD_JUST_RIGHT,
					TFT_WHITE,
					opt->selected ? TFT_BLUE : TFT_BLACK,
					"%s",
					opt->getValueString());
			}
	        y += LINE_HEIGHT;
		}

        opt = opt->pNextOption;
    }

}
