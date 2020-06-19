
#include <EEPROM.h>
#include <myDebug.h>
#include "expSystem.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"
#include "pedals.h"
#include "rotary.h"
#include "myMidiHost.h"
#include "midiQueue.h"
#include "FTP.h"


// Fishman TriplePlay MIDI HOST Spoof Notes
//
// This version WORKS as a midi host to the FTP dongle, appears in
// windows as a "Fishman TriplePlay" with similarly named
// midi ports, and successfully runs within the Windows FTP Editor
//
// REQUIRES setting MIDI4+SERIAL in Arduino IDE, as I did not want
// to muck around with Paul's midi.h file where it checks MIDI_NUM_CABLES
// inline, and IT is compiled with the original usb_desc.h, and it
// will not work properly as just a MIDI device (which uses SEREMU).
//
// Also note that the COM port changes from 3 to 11 when you change
// the configuration.
//
// As it stands right now, I am using a modified version of Paul's
// USBHost_t36.h file that exposes variables on it's MIDIDevice class,
// and makes a couple of functions (the usb IRQ handlers) virutal,
// so that I can inherit from it and implement the myMidiHost object,
// which tightly sends the hosted midi messges directly to the
// teensyDuino USBMidi, via the low level 'C' calls underlying
// it's hardwired "usbMIDI" class.
//
// HOST    myMidiHost : public USBHost_t36.h MIDIDevice
//
//      myMidiHost
//      Variable Name:  midi1 
//      Derivees from USBHost_t36.h MIDIDevice
//      Which has been modified to expose protected vars
//         and make a method rx_data() virtual
//      Spoof requires setting MIDI4+SERIAL in Arduino IDE
//      Hooks rx_data(), which is the host usb IRQ handler, to
//           directly call the low level 'C' routines
//           usb_midi_write_packed(msg) and usb_midi_flush_output()
//           upon every received packet.
//
// DEVICE (teensyDuino "self") usbMidi
//      Variable Name: usbMIDI (hardwired)
//      available based on USB Setting in Arduino IDE
//      I get it's messages based on calls to low calls to
//         low levl usb_midi_read_message() 'C' function
//         in the critical_timer_handler() implementation
//      which is where they get written TO the hosted device (FTP)
//         via the exposed USBHost_t36 MIDIDevice myMidiHost
//         midi1.write_packed(msg) method
//
// IT WAS IMPORTANT AND HARD TO FIND THAT I HAD TO LOWER THE PRIORITY
// OF THE critical_timer to let the host usb IRQs have priority.
//
// THE SYSTEM IS NOT SYMETTRIC.  We read from the host based on direct
// usb IRQ's, but we read from the device based on a timer loop and the
// usb_midi_read_message() function.
//
// The IRQ is enqueing the 32bit messages (and I also modified USBHost_t36.h
// to increase the midi rx buffer size from 80 to 2048), which are currently,
// and messily, then dequeud in the "critical_timer_handler()" method, then
// printed to buffered text, and finally displayed in the updateUI() method
// called from loop().   That whole thing could be cleaned up to work with
// a single queue of 32 bit words, and to decode and show the queued messages
// separately for display.
//


//-------------------------------------
// critical timer loop
//-------------------------------------
// The critical_timer_handler() is ONLY used to dequeue DEVICE
// (teensyDuino) usb midi events and send them as rapidly as
// possible to the Hosted device and enqueue them for further
// processing in the normal processing loop.

#define EXP_CRITICAL_TIMER_INTERVAL 1000
#define EXP_CRITICAL_TIMER_PRIORITY  192
    // Available priorities:
    // Cortex-M4: 0,16,32,48,64,80,96,112,128,144,160,176,192,208,224,240
    // Cortex-M0: 0,64,128,192


//----------------------------------
// normal timer loop
//----------------------------------
// The "normal" timer loop does the bulk of the work in the system.
// It is used to
//
//      (a) check the BUTTONS, PEDALS, and ROTARY states and
//          generate events based on their changes.
//      (b) process incoming or outgoing MIDI as necessary
//          to generate program related events based on them.
//      (c) re-enqueue the incoming and outgoing (processed) MIDI
//          messags for display.
//      (d) used variously by other objects to implement key
//          repeats, etc.


#define EXP_TIMER_INTERVAL 5000
    // 5000 us = 5 ms == 200 times per second
#define EXP_TIMER_PRIORITY  240                     // lowest priority
    // compared to default priority of 128
    
// 1. I have more or less determined that the timers don't start again
//    until the handler has returned.
// 2. At some point the timers use so much resources that the rest of
//    the system is non functional


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
// events
//-----------------------------------------


void expSystem::pedalEvent(int num, int value)
{
    getCurConfig()->onPedalEvent(num,value);
}


void expSystem::rotaryEvent(int num, int value)
{
    getCurConfig()->onRotaryEvent(num,value);
}



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






//-----------------------------------------
// timer handlers
//-----------------------------------------


// static
void expSystem::critical_timer_handler()
{
    uint32_t msg = usb_midi_read_message();  // read from device
    
    if (msg)
    {
		// write it to the midi host

        midi1.write_packed(msg);
        
        // enqueue it for processing (display from device)
        
        enqueueProcess(msg);
    }
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

    // process incoming and outgoing midi events
    
    dequeueProcess();
    
    theSystem.getCurConfig()->timer_handler();
}



void expSystem::updateUI()
{
	// current "process" function called from timer_handler()
	// dequeueProcess();
	
    getCurConfig()->updateUI();
}





            

    
