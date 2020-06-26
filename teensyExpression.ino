
#include <myDebug.h>
#include "prefs.h"
#include "myTFT.h"
#include "myLeds.h"
#include "pedals.h"
#include "rotary.h"
#include "buttons.h"
#include "expSystem.h"
#include "myMidiHost.h"
#include "myTouchScreen.h"



#define TOUCH_DRAW_TEST  0
#define WITH_SDCARD      0
    // test defines at this time



//-------------------------------------
// SD Card Test
//-------------------------------------

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


extern "C" {
    extern void setFishmanFTPDescriptor();
    extern void my_usb_init();          // in usb_dev.c
}


void setup()
{
    init_global_prefs();

    // start the hardware serial port

    Serial3.begin(115200);
    elapsedMillis serial_started = 0;
    while (serial_started<1000 && !Serial3) {}

    // Turn off display() and the like
    // The ports will still be opened even if nothing goes
    // to them.  This may add to startup time if nobody is
    // there to open them ..

    uint8_t serial_debug_pref = getPref8(PREF_DEBUG_PORT);
    if (serial_debug_pref == 2)
    {
        dbgSerial = &Serial3;
        display(0,"debugging output redirected to Serial3",0);
    }
    else if (!serial_debug_pref)
    {
        dbgSerial = 0;      // yikes!
    }

    // start the teensyDuino (self) USB device

    if (getPref8(PREF_SPOOF_FTP))
        setFishmanFTPDescriptor();
    my_usb_init();
    delay(1000);

    // initialize the main (debugging) serial port

    Serial.begin(115200);
    serial_started = 0;
    while (serial_started<1000 && !Serial) {}

    delay(400);
    display(0,"teensyExpression version %s started",VERSION);

    // start the TFT display device

    initMyTFT();

    mylcd.Set_Text_Back_colour(0);
    mylcd.Set_Text_colour(TFT_WHITE);
    mylcd.setFont(Arial_16);
    mylcd.Set_Text_Cursor(5,5);
    mylcd.print("teensyExpression version ");
    mylcd.print(VERSION);
    mylcd.println(" started ...");

    int do_delay = 0;

    if (!dbgSerial)
    {
        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.println("    NO SERIAL PORT IS ACTIVE!!");
        do_delay = 3000;
    }
    else if (serial_debug_pref == 2)
    {
        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.println("    DEBUG_OUTPUT to hardware Serial3!");
        if (!do_delay) do_delay = 1200;
    }

    #if !defined(USB_MIDI4_SERIAL)
        error("PROGRAM IS NOT COMPILED UNDER USB_MIDI4_SERIAL teensyDuino type!! Things may not work correctly!!!",0);
        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.println("    NOT COMPILED WITH USB_MIDI4_SERIAL !!");
        do_delay = 5000;
    #endif

    if (do_delay)
        delay(do_delay);

    // start myMidihost

    midi_host.init();

    // and the rest of the stuff

    display(0,"initializing system ...",0);

    initLEDs();
    clearLEDs();
    showLEDs();

    // give a little time to see start up messages
    // before we erase the screen and start the system

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


    #if TOUCH_DRAW_TEST

        // if I don't assume a "release" for at least 350 or so ms
        // after a "press", then I can implement swipe gestures.

        static bool cleared = 0;
        static elapsedMillis clear_it = 0;
        if (!cleared && clear_it > 350)
        {
            mylcd.Fill_Screen(0);
            clear_it = 0;
            cleared = 1;
        }

        int x,y,z;
        theTouchScreen.get(&z,&x,&y);
        if (z)
        {
            cleared = 0;
            clear_it = 0;
            mylcd.setDefaultFont();
            mylcd.Set_Text_Size(3);
            mylcd.Print_String("o",x,y);
        }

    #endif

}   // loop()
