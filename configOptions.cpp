#include <myDebug.h>
#include "configOptions.h"
#include "prefs.h"
#include "myTFT.h"
#include "myLeds.h"
#include "myMidiHost.h"



//--------------------------------------------
// configOption
//--------------------------------------------

configOption::configOption()
{
    init_cold(0,"",0,0,0,-1);
}

configOption::configOption(configOption *parent, const char *title, int type, int pref_num)
{
    init_cold(parent,title,type,0,0,pref_num);
}

configOption::configOption(configOption *parent, const char *title, int type, int min, int max, int pref_num)
{
    init_cold(parent,title,type,min,max,pref_num);
}



void configOption::init_cold(configOption *parent, const char *tit, int typ, int min, int max, int pref_num)
{
    title            = tit;
    type             = typ;
    pParent          = parent;
    m_pref_num       = pref_num;
    min_value        = min;
    max_value        = max;

    m_pSysConfig     = 0;

    num_children     = 0;
    option_num       = 0;
    if (parent)
        option_num = parent->num_children++;

    value            = 0;
    orig_value       = 0;

    display_value    = -1;
    selected         = 0;
    display_selected = 0;
    draw_needed      = 0;
    redraw_needed    = 0;
    display_selected = -1;

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
void configOption::init(configSystem *sysConfig)
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
    if (m_pref_num > 0)
    {
        if (max_value > 254)
            value = orig_value = getPref16(m_pref_num);
        else
            value = orig_value = getPref8(m_pref_num);
    }
    else
    {
        value            = 0;
        orig_value       = 0;
    }

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
    // enforces min/max
{
    if (i > max_value) i = max_value;
    if (i < min_value) i = min_value;
    if (m_pref_num >= 0)
    {
        if (max_value > 254)
            setPref16(m_pref_num,i);
        else
            setPref8(m_pref_num,i);
    }
    value = i;
}


// virtual
void configOption::incValue(int inc_dec)
    // wraps thru min/max
{
    int i = value += inc_dec;
    if (i > max_value) i = min_value;
    if (i < min_value) i = max_value;
    if (m_pref_num >= 0)
    {
        if (max_value > 254)
            setPref16(m_pref_num,i);
        else
            setPref8(m_pref_num,i);
    }
    value = i;
}



//--------------------------------------------
// integerOption
//--------------------------------------------
// has the odd behavior of allowing you to modify
// the brightness option in a pseudo modal dialog,
// while also displaying, and reacting to the
// configuration "quick" buttons ..
//
// the use of the left arrow to go back also feels
// weird, as the green "select" button does nothing

integerOption::integerOption(configOption *parent, const char *title, int type, int min, int max, int pref_num) :
    configOption(parent,title,type | OPTION_TYPE_VALUE,min,max,pref_num)
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

    if (num == BUTTON_MOVE_LEFT ||
        num == BUTTON_SELECT)
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
            false,
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
            true,
            "%s",
            getValueString());
    }
}



//--------------------------------------------
// brightnessOption
//--------------------------------------------

brightnessOption::brightnessOption(configOption *parent) :
    integerOption(
        parent,
        "Brightness",
        OPTION_TYPE_TERMINAL | OPTION_TYPE_BRIGHTNESS,
        1,
        100,
        PREF_BRIGHTNESS)
{}


// virtual
void brightnessOption::init()
{
    integerOption::init();
    value = getLEDBrightness();
        // should be equal to the pref at any point
        // the "value" may be duplicitous
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
// patchNumOption
//--------------------------------------------

patchNumOption::patchNumOption(configOption *parent) :
    integerOption(
        parent,
        "Patch",
        OPTION_TYPE_IMMEDIATE | OPTION_TYPE_CONFIG_NUM,
        1,
        0,
        PREF_PATCH_NUM)
{}


// virtual
void patchNumOption::init()
{
    integerOption::init();
    value = theSystem.getPrevConfigNum();
    max_value = theSystem.getNumPatches() - 1;
    orig_value = value;
    display_value = -1;
}


// virtual
const char *patchNumOption::getValueString()
{
    return theSystem.getPatch(value)->short_name();
}



serialPortOption::serialPortOption(configOption *parent) :
    integerOption(
        parent,
        "DebugPort",
        OPTION_TYPE_IMMEDIATE | OPTION_TYPE_NEEDS_REBOOT,
        0,2,
        PREF_DEBUG_PORT)
{}

// virtual
const char *serialPortOption::getValueString()
{
    return
        value==2 ? "Serial" :
        value==1 ? "USB" :
        "Off";
}




//-------------------------------------
// onOffOption
//-------------------------------------

onOffOption::onOffOption(configOption *parent, const char *title, int pref_num) :
    integerOption(parent,title,OPTION_TYPE_IMMEDIATE,0,1,pref_num)
{}

// virtual
const char *onOffOption::getValueString()
{
    return value ? "ON" : "OFF";
}





spoofFTPOption::spoofFTPOption(configOption *parent) :
    onOffOption(parent,"Spoof FTP",PREF_SPOOF_FTP) {}

// virtual
void spoofFTPOption::init()
{
    onOffOption::init();
    orig_value = getPref8(PREF_SPOOF_FTP);
}


// virtual
// bool spoofFTPOption::isEnabled()
// {
//     return getLEDBrightness() > 20;
// }
