


#include "defines.h"
#include <myDebug.h>
#include <EEPROM.h>
#include "myTFT.h"
#include "myLeds.h"
#include "expSystem.h"
#include "buttons.h"
#include "systemConfig.h"
#include "oldRigConfig.h"
#include "testConfig.h"
#include "midiHostConfig.h"

#define WITH_SDCARD   0
#define TOUCH_DRAW_TEST  0
    // test defines at this time
    

#if WITH_PEDALS
    #include "pedals.h"
#endif

#if WITH_ROTARY
    #include "rotary.h"
#endif


#if WITH_SERIAL_PORT
    int serial_port_on = 0;
        // no current usage
#endif
    
    
#if WITH_MIDI_HOST
    #include "myMidiHost.h"
    int midi_host_on = 0;
#endif


#if WITH_TOUCH
    // Resistive touch screen on Cheap Ardino 3.5" 320x480 TFT's
    
    #include <stdint.h>
    #include <Arduino.h>
    #include "TouchScreen.h"    // modified to (at least) reset pinModes

    // need an object, calibration routine, etc
    
    int touch_minx=240;
    int touch_maxx=920;
    int touch_miny=90;
    int touch_maxy=860;

    TouchScreen theTouchScreen = TouchScreen(XP, YP, XM, YM);   // , 300);
#endif


#if WITH_SDCARD
    #include <SD.h>
    #include <SPI.h>

    Sd2Card card;
    SdVolume volume;
    SdFile root;

    const int chipSelect = BUILTIN_SDCARD;
        // change this to match your SD shield or module;
        // Arduino Ethernet shield: pin 4
        // Adafruit SD shields and modules: pin 10
        // Sparkfun SD shield: pin 8
        // Teensy audio board: pin 10
        // Teensy 3.5 & 3.6 on-board: BUILTIN_SDCARD
        // Wiz820+SD board: pin 4
        // Teensy 2.0: pin 0
        // Teensy++ 2.0: pin 20


    void sdCardTest()
    {
        // UNCOMMENT THESE TWO LINES FOR TEENSY AUDIO BOARD:
        // SPI.setMOSI(7);  // Audio shield has MOSI on pin 7
        // SPI.setSCK(14);  // Audio shield has SCK on pin 14
        
        Serial.print("\nInitializing SD card...");
        
        // we'll use the initialization code from the utility libraries
        // since we're just testing if the card is working!
        
        if (!card.init(SPI_HALF_SPEED, chipSelect))
        {
            Serial.println("initialization failed. Things to check:");
            Serial.println("* is a card inserted?");
            Serial.println("* is your wiring correct?");
            Serial.println("* did you change the chipSelect pin to match your shield or module?");
            return;
        } else
        {
            Serial.println("Wiring is correct and a card is present."); 
        }
        
        // print the type of card
        Serial.print("\nCard type: ");
        switch(card.type())
        {
            case SD_CARD_TYPE_SD1:
                Serial.println("SD1");
                break;
            case SD_CARD_TYPE_SD2:
                Serial.println("SD2");
                break;
            case SD_CARD_TYPE_SDHC:
                Serial.println("SDHC");
                break;
            default:
                Serial.println("Unknown");
        }
        
        // Now we will try to open the 'volume'/'partition' - it should be FAT16 or FAT32
        if (!volume.init(card))
        {
            Serial.println("Could not find FAT16/FAT32 partition.\nMake sure you've formatted the card");
            return;
        }
        
        // print the type and size of the first FAT-type volume
        uint32_t volumesize;
        Serial.print("\nVolume type is FAT");
        Serial.println(volume.fatType(), DEC);
        Serial.println();
        
        volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
        volumesize *= volume.clusterCount();       // we'll have a lot of clusters
        if (volumesize < 8388608ul)
        {
            Serial.print("Volume size (bytes): ");
            Serial.println(volumesize * 512);        // SD card blocks are always 512 bytes
        }
        Serial.print("Volume size (Kbytes): ");
        volumesize /= 2;
        Serial.println(volumesize);
        Serial.print("Volume size (Mbytes): ");
        volumesize /= 1024;
        Serial.println(volumesize);
        
        Serial.println("\nFiles found on the card (name, date and size in bytes): ");
        root.openRoot(volume);
        
        // list all files in the card with date and size
        root.ls(LS_R | LS_DATE | LS_SIZE);
    }
#endif




//-----------------------------------------------------------
// setup
//-----------------------------------------------------------


