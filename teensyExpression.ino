// teensyExpression.ino


#include <myDebug.h>
#include "src/prefs.h"
#include "src/myTFT.h"
#include "src/myLeds.h"
#include "src/pedals.h"
#include "src/rotary.h"
#include "src/buttons.h"
#include "src/expSystem.h"
#include "src/myMidiHost.h"
#include "src/fileSystem.h"


#define TOUCH_DRAW_TEST  0

#if TOUCH_DRAW_TEST
    #include "src/myTouchScreen.h"
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
    bool prefs_reset = init_global_prefs();

    // turn off myDebug output and
    // start the SERIAL_DEVICE (before USB is started)

    dbgSerial = 0;
    SERIAL_DEVICE.begin(115200);
    elapsedMillis serial_started = 0;
    while (serial_started<1000 && !SERIAL_DEVICE) {}

    #if 1
        if (SERIAL_DEVICE)
        {
            delay(1000);
            SERIAL_DEVICE.println("hello from teensyExpression.ino() setup() " TEENSY_EXPRESSION_VERSION " SERIAL_DEVICE port");
        }
    #endif

    // if the DEBUG_PORT is set to serial, we can start myDebug now

    uint8_t debug_device = getPref8(PREF_DEBUG_PORT);
    if (debug_device == DEBUG_DEVICE_SERIAL)
    {
        dbgSerial = &SERIAL_DEVICE;
    }


    //------------------------------------------
    // start the USB device
    //------------------------------------------

    if (getPref8(PREF_SPOOF_FTP))
        setFishmanFTPDescriptor();
    my_usb_init();
    delay(1000);

    // start the USB Serial port

    Serial.begin(115200);
    serial_started = 0;
    while (serial_started<1000 && !Serial) {}
    delay(400);

    // and if the pref is DEBUG_DEVICE_USB
    // set it into myDebug

    if (debug_device == DEBUG_DEVICE_USB)
    {
        dbgSerial = &Serial;
    }

    display(0,"teensyExpression,ino " TEENSY_EXPRESSION_VERSION " setup() started",0);

    //-------------------------------------------------
    // start the TFT display device
   //-------------------------------------------------

    initMyTFT();

    mylcd.setTextBackColor(0);
    mylcd.setTextColor(TFT_WHITE);
    mylcd.setFont(Arial_16);
    mylcd.setCursor(5,5);
    mylcd.print("teensyExpression ");
    mylcd.print(TEENSY_EXPRESSION_VERSION);
    mylcd.println(" started ... ");

    int do_delay = 2000;
    mylcd.setTextColor(TFT_YELLOW);

    if (prefs_reset)
    {
        const char *msg = "    PREFS WERE AUTOMATICALLY RESET!!";
        warning(0,"%s",msg);
        mylcd.println(msg);
        do_delay = 5000;
    }
    if (!dbgSerial)
    {
        const char *msg = "    NO SERIAL PORT IS ACTIVE!!";
        warning(0,"%s",msg);
        mylcd.println(msg);
        do_delay = 5000;
    }
    else if (debug_device == DEBUG_DEVICE_SERIAL)
    {
        const char *msg = "    DEBUG_OUTPUT to hardware SERIAL_DEVICE!";
        warning(0,"%s",msg);
        mylcd.println(msg);
        do_delay = 5000;
    }

    #if !defined(USB_MIDI4_SERIAL)
    {
        const char *msg = "    NOT COMPILED WITH USB_MIDI4_SERIAL !!!";
        my_error("%s",msg);
        mylcd.println(msg);
        do_delay = 5000;
    }
    #endif


    //--------------------------------------
    // start the file system
    //--------------------------------------

	uint8_t fs_device = getPref8(PREF_FILE_SYSTEM_PORT);
    warning(0,"FILE_SYS_DEVICE %s",
		fs_device == FILE_SYS_DEVICE_SERIAL ? "is SERIAL" :
		fs_device == FILE_SYS_DEVICE_USB 	? "is USB" :
		debug_device == DEBUG_DEVICE_SERIAL 	? "follows DEBUG_DEVICE which is SERIAL" :
		debug_device == DEBUG_DEVICE_USB 		? "follows DEBUG_DEVICE which is USB" :
		"follows DEBUG_DEVICE which is OFF" );

	if (!initFileSystem())
	{
        const char *msg = "    COULD NOT START FILE SYSTEM!!";
        my_error("%s",msg);
        mylcd.println(msg);
        do_delay = 10000;
	}
    else
    {
        mylcd.setTextColor(TFT_WHITE);
        mylcd.println("File System Started OK");
    }

    delay(do_delay);

    // start myMidihost
    // and the rest of the stuff

    midi_host.init();

    display(0,"initializing system ...",0);

    initLEDs();
    LEDFancyStart();
    clearLEDs();
    showLEDs();

    theSystem.begin();

    display(0,"system running ...",0);

}   // setup()




//-----------------------------------------
// loop
//-----------------------------------------

void loop()
{
    freeFileCommands();
    theSystem.updateUI();
}


