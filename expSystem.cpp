
#include <myDebug.h>
#include "defines.h"
#include "expSystem.h"
#include "systemConfig.h"
#include "myLeds.h"
#include "pedals.h"
#include "rotary.h"
#include <EEPROM.h>

#define EXP_TIMER_INTERVAL 1000
    // 1000 us = 1 ms == 1000 times per second!
#define EXP_TIMER_PRIORITY  192
    // compared to presumed (higher) priority of 128
    // for USB, midi HOST, and other (serial) interrupts


//----------------------------------------
// expConfig (base class)
//----------------------------------------


expConfig::expConfig(expSystem *pSystem)
{
    m_pSystem = pSystem;
}


// virtual
void expConfig::begin()
    // derived classes should call base class method FIRST
    // base class clears all button registrations.
{
    proc_entry();
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            m_pSystem->getRawButtonArray()->setButtonEventMask(row,col,0);
            setLED(row,col,0);
        }
    proc_leave();
}


// virtual
void expConfig::onButtonEvent(int row, int col, int event)
{
    display(0,"expConfig::onButtonEvent(%d,%d,%s) SHOULD NOT BE CALLED!!",row,col,
        rawButtonArray::buttonEventName(event));
}


// virtual
void expConfig::onRotaryEvent(int num, int val)
{
    display(0,"expConfig::onRotaryEvent(%d) val=%d",num,val);
}


// virtual
void expConfig::onPedalEvent(int num, int val)
{
    display(0,"expConfig::onRotaryEvent(%d) val=%d",num,val);
}




//----------------------------------------
// expSystem
//----------------------------------------


expSystem::expSystem()
{
    m_num_configs = 0;
    m_cur_config_num = -1;
    m_prev_config_num = 0;

    for (int i=0; i<MAX_EXP_CONFIGS; i++)
        m_pConfigs[i] = 0;
        
    m_pRawButtonArray = new rawButtonArray(this,staticOnButtonEvent);
    
    addConfig(new systemConfig(this));
    
    m_timer.priority(EXP_TIMER_PRIORITY);
    m_timer.begin(timer_handler,EXP_TIMER_INTERVAL);
}


void expSystem::begin()
{
    // get the brightness from EEPROM
    
    int brightness = EEPROM.read(EEPROM_BRIGHTNESS);
    if (brightness == 255)
        brightness = DEFAULT_BRIGHTNESS;
    setLEDBrightness(brightness);        

    // get config_num from EEPROM and activate it
    
    int config_num = EEPROM.read(EEPROM_CONFIG_NUM);
    if (config_num == 255)
        config_num = DEFAULT_CONFIG_NUM;
    if (config_num == 255)
        config_num = DEFAULT_CONFIG_NUM;
    if (config_num >= m_num_configs)
        config_num = m_num_configs - 1;

    config_num = 3;
        // override EEPROM setting 
        // for working on a particular config
        
    activateConfig(config_num);
}


// static
void expSystem::staticOnButtonEvent(void *obj, int row, int col, int event)
{
    ((expSystem *)obj)->onButtonEvent(row,col,event);
}


void expSystem::onButtonEvent(int row, int col, int event)
{
    // display(0,"expSystem::onButtonEvent(%d,%d,%s)",row,col,rawButtonArray::buttonEventName(event));
    
    #if 0 
        Serial3.print("E row(");
        Serial3.print(row,DEC);
        Serial3.print(") col(");
        Serial3.print(col,DEC);
        Serial3.print(") event:");
        Serial3.println(rawButtonArray::buttonEventName(event));
    #endif
    
    // handle changes to systemConfig
    
    if (row == 0 &&
        col == 4 &&
        m_cur_config_num &&
        event == BUTTON_EVENT_LONG_CLICK)
    {
        setLED(0,4,LED_PURPLE);
        activateConfig(0);
    }
    else
    {
        getCurConfig()->onButtonEvent(row,col,event);
    }
}


// static
void expSystem::timer_handler()
{
    s_pTheSystem->m_pRawButtonArray->task();
    
    #if WITH_ROTARY
        for (int i=0; i<NUM_ROTARY; i++)
            if (pollRotary(i))
                s_pTheSystem->getCurConfig()->onRotaryEvent(i,getRotaryValue(i));
    #endif
    
    #if WITH_PEDALS
        for (int i=0; i<NUM_ROTARY; i++)
            if (pollPedal(i))
                s_pTheSystem->getCurConfig()->onPedalEvent(i,getPedalValue(i));
    #endif
    
    s_pTheSystem->getCurConfig()->timer_handler();
}




void expSystem::updateUI()
{
    getCurConfig()->updateUI();
}



//-------------------------------------------------
// Config management
//-------------------------------------------------

    
void expSystem::addConfig(expConfig *pConfig)
{
    if (m_num_configs >= MAX_EXP_CONFIGS)
    {
        my_error("TOO MANY CONFIGURATIONS! %d",m_num_configs+1);
        return;
    }
    m_pConfigs[m_num_configs++] = pConfig;
}


void expSystem::activateConfig(int i)
{
    if (m_cur_config_num >= m_num_configs)
    {
        my_error("attempt to activate illegal configuarion %d",i);
        return;
    }
    
    // deactivate previous configuration
    
    if (m_cur_config_num >= 0)
    {
        getCurConfig()->end();
        m_prev_config_num = m_cur_config_num;
    }
    
    m_cur_config_num = i;
    
    // clear the TFT and show the config title
    
    #if WITH_CHEAP_TFT
    if (m_cur_config_num)
    {
        mylcd.Fill_Screen(0);
        mylcd.setFont(Arial_16_Bold);
        mylcd.Set_Text_Cursor(10,10);
        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.print(getCurConfig()->name());
        mylcd.Set_Draw_color(TFT_YELLOW);
	    mylcd.Draw_Line(0,36,mylcd.Get_Display_Width()-1,36);
    }
    #endif
    
    // start the configuration running
    
    getCurConfig()->begin();
    
    // add the system long click handler
    
    int mask = m_pRawButtonArray->getButtonEventMask(0,4);
    m_pRawButtonArray->setButtonEventMask(0,4, mask | BUTTON_EVENT_LONG_CLICK );
}    

            

    