void setup()
{
    // Only initialize the serial port if it's in the build options
    // Wait up to 1 second for it to start .. 
    
    #if defined(USB_SERIAL) || defined(USB_MIDI_SERIAL)
        Serial.begin(115200);
        elapsedMillis serial_started = 0;
        while (serial_started<1000 && !Serial) {}
        delay(400);
        display(0,"teensyExpression version %s started",VERSION);
    #endif

    // start the TFT display device

    initMyTFT();

    mylcd.Set_Text_Back_colour(0); 
    mylcd.Set_Text_colour(TFT_WHITE);
    mylcd.setFont(Arial_16);
    mylcd.Set_Text_Cursor(5,5);
    mylcd.print("teensyExpression version ");
    mylcd.print(VERSION);
    mylcd.println(" started ...");
    #if defined(USB_SERIAL)
        display(0,"    USB_SERIAL",0);
        mylcd.println("    USB_SERIAL");
    #elif defined(USB_MIDI)
        display(0,"    USB_MIDI",0);
        mylcd.println("    USB_MIDI");
    #elif defined(USB_MIDI_SERIAL)
        display(0,"    USB_MIDI_SERIAL",0);
        mylcd.println("    USB_MIDI_SERIAL");
    #else
        display(0,"    WARNING: NO MIDI OR SERIAL!!",0);
        mylcd.println("    WARNING: NO MIDI OR SERIAL!!");
    #endif

    // serial port
    
    #if WITH_SERIAL_PORT
        serial_port_on = EEPROM.read(EEPROM_SERIAL_PORT);
        if (serial_port_on == 255)
            serial_port_on = DEFAULT_SERIAL_PORT;
    
        display(0,"    SERIAL_PORT %s",serial_port_on?"ON":"OFF");
        mylcd.print("    SERIAL_PORT ");
        mylcd.println(serial_port_on?"ON":"OFF");
        
        if (serial_port_on)
        {
            Serial3.begin(115200);
            Serial3.println("teensy expression Serial3 to rPi started");
        }
    #else
        display(0,"    NO SERIAL_IO_PORT!!",0);
        mylcd.println("    NO SERIAL_IO_PORT!!");
    #endif
    
    
    #if WITH_MIDI_HOST
        // Wait 1.5 seconds before turning on USB Host.  If connected USB devices
        // use too much power, Teensy at least completes USB enumeration, which
        // makes isolating the power issue easier.

        midi_host_on = EEPROM.read(EEPROM_MIDI_HOST);
        if (midi_host_on == 255)
            midi_host_on = DEFAULT_MIDI_HOST;
            
        display(0,"    MiDI_HOST %s",midi_host_on?"ON":"OFF");
        mylcd.print("    MiDI_HOST ");
        mylcd.println(midi_host_on?"ON":"OFF");

        if (midi_host_on)
        {
            // delay(1500);
            myusb.begin();
        }
    #else
        display(0,"    NO MIDI_HOST!!",0);
        #if WITH_CHEAP_TFT
            mylcd.println("    NO MIDI_HOST!!");
        #endif
    #endif    
    
    
    #if WITH_ROTARY
        display(0,"    WITH ROTARY",0);
        mylcd.println("    WITH ROTARY");
        initRotary();
    #else
        display(0,"    NO ROTARY!!",0);
        mylcd.println("    NO ROTARY!!");
    #endif

    
    #if WITH_PEDALS
        display(0,"    WITH PEDALS",0);
        mylcd.println("    WITH PEDALS");
        thePedals.init();
    #else
        display(0,"    NO PEDALS!!",0);
        mylcd.println("    NO PEDALS!!");
    #endif

    
    #if WITH_TOUCH
        display(0,"    WITH TOUCH",0);
        mylcd.println("    WITH TOUCH");
    #else
        display(0,"    NO TOUCH!!",0);
        mylcd.println("    NO TOUCH!!");
    #endif


    display(0,"initializing system ...",0);
    mylcd.println("initializing system .");

    initLEDs();
    clearLEDs();
    showLEDs();

    // give a little time to see start up messages
    // before we erase the screen and start the system
    
    delay(1200);

    theSystem.addConfig(new systemConfig());
    theSystem.addConfig(new oldRigConfig());
    theSystem.addConfig(new testConfig());
    theSystem.addConfig(new midiHostConfig());
    theSystem.begin();
        
    display(0,"system running ...",0);
    
    #if WITH_SDCARD
        sdCardTest();
    #endif

}   // setup()





//-----------------------------------------
// loop
//-----------------------------------------

void loop()
{
    theSystem.updateUI();
        // midiHostConfig is only person who calls myUSB.Task(),
        // midi1.read(), and usbMIDI.read() at this time.

    
    #if WITH_TOUCH && TOUCH_DRAW_TEST
    
        static elapsedMillis clear_it = 0;
        if (clear_it > 3000)
        {
            mylcd.Fill_Screen(0);
            clear_it = 0;
        }
        
        TSPoint p = theTouchScreen.getPoint();
        if (p.z> 50 &&        // > ts.pressureThreshhold) {
            p.x > touch_minx && 
            p.x < touch_maxx && 
            p.y > touch_miny && 
            p.y < touch_maxy) 
            
        {
            clear_it = 0;
            float myx = ((float)(p.x - touch_minx)) / ((float)(touch_maxx - touch_minx));
            float myy = ((float)(p.y - touch_miny)) / ((float)(touch_maxy - touch_miny));
            
            int use_x = ((1-myx) * 480);
            int use_y = ((1-myy) * 320);
            mylcd.Print_String("o",use_x,use_y);
            
            #if 0
                display("x=%d \ty=%d \tpressure=%d \t\t%d\t%d",p.x,p.y,p.z,myx,myy);
            #endif
        }
        
    #endif

}   // loop()





