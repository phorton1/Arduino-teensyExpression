#include "configOptions.h"
#include <myDebug.h>
#include "myTFT.h"
#include "myLeds.h"
#include "myMidiHost.h"
#include <EEPROM.h>



//--------------------------------------------
// configOption
//--------------------------------------------

configOption::configOption()
{
    init_cold(0,"",0,0,0);
}

configOption::configOption(configOption *parent, const char *title, int type)
{
    init_cold(parent,title,type,0,0);
}

configOption::configOption(configOption *parent, const char *title, int type, int min, int max)
{
    init_cold(parent,title,type,min,max);
}



void configOption::init_cold(configOption *parent, const char *tit, int typ, int min, int max)
{
    title            = tit;
    type             = typ;
    pParent          = parent;
    min_value        = min;
    max_value        = max;

    m_pSysConfig     = 0;
    
    value            = 0;
    orig_value       = 0;
    display_value    = -1;
    selected         = 0;
    display_selected = 0;

    draw_needed      = 0;
    redraw_needed    = 0;
    
    pPrevOption = 0;
    if (parent)
        pPrevOption = parent->pLastChild;
    if (pPrevOption)
        pPrevOption->pNextOption = this;
    if (parent && !parent->pFirstChild)
        parent->pFirstChild = this;
    if (parent)
        parent->pLastChild = this;
        
}



// not virutal!
void configOption::init(systemConfig *sysConfig)
    // non-virtual entry point for root node
{
    m_pSysConfig = sysConfig;
    init();
}
        

// virtual
void configOption::init()
    // called recursively during begin() on subclasses that
    // are linked to other objects to initialize the value, etc
{
    value            = 0;
    orig_value       = 0;
    display_value    = -1;
    selected         = 0;
    display_selected = 0;
    
    // terminal mode initialization
    
    draw_needed      = 0;
    redraw_needed    = 0;
    
    // recurse thru children
    
    configOption *opt = pFirstChild;
    while (opt) { opt->init(); opt=opt->pNextOption; }
}




// virtual
void  configOption::setValue(int i)   
{
    value = i;  // enforces min/max
    if (value > max_value) value = max_value;
    if (value < min_value) value = min_value;
}


// virtual
void configOption::incValue(int inc_dec)  // wraps
{
    value += inc_dec;   // wraps thru min/max
    if (value > max_value) value = min_value;
    if (value < min_value) value = max_value;
}



//--------------------------------------------
// integerOption
//--------------------------------------------

integerOption::integerOption(configOption *parent, const char *title, int type, int min, int max) :
    configOption(parent,title,type | OPTION_TYPE_VALUE,min,max)
{}


// virtual
const char *integerOption::getValueString()
{
    sprintf(buffer,"%d",value);
    return (const char *) buffer;
}


// virtual
bool integerOption::beginTerminalMode()
{
    draw_needed = true;
    return true;
}



void integerOption::terminalNav(int num)
{
    // display(0,"integerOption::terminalNav(%d)",num);
    
    if (num == BUTTON_MOVE_LEFT)
    {
        m_pSysConfig->notifyTerminalModeEnd();
    }
    if (num == BUTTON_MOVE_UP ||
        num == BUTTON_MOVE_DOWN)
    {
        int inc = num == BUTTON_MOVE_UP ? 1 : -1;
        setValue(value + inc);
    }
    
}

void integerOption::terminalDraw()
{
    if (draw_needed)
    {
        draw_needed = false;
        redraw_needed = true;
        
        mylcd.Fill_Rect(0,0,TFT_WIDTH,TFT_HEIGHT,0);
        mylcd.setFont(Arial_32_Bold);
        mylcd.printf_justified(
            30,
            30,
            TFT_WIDTH - 60,
            80,
            LCD_JUST_CENTER,
            TFT_YELLOW,
            TFT_BLACK,
            "%s",
            title);
    }
    
    if (redraw_needed ||
        needsValueDisplay())
    {
        clearDisplayValue();
        redraw_needed = false;
        mylcd.setFont(Arial_48);
        mylcd.printf_justified(
            180,
            100,
            120,
            80,
            LCD_JUST_CENTER,
            TFT_WHITE,
            TFT_BLACK,
            "%s",
            getValueString());
    }
}



//--------------------------------------------
// brightnessOption
//--------------------------------------------

brightnessOption::brightnessOption(configOption *parent) :
    integerOption(parent,"Brightness",
        OPTION_TYPE_TERMINAL | OPTION_TYPE_BRIGHTNESS,1,100)
{}


// virtual
void brightnessOption::init()
{
    integerOption::init();
    value = getLEDBrightness(); 
    orig_value = value;
    display_value = -1;
}


// virtual
void brightnessOption::setValue(int i)
{
    integerOption::setValue(i);
    setLEDBrightness(value);
}
    


//--------------------------------------------
// configNumOption
//--------------------------------------------

configNumOption::configNumOption(configOption *parent) :
    integerOption(parent,"Config",
        OPTION_TYPE_IMMEDIATE | OPTION_TYPE_CONFIG_NUM,1,0)
{}


// virtual
void configNumOption::init()
{
    integerOption::init();
    value = theSystem.getPrevConfigNum();
    max_value = theSystem.getNumConfigs() - 1;
    orig_value = value;
    display_value = -1;
}


// virtual
const char *configNumOption::getValueString()
{
    return theSystem.getConfig(value)->short_name();
}



//-------------------------------------
// onOffOption
//-------------------------------------

onOffOption::onOffOption(configOption *parent, const char *title) :
    integerOption(parent,title,OPTION_TYPE_IMMEDIATE,0,1)
{}

// virtual
const char *onOffOption::getValueString()
{
    return value ? "ON" : "OFF";
}



//-------------------------------------
// Specific Options
//-------------------------------------

midiHostOption::midiHostOption(configOption *parent) :
    onOffOption(parent,"Midi Host") {}

// virtual
void midiHostOption::init()
{
    onOffOption::init();
    orig_value = value = midi1.isOn();
}



serialPortOption::serialPortOption(configOption *parent) :
    onOffOption(parent,"Serial Port") {}
    
// virtual
void serialPortOption::init()
{
    onOffOption::init();
    orig_value = value = serial_port_on;
}




spoofFTPOption::spoofFTPOption(configOption *parent) :
    onOffOption(parent,"Spoof FTP") {}
    
// virtual
void spoofFTPOption::init()
{
    onOffOption::init();
    uint8_t v = EEPROM.read(EEPROM_SPOOF_FTP);
    orig_value = value = (v != 255) ? v : 0;
}






