
#include "defines.h"
#include <myDebug.h>
#include <EEPROM.h>


#if WITH_SYSTEM
    #include "myLeds.h"
    #include "expSystem.h"
    #include "oldRigConfig.h"
    #include "testConfig.h"
    
    expSystem *s_pTheSystem;
#endif


#if WITH_CHEAP_TFT
    #include <LCDWIKI_GUI.h>    // my modified Core graphics library
    #include <LCDWIKI_KBV.h>    // my modified Hardware-specific library
    #include <font_Arial.h>

    // LCDWIKI_KBV(model,cs,cd,wr,rd,reset)
    
    LCDWIKI_KBV mylcd(
        ILI9486,
        CHEAP_TFT_CS,
        CHEAP_TFT_CD_RS,
        CHEAP_TFT_WR,
        CHEAP_TFT_RD,
        CHEAP_TFT_RESET);
#endif


#if WITH_SERIAL_PORT
    int serial_port_on = 0;
#endif
    
    
#if WITH_MIDI_HOST
    #include <USBHost_t36.h>

    USBHost myusb;
    USBHub hub1(myusb);
    USBHub hub2(myusb);
    MIDIDevice midi1(myusb);
    
    int midi_host_on = 0;
#endif


#if WITH_PEDALS
    #include "pedals.h"
#endif


#if WITH_ROTARY
    #include "rotary.h"
#endif


#if WITH_TOUCH
    // Resistive touch screen on Cheap Ardino 3.5" 320x480 TFT's

    #define TOUCH_DRAW_TEST  0
    
    #include <stdint.h>
    #include <Arduino.h>
    #include "TouchScreen.h"    // modified to (at least) reset pinModes

    // need a calibration routine
    
    int minx=240;
    int maxx=920;
    int miny=90;
    int maxy=860;

    TouchScreen ts = TouchScreen(XP, YP, XM, YM);   // , 300);
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

    #if WITH_CHEAP_TFT
        display(0,"    WITH CHEAP_TFT",0);
        extern void setCheapTFTDataPins(int p0,int p1,int p2,int p3, int p4, int p5, int p6, int p7);
        setCheapTFTDataPins(
            CHEAP_TFT_DATA0,
            CHEAP_TFT_DATA1,
            CHEAP_TFT_DATA2,
            CHEAP_TFT_DATA3,
            CHEAP_TFT_DATA4,
            CHEAP_TFT_DATA5,
            CHEAP_TFT_DATA6,
            CHEAP_TFT_DATA7);

        mylcd.Init_LCD();
        mylcd.Set_Rotation(1);
        mylcd.Fill_Screen(0);
        mylcd.Set_Text_Mode(0);
        mylcd.Set_Text_Back_colour(0); 
        mylcd.Set_Text_colour(TFT_WHITE);
        // mylcd.Set_Text_Size(2);
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
    #else
        display(0,"    NO CHEAP_TFT!!",0);
    #endif
    
    #if WITH_SERIAL_PORT
        serial_port_on = EEPROM.read(EEPROM_SERIAL_PORT);
        if (serial_port_on == 255)
            serial_port_on = DEFAULT_SERIAL_PORT;
    
        display(0,"    SERIAL_PORT %s",serial_port_on?"ON":"OFF");
        #if WITH_CHEAP_TFT
            mylcd.print("    SERIAL_PORT ");
            mylcd.println(serial_port_on?"ON":"OFF");
        #endif
        
        if (serial_port_on)
        {
            Serial3.begin(115200);
            Serial3.println("teensy expression Serial3 to rPi started");
        }
    #else
        display(0,"    NO SERIAL_IO_PORT!!",0);
        #if WITH_CHEAP_TFT
            mylcd.println("    NO SERIAL_IO_PORT!!");
        #endif
    #endif
    
    
    #if WITH_MIDI_HOST
        // Wait 1.5 seconds before turning on USB Host.  If connected USB devices
        // use too much power, Teensy at least completes USB enumeration, which
        // makes isolating the power issue easier.

        midi_host_on = EEPROM.read(EEPROM_MIDI_HOST);
        if (midi_host_on == 255)
            midi_host_on = DEFAULT_MIDI_HOST;
            
        display(0,"    MiDI_HOST %s",midi_host_on?"ON":"OFF");
        #if WITH_CHEAP_TFT
            mylcd.print("    MiDI_HOST ");
            mylcd.println(midi_host_on?"ON":"OFF");
        #endif

        if (midi_host_on)
        {
            // delay(1500);
            myusb.begin();
            midi1.setHandleNoteOn(myNoteOn);
            midi1.setHandleNoteOff(myNoteOff);
        }
    #else
        display(0,"    NO MIDI_HOST!!",0);
        #if WITH_CHEAP_TFT
            mylcd.println("    NO MIDI_HOST!!");
        #endif
    #endif    
    
    
    #if WITH_ROTARY
        display(0,"    WITH ROTARY",0);
        #if WITH_CHEAP_TFT
            mylcd.println("    WITH ROTARY");
        #endif
        initRotary();
    #else
        display(0,"    NO ROTARY!!",0);
        #if WITH_CHEAP_TFT
            mylcd.println("    NO ROTARY!!");
        #endif
    #endif

    
    #if WITH_PEDALS
        display(0,"    WITH PEDALS",0);
        #if WITH_CHEAP_TFT
            mylcd.println("    WITH PEDALS");
        #endif
        initPedals();
    #else
        display(0,"    NO PEDALS!!",0);
        #if WITH_CHEAP_TFT
            mylcd.println("    NO PEDALS!!");
        #endif
    #endif

    
    #if WITH_TOUCH
        display(0,"    WITH TOUCH",0);
        #if WITH_CHEAP_TFT
            mylcd.println("    WITH TOUCH");
        #endif
    #else
        display(0,"    NO TOUCH!!",0);
        #if WITH_CHEAP_TFT
            mylcd.println("    NO TOUCH!!");
        #endif
    #endif


    #if WITH_SYSTEM    
        display(0,"initializing system ...",0);
        #if WITH_CHEAP_TFT
            mylcd.println("initializing system .");
        #endif        

        initLEDs();
        clearLEDs();
        showLEDs();

        #if WITH_CHEAP_TFT
            // give it time to see start up messages
            delay(1200);
        #endif
        
        s_pTheSystem = new expSystem;
        s_pTheSystem->addConfig(new oldRigConfig(s_pTheSystem));
        s_pTheSystem->addConfig(new testConfig(s_pTheSystem));
        s_pTheSystem->begin();
        
        display(0,"system running ...",0);
    #else
        display(0,"    NO SYSTEM!!",0);
        #if WITH_CHEAP_TFT
            mylcd.println("    NO SYSTEM!!");
        #endif
    #endif
    
    
    
}   // setup()





