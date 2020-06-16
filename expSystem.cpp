
#include <myDebug.h>
#include "expSystem.h"
#include "myTFT.h"
#include "myLeds.h"
#include "buttons.h"
#include "pedals.h"
#include "rotary.h"
#include "myMidiHost.h"
#include <EEPROM.h>

// Fishman TriplePlay MIDI HOST Spoof Notes
//
// This version WORKS as a midi host to the FTP dongle, appears in
// windows as a "Fishman TriplePlay" with similarly named
// midi ports, and successfully runs within the Windows FTP Editor !!
// REQUIRES setting MIDI4+SERIAL in Arduino IDE, and uninstalling windows
//    device upon name changes.
//
// I DON't LIKE THAT THERE 8 MIDI DEVICES WITH NO MEANINGFUL NAMES
//
//    do I wanna get into the whole desceiptors mess?
//    would need to denormalize usb_desc.c and for safety
//    a renamed version of prh_usb_desc.h, and tweak the
//    shit out of it to get it to happen at run time.
//
//    What I would like is the teensyExpression device to have
//    two jacks (labelled "in" and "out"), and when I wanna spoof
//    the FTP editor, there would be the four required jacks.
//    When running on the iPad, there should only be one device,
//    since nobody pays attentions to routines anyways.
//
//    
// I am checking in messy code because it has been a long road
// getting here, and I don't want to lose this.
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
//           And simply modifying the name of the device 
//           to "Fishman TriplePlay" in midi_names.c
//           It DOES NOT require spoofing the descriptors, etc.
//      Hooks rx_data(), which is the host usb IRQ handler, to
//           directly call the low level 'C' routines
//           usb_midi_write_packed(msg) and usb_midi_flush_output()
//           upon every receive packet.
//
//
// DEVICE (teensyDuino "self") usbMidi
//      Variable Name: usbMIDI (hardwired)
//      available based on USB Setting in Arduino IDE
//      I get it's messages based on calls to low calls to
//         low levl usb_midi_read_message() 'C' function
//         in current "critical_timer_handler()" implementation
//      which calls midiHostConfig::onMidiHostEvent() which
//         is where they get written TO the hosted device (FTP)
//         via the exposed USBHost_t36 MIDIDevice myMidiHost
//         midi1.write_packed(msg)
//
// This checked in, working spoof, uses the USE_MIDI_HOST_IRQ=1
// define in myMidiHost.h to override the rx_data() method and
// send the bytes to the teensyduino device directly from the IRQ.
//
// IT WAS IMPORTANT AND HARD TO FIND THAT I HAD TO LOWER THE PRIORITY
// OF THE critical_timer to let the host usb IRQs have priority.
//
// As it stands now THE SYSTEM IS NOT SYMETTRIC.  We read from the
// host based on direct usb IRQ's, but we read from the device based
// on a timer loop and the usb_midi_read_message() function.
//
// The IRQ is enqueing the 32bit messages (and I also modified USBHost_t36.h
// to increase the midi rx buffer size from 80 to 2048), which are currently,
// and messily, then dequeud in the "critical_timer_handler()" method, then
// printed to buffered text, and finally displayed in the updateUI() method
// called from loop().   That whole thing could be cleaned up to work with
// a single queue of 32 bit words, and to decode and show the queued messages
// separately for display.
//
// I have high hopes that this is going to let me play the midi guitar through
// teensyExpression device, but there is much work left to be done.
//
// Checking in !!!


#define EXP_TIMER_INTERVAL 5000
    // 5000 us = 5 ms == 200 times per second
#define EXP_TIMER_PRIORITY  192
    // compared to default priority of 128
    
#if USE_MIDI_HOST_IRQ
    #define EXP_CRITICAL_TIMER_INTERVAL 1000
    #define EXP_CRITICAL_TIMER_PRIORITY  192
#else
    #define EXP_CRITICAL_TIMER_INTERVAL 1000
    #define EXP_CRITICAL_TIMER_PRIORITY  64
    // compared to default priority of 128
#endif

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
    //    read what it has enqued.  
    
    


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
    #if WITH_MIDI_HOST  // && !USE_MIDI_HOST_IRQ
        myusb.Task();       // does nothing on midi host device!
        uint32_t msg = midi1.myRead();		// read from host
        if (msg)
        {
            theSystem.midiHostEvent(msg);
            // msg = midi1.myRead();
        }
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




            

    
