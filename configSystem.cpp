
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


#define BUTTON_BRIGHTNESS_DOWN  0
#define BUTTON_BRIGHTNESS_UP    1
#define BUTTON_EXIT_CANCEL      3
#define BUTTON_EXIT_DONE        4

#define ROW_CONFIGS             1
#define FIRST_PATCH_BUTTON     (ROW_CONFIGS * NUM_BUTTON_COLS)



configOption     rootOption;
brightnessOption optBrightness(&rootOption);
patchNumOption  optPatchNum(&rootOption);
configOption     optPedals(&rootOption,"Pedals",OPTION_TYPE_MENU);
configOption     optSystem(&rootOption,"System",OPTION_TYPE_MENU);
spoofFTPOption   optSpoofFTP(&rootOption);


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


midiHostOption   optMidiHost(&optSystem);
serialPortOption optSerialPort(&optSystem);
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
{
}


bool config_changed()
{
    return
        optBrightness.getValue() != optBrightness.getOrigValue() ||
        optPatchNum.getValue() != optPatchNum.getOrigValue() ||
        optMidiHost.getValue() != optMidiHost.getOrigValue() ||
        optSerialPort.getValue() != optSerialPort.getOrigValue();
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
void configSystem::begin()
{
    display(0,"configSystem::begin()",0);
    expWindow::begin();

    // setup option terminal nodes
    // calls init() on entire tree
    
    rootOption.init(this);      

    // initialize globals
    
    cur_menu = rootOption.pFirstChild;
    cur_option = rootOption.pFirstChild;
    cur_option->selected = 1;
    display_menu = 0;
    display_option = 0;
    in_terminal_mode = false;

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
    if (num_show >= MAX_EXP_PATCHES) num_show = MAX_EXP_PATCHES;
    for (int i=0; i<num_show; i++)
        theButtons.setButtonType(FIRST_PATCH_BUTTON+i,BUTTON_TYPE_RADIO(1));
        
    if (optPatchNum.value && optPatchNum.value<=MAX_EXP_PATCHES)
        theButtons.select(FIRST_PATCH_BUTTON+optPatchNum.value-1,1);
        
    // finished
    // do not call draw() here!
    // only draw on the main thread ..
    
    showLEDs();
    
}   // configSystem::begin



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
            
            if ((cur_option->type & OPTION_TYPE_CONFIG_NUM) &&
                optPatchNum.value &&
                optPatchNum.value <= 5)
            {
                theButtons.select(FIRST_PATCH_BUTTON+optPatchNum.value-1,1);
                showLEDs();
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
            display(0,"write bright=%d and config=%d to EEPROM",
                optBrightness.value,
                optPatchNum.value);
  
            EEPROM.write(EEPROM_BRIGHTNESS,optBrightness.value);
            EEPROM.write(EEPROM_PATCH_NUM,optPatchNum.value);
            EEPROM.write(EEPROM_MIDI_HOST,optMidiHost.value);
            EEPROM.write(EEPROM_SERIAL_PORT,optSerialPort.value);
            EEPROM.write(EEPROM_SPOOF_FTP,optSpoofFTP.value);
            
            if ((optMidiHost.value != optMidiHost.orig_value) ||
                (optSerialPort.value != optSerialPort.orig_value) ||
                (optSpoofFTP.value != optSpoofFTP.orig_value))
            {
                reboot(num);                
            }
        }

        // you cannot change these at run time!!!
        //
        // midi_host_on = optMidiHost.value;
        // serial_port_on = optSerialPort.value;
        
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


void configSystem::updateUI()

{
    if (in_terminal_mode)
    {
        cur_option->terminalDraw();
        return;
    }
    
    bool draw_all = false;
    
    // title
    
    if (display_menu != cur_menu)
    {
        display_menu = cur_menu;
        draw_all = true;
        
        mylcd.setFont(Arial_16_Bold);
        mylcd.Set_Text_Cursor(10,10);
        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.Set_Draw_color(TFT_YELLOW);
        mylcd.Fill_Rect(0,0,TFT_WIDTH,TFT_HEIGHT,0);
        
        if (cur_option->pParent == &rootOption)
            mylcd.print( theSystem.getCurPatch()->name());
        else
        {
            configOption *opt = cur_option->pParent;
            mylcd.print(opt->title);
        }

	    mylcd.Draw_Line(0,36,TFT_WIDTH-1,36);
    }

    mylcd.setFont(Arial_20);

    int num = 0;
    int y = TOP_OFFSET;
    configOption *opt = cur_menu;
    
    while (opt)
    {
        num++;
        
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
            mylcd.print(num,DEC);
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

        opt = opt->pNextOption;
        y += LINE_HEIGHT;
    }
}