//-----------------------------------------
// loop
//-----------------------------------------

void loop()
{
    #if WITH_MIDI_HOST
        myusb.Task();
        midi1.read();
    #endif
    
    #if WITH_SYSTEM
        s_pTheSystem->updateUI();
    #endif
    
    #if WITH_TOUCH && WITH_CHEAP_TFT && TOUCH_DRAW_TEST
    
        static elapsedMillis clear_it = 0;
        if (clear_it > 3000)
        {
            mylcd.Fill_Screen(0);
            clear_it = 0;
        }
        
        TSPoint p = ts.getPoint();
        if (p.z> 50 &&        // > ts.pressureThreshhold) {
            p.x > minx && 
            p.x < maxx && 
            p.y > miny && 
            p.y < maxy) 
            
        {
            clear_it = 0;
            float myx = ((float)(p.x - minx)) / ((float)(maxx - minx));
            float myy = ((float)(p.y - miny)) / ((float)(maxy - miny));
            
            int use_x = ((1-myx) * 480);
            int use_y = ((1-myy) * 320);
            mylcd.Print_String("o",use_x,use_y);
            
            #if 0
                Serial.print("X = "); Serial.print(p.x);
                Serial.print("\tY = "); Serial.print(p.y);
                Serial.print("\tPressure = "); Serial.print(p.z);
                Serial.print("    ");
                Serial.print(myx);
                Serial.print("    ");
                Serial.print(myy);
                Serial.println();
            #endif
        }
        
    #endif

}   // loop()






//---------------------------------------
// midi HOST handlers
//---------------------------------------


#if WITH_MIDI_HOST
    // these methods will only be called if usb_host_on == true

    void myNoteOn(byte channel, byte note, byte velocity)
    {
        // When a USB device with multiple virtual cables is used,
        // midi1.getCable() can be used to read which of the virtual
        // MIDI cables received this message.
        
        int cable = midi1.getCable();
        if (cable) return;
        usbMIDI.sendNoteOn(note, velocity, channel, cable);

        #if WITH_TFT
            int16_t x,y;
            tft.getCursor(&x,&y);
            if (y > 220)
            {
                y = 0;
                tft.fillScreen(ILI9341_BLACK);
            }
            tft.setCursor(0,y);
            tft.print("Note On  ");
            tft.setCursor(60,y);
            tft.print(note,DEC);
            tft.setCursor(100,y);
            tft.println(velocity,DEC);
        #endif
        
        // display(0,"Cable=%d  Note on, ch=%d note=%d vel=%d",cable,channel,note,velocity);
    }
    
    
    void myNoteOff(byte channel, byte note, byte velocity)
    {
        int cable = midi1.getCable();
        if (cable) return;
        usbMIDI.sendNoteOff(note, velocity, channel, cable);
        
        //display(0,"Cable=%d  Note off, ch=%d note=%d vel=%d",cable,channel,note,velocity);

        #if WITH_TFT
            int16_t x,y;
            tft.getCursor(&x,&y);
            if (y > 220)
            {
                y = 0;
                tft.fillScreen(ILI9341_BLACK);
            }
            tft.setCursor(0,y);
            tft.print("Note Off");
            tft.setCursor(160,y);
            tft.print(note,DEC);
            tft.setCursor(220,y);
            tft.println(velocity,DEC);
        #endif
    }
#endif
