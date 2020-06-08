
#include <myDebug.h>
#include "defines.h"
#include "expSystem.h"
#include "myLeds.h"
#include "pedals.h"
#include "rotary.h"
#include <EEPROM.h>

#define EXP_TIMER_INTERVAL 1000
    // 1000 us = 1 ms == 1000 times per second!
#define EXP_TIMER_PRIORITY  192
    // compared to presumed (higher) priority of 128
    // for USB, midi HOST, and other (serial) interrupts


// THEORY OF OPERATION
//
// There are at least 3 interrupt timers (that I know of) going on at any time.
//
// - I presume there is a timer for the basic MIDI usb device functionality.
// - I presume there is a timer if I am using the midi HOST functionality.
// - I use a timer to check button, rotary, pedal, and serial states and send midi.
//
// Plus there are likely DMA interrupts used during showLEDs() and interrupts
// for any serial IO that happens (the USB port and/or the 2nd Serial port).
//
// I presume the basic MIDI and midi HOST interrupt timers are at default (128) priority level
// My basic timer loop will operate a lower priority (192) as per EXP_TIMER_INTERVAL.
//
// Funamental assumption is that I can send MIDI events from my interrupt handler.
// Secondary assumption, at this time, is that showLEDs() is fast enough to also
//     call from my interrupt handler.
//
// However, updating the TFT screen shall take place in the loop() method and may
// be interrupted.
//
// (1) I need to add an on/off switch as some configuration changes *may* require
//     reboots (i.e. turning the midi HOST on or off)




//-----------------------------------------------------
// configuration(0) systemConfig
//-----------------------------------------------------

class systemConfig : public expConfig
{
    public:
        
        systemConfig(expSystem *pSystem) :
            expConfig(pSystem)
        {
            m_changed = false;
            m_save_brightness = 0;
            m_next_config = 0;
        }
        
        virtual const char *name() { return "SYSTEM CONFIGURATION"; }
        
        virtual void begin()
        {
            expConfig::begin();
            
            m_changed = false;
            m_save_brightness = getLEDBrightness();
            m_next_config = m_pSystem->getPrevConfigNum();
            
            rawButtonArray *ba = m_pSystem->getRawButtonArray();
            
            ba->setButtonEventMask(0,0,BUTTON_EVENT_CLICK);
            ba->setButtonEventMask(0,1,BUTTON_EVENT_CLICK);
            ba->setButtonEventMask(0,3,BUTTON_EVENT_CLICK);
            ba->setButtonEventMask(0,4,BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK);
            
            setLED(0,0,LED_RED);
            setLED(0,1,LED_GREEN);
            setLED(0,3,LED_YELLOW);
            setLED(0,4,LED_PURPLE);
            
            for (int i=0; i<m_pSystem->getNumConfigs()-1; i++)
            {
                ba->setButtonEventMask(1,i,BUTTON_EVENT_CLICK);
                setLED(1,i, i == m_next_config-1 ? LED_WHITE : LED_BLUE);
            }
            
            showLEDs();
        }
    
        virtual void onButtonEvent(int row, int col, int event)
        {
            display(0,"systemConfig(%d,%d) event(%s)",row,col,rawButtonArray::buttonEventName(event));
            
            if (row == 0)
            {
                if (col == 0)
                {
                    int brightness = getLEDBrightness();
                    brightness -= 5;
                    if (brightness < 5) brightness = 5;
                    display(0,"decrease brightness to %d",brightness);
                    setLEDBrightness(brightness);
                    setLED(0,0,LED_RED);
                    showLEDs();
                }
                else if (col == 1)
                {
                    int brightness = getLEDBrightness();
                    brightness += 5;
                    if (brightness > 100) brightness = 100;
                    display(0,"increase brightness to %d",brightness);
                    setLEDBrightness(brightness);
                    setLED(0,1,LED_GREEN);
                    showLEDs();
                }
                else if (col == 3)
                {
                    setLEDBrightness(m_save_brightness);
                    m_pSystem->activateConfig(m_pSystem->getPrevConfigNum());
                }
                else if (col == 4)
                {
                    if (event == BUTTON_EVENT_LONG_CLICK)
                    {
                        int bright = getLEDBrightness();
                        display(0,"write bright=%d and config=%d to EEPROM",bright,m_next_config);
                        EEPROM.write(EEPROM_BRIGHTNESS,bright);
                        EEPROM.write(EEPROM_CONFIG_NUM,m_next_config);
                    }
                    m_pSystem->activateConfig(m_next_config);
                }
            }
            else if (row == 1)
            {
                if (col != m_next_config -1)
                {
                    setLED(1,m_next_config-1,LED_BLUE);
                    m_next_config = col + 1;
                    showLEDs();
                }
            }
        }

        
    private:
        
        bool m_changed;
        int m_save_brightness;
        int m_next_config;
        
};



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

extern expSystem *s_pTheSystem;
    // in teensyExpression.ino
    

expSystem::expSystem()
{
    m_num_configs = 0;
    m_cur_config_num = 0;
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
    proc_entry();
    
    #if 0
        for (int row=0; row<NUM_BUTTON_ROWS; row++)
            for (int col=0; col<NUM_BUTTON_COLS; col++)
                m_pRawButtonArray->setButtonEventMask(row,col,BUTTON_ALL_EVENTS);
    #endif
    
    // get the current configuration number and brightness from EEPROM
    
    int brightness = EEPROM.read(EEPROM_BRIGHTNESS);
    m_cur_config_num = EEPROM.read(EEPROM_CONFIG_NUM);
    
    display(0,"got bright=%d and config=%d from EEPROM",brightness,m_cur_config_num);
    
    if (brightness == 255)
        brightness = 70;
    if (m_cur_config_num == 255)
        m_cur_config_num = 1;
    if (m_cur_config_num >= m_num_configs)
        m_cur_config_num = m_num_configs - 1;

    setLEDBrightness(brightness);        
    activateConfig(m_cur_config_num);
    
    proc_leave();
}


// static
void expSystem::staticOnButtonEvent(void *obj, int row, int col, int event)
{
    ((expSystem *)obj)->onButtonEvent(row,col,event);
}


void expSystem::onButtonEvent(int row, int col, int event)
{
    display(0,"expSystem::onButtonEvent(%d,%d,%s)",row,col,rawButtonArray::buttonEventName(event));
    
    #ifdef USE_SERIAL_TO_RPI
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
    m_prev_config_num = m_cur_config_num;
    m_cur_config_num = i;
    
    // clear the TFT and show the config title
    
    #if WITH_CHEAP_TFT
        mylcd.Fill_Screen(0);
        mylcd.setFont(Arial_16_Bold);
        mylcd.Set_Text_Cursor(10,10);
        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.print(getCurConfig()->name());
        mylcd.Set_Draw_color(TFT_YELLOW);
	    mylcd.Draw_Line(0,36,mylcd.Get_Display_Width()-1,36);
    #endif
    
    // start the configuration running
    
    getCurConfig()->begin();
    
    // add the system long click handler
    
    int mask = m_pRawButtonArray->getButtonEventMask(0,4);
    m_pRawButtonArray->setButtonEventMask(0,4, mask | BUTTON_EVENT_LONG_CLICK );
}    

            

    
