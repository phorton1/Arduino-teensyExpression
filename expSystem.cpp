
#include <myDebug.h>
#include "expSystem.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"
#include "pedals.h"
#include "rotary.h"
#include "myMidiHost.h"
#include <EEPROM.h>


#define EXP_TIMER_INTERVAL 5000
    // 5000 us = 5 ms == 200 times per second
#define EXP_TIMER_PRIORITY  192
    // compared to default priority of 128
#define EXP_CRITICAL_TIMER_INTERVAL 10000
    // 2000 us = 2 ms == 500 times per second      ... has problems
    // 1000 us                                     ... has problems
    // 100  us - 1/10 ms = 10,000 times per second ... works, but
    // 500  us = 1/2 ms = 2000 times per second    ... wow, is that the spec?
    
    // 1. I have more or less determined that the timer doesnt start again
    //    until the handler has returned.
    // 2. At some point the timer uses so much resources that the rest of
    //    the system is non functional
    // 3. I am just servicing a queue from an interrupt anyways. The most
    //    critical is to receive from the hosted device and send to the
    //    actual device, and currently, I am not responding directly to the
    //    usb_host interrupts, but rather, using a seperate timer loop to
    //    read what it has enqued.  I bumped the RX_QUEUE_SIZE for the
    //    midi device in USBHost_t36.h, and now I can reliably, if oafishly,
    //    turn the FTP on, and get all the bytes it sends, more or less
    //    regardless of the critical loop timing interval, which worked
    //    as high as 10000 (every 10 ms) or more, or even ridiculous values
    //    like 100000 (10 times per second).
    
    
#define EXP_CRITICAL_TIMER_PRIORITY  64
    // compared to default priority of 128


expSystem theSystem;


//----------------------------------------
// expConfig (base class)
//----------------------------------------


expConfig::expConfig()
{}


// virtual
void expConfig::begin()
    // derived classes should call base class method FIRST
    // base class clears all button registrations.
{
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            theButtons.setButtonEventMask(row,col,0);
            setLED(row,col,0);
        }
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

}


void expSystem::begin()
{
    theButtons.init();
    
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

    // config_num = 0;
        // override EEPROM setting 
        // for working on a particular config
    
    m_timer.priority(EXP_TIMER_PRIORITY);
    m_timer.begin(timer_handler,EXP_TIMER_INTERVAL);
    
    m_critical_timer.priority(EXP_CRITICAL_TIMER_PRIORITY);
    m_critical_timer.begin(critical_timer_handler,EXP_CRITICAL_TIMER_INTERVAL);
        // start the timer
        
    activateConfig(config_num);
        // show the first windw
    
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
    
    // start the configuration running
    
    getCurConfig()->begin();
    
    // add the system long click handler
    
    int mask = theButtons.getButtonEventMask(0,4);
    theButtons.setButtonEventMask(0,4, mask | BUTTON_EVENT_LONG_CLICK );
}    


//-----------------------------------------
// timer and events
//-----------------------------------------

void expSystem::updateUI()
{
    getCurConfig()->updateUI();
}



// static
void expSystem::timer_handler()
{
    theButtons.task();
    
    #if WITH_PEDALS
        thePedals.task();
    #endif
    
    #if WITH_ROTARY
        pollRotary();
    #endif
    
    theSystem.getCurConfig()->timer_handler();
}



// static
void expSystem::critical_timer_handler()
{
    #if 0 && WITH_MIDI_HOST
        // myusb.Task();       // does nothing on midi host device!
        uint32_t msg = midi1.myRead();		// read from host
        if (msg) theSystem.midiHostEvent(msg);
    #endif
    
    uint32_t msg2 = usb_midi_read_message();  // read from device   
    if (msg2) theSystem.midiEvent(msg2);

    theSystem.getCurConfig()->critical_timer_handler();
}



void expSystem::pedalEvent(int num, int value)
{
    getCurConfig()->onPedalEvent(num,value);
}


void expSystem::rotaryEvent(int num, int value)
{
    getCurConfig()->onRotaryEvent(num,value);
}


void expSystem::midiEvent(uint32_t msg)
{
    // display(0,"expSystem::midiEvent(%08x)",msg);
    getCurConfig()->onMidiEvent(msg);
}

#if WITH_MIDI_HOST
    void expSystem::midiHostEvent(uint32_t msg)
    {
        // display(0,"expSystem::midiHostEvent(%08x) to m_cur_config_num=%d",msg,m_cur_config_num);
        getCurConfig()->onMidiHostEvent(msg);
    }
#endif


void expSystem::buttonEvent(int row, int col, int event)
{
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




            

    
