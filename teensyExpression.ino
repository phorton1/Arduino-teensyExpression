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
#include "src/myTouchScreen.h"



#define TOUCH_DRAW_TEST  0


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

    #if 1
        if (Serial3)
        {
            delay(1000);
            Serial3.println("hello from teensyExpression Serial3 port");
        }
    #endif

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
    display(0,"teensyExpression v2.01 started",0);

    // start the TFT display device

    initMyTFT();

    mylcd.Set_Text_Back_colour(0);
    mylcd.Set_Text_colour(TFT_WHITE);
    mylcd.setFont(Arial_16);
    mylcd.Set_Text_Cursor(5,5);
    mylcd.print("teensyExpression ");
    mylcd.print(TEENSY_EXPRESSION_VERSION);
    mylcd.println(" started ... ");

    int do_delay = 3000;

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
    theSystem.begin();

    display(0,"system running ...",0);

}   // setup()




//-----------------------------------------
// loop
//-----------------------------------------

void loop()
{
    theSystem.updateUI();
        // see expSystem.cpp

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
