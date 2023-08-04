#include <myDebug.h>
#include "configOptions.h"
#include "prefs.h"

static bool s_reboot_needed = 0;


//--------------------------------------------
// configOption
//--------------------------------------------

configOption::configOption()
{
    init_cold(0,"",0,-1,0);
}

configOption::configOption(configOption *parent, const char *title, int type, int pref_num, setterFxn setter)
{
    init_cold(parent,title,type,pref_num,setter);
}


void configOption::init_cold(configOption *parent, const char *tit, int typ, int pref_num, setterFxn setter)
{
    title            = tit;
    type             = typ;
    pParent          = parent;
    m_pref_num       = pref_num;
    m_setter_fxn     = setter;

    display_value    = -1;
    selected         = 0;
    display_selected = -1;

    num_children     = 0;
    option_num       = 0;
    if (parent)
        option_num = parent->num_children++;

    pFirstChild = 0;
    pLastChild = 0;
    pPrevOption = 0;
    pNextOption = 0;

    if (parent)
        pPrevOption = parent->pLastChild;
    if (pPrevOption)
        pPrevOption->pNextOption = this;
    if (parent && !parent->pFirstChild)
        parent->pFirstChild = this;
    if (parent)
        parent->pLastChild = this;
}


// static
bool configOption::reboot_needed()
{
    return s_reboot_needed;
}


void configOption::init()
{
    selected         = 0;
    display_value    = -1;
    display_selected = -1;
    configOption *pChild = pFirstChild;
    while (pChild)
    {
        pChild->init();
        pChild = pChild->pNextOption;
    }
}



void  configOption::setValue(int i)
    // enforces min/max
{
    if (m_pref_num >= 0)
    {
        if (i > getPrefMax(m_pref_num)) i = getPrefMax(m_pref_num);
        if (i < getPrefMin(m_pref_num)) i = getPrefMin(m_pref_num);

        if (getPrefMax(m_pref_num) > 254)
            setPref16(m_pref_num,i);
        else
            setPref8(m_pref_num,i);
        if (m_setter_fxn)
            m_setter_fxn(i);
        if (type & OPTION_TYPE_NEEDS_REBOOT)
            s_reboot_needed = 1;
    }
}


void configOption::incValue(int inc_dec)
    // wraps thru min/max
{
    if (m_pref_num >= 0)
    {
        int i = getPrefMax(m_pref_num) > 254 ?
            getPref16(m_pref_num) :
            getPref8(m_pref_num);

        i += inc_dec;
        if (i > getPrefMax(m_pref_num)) i = getPrefMin(m_pref_num);
        if (i < getPrefMin(m_pref_num)) i = getPrefMax(m_pref_num);
        setValue(i);
    }
}



int configOption::getValue()
{
    if (m_pref_num >= 0)
        return getPrefMax(m_pref_num) > 254 ?
            getPref16(m_pref_num) :
            getPref8(m_pref_num);
    return 0;
}


const char *configOption::getValueString()
{
    if (m_pref_num >= 0)
    {
        int val = getValue();
        const char **strings = getPrefStrings(m_pref_num);
        if (strings)
            return strings[val];
        else
        {
            static char buf[12];
            sprintf(buf,"%d",val);
            return buf;
        }
    }
    return "";
}
