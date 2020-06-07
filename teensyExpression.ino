
#include <myDebug.h>

#define WITH_SYSTEM           1
#define WITH_MIDI_HOST        0
#define WITH_CHEAP_TFT        1         // Cheap Ardino 3.5" 320x480 TFT's
#define WITH_TOUCH            1         // Cheap Arduino Resistive touch screen
#define WITH_ROTARY           1

// not used in initial implementation

#define WITH_TFT              0      // Teensy 320x400 ILI9341/XPT2046 touch screen
#define WITH_TOUCH_XPT2046    0


#define VERSION   "1.2"

#if WITH_SYSTEM
    #include "myLeds.h"
    #include "expSystem.h"
    #include "oldRigConfig.h"
    #include "testConfig.h"
#endif


#if WITH_MIDI_HOST
    #include <USBHost_t36.h>
    
    USBHost myusb;
    USBHub hub1(myusb);
    USBHub hub2(myusb);
    MIDIDevice midi1(myusb);
#endif



//----------------------------------------------------------------------
// PIN USAGE
//----------------------------------------------------------------------
// left/right  main               alternate             
//   pin
//    GND
// L  0        unused1 RX1 used by LEDS             
// L  1        unused2 TX1 used by LEDS             
// L  2        ROTARY_1A                        rotary pin assignments not updated from below yet
// L  3        ROTARY_1B
// L  4        ROTARY_2A                            
// L  5        LEDS_OUT (Serial1)                   
// L  6        ROTARY_2B                            
// L  7        SERIAL_RX3                           
// L  8        SERIAL_TX3         
// L  9        ROTARY_3A
// L 10        ROTARY_3B
// L 11        ROTARY_4A
// L 12        ROTARY_4B
//   3.3V
// L 24        BUTTON_OUT0           
// L 25        BUTTON_OUT1           
// L 26        BUTTON_OUT2           
// L 27        BUTTON_OUT3           
// L 28        BUTTON_OUT4           
// L 29        BUTTON_IN0            
// L 30        BUTTON_IN1            
// L 31 A12    BUTTON_IN2            
// L 32 A13    BUTTON_IN3            
//      5V
//      AGND
//      GND
// R 23 A9     EXPR4                                            
// R 22 A8     EXPR3                 
// R 21 A7     EXPR2                 
// R 20 A6     EXPR1                 
// R 19 A5     CHEAP_TFT_DATA0                         
// R 18 A4     CHEAP_TFT_RESET
// R 17 A3     CHEAP_TFT_CS            
// R 16 A2     CHEAP_TFT_CD(RS)          
// R 15 A1     CHEAP_TFT_WR              
// R 14 A0     CHEAP_TFT_RD                           
// R 13        CHEAP_TFT_DATA1               
//      3,3V
// R    A22    x - unused analog only                             
// R    A21    x - unused analog only                             
// R 39 A20    CHEAP_TFT_DATA7                              
// R 38 A19    CHEAP_TFT_DATA6                                                       
// R 37 A18    CHEAP_TFT_DATA5                                                                                   
// R 36 A17    CHEAP_TFT_DATA4                                                          
// R 35 A16    CHEAP_TFT_DATA3                                                          
// R 34 A15    CHEAP_TFT_DATA2                                                          
// R 33 A14    BUTTON_IN4                               

// The pins for the TFT were not well laid on in the circuit,
// causing a twisted cable, and some late fixes (13 and 19 to
// CHEAP_DATA1 and 0, right on top of unused A21 and A22 pins
// which cannot be used for digital io.
//
// The pins for the ROTARY were munged ... one of em is
// different, and the order is weird
// 
//--------------------------------------------
// DEFINES and CONDITIONAL COMPILATION
//--------------------------------------------
// defines needed by more than one compilation

#define CHEAP_TFT_DATA0     19      // needed by ts
#define CHEAP_TFT_DATA1     13      // needed by ts
#define CHEAP_TFT_DATA2     34
#define CHEAP_TFT_DATA3     35
#define CHEAP_TFT_DATA4     36
#define CHEAP_TFT_DATA5     37
#define CHEAP_TFT_DATA6     38
#define CHEAP_TFT_DATA7     39
        

#define CHEAP_TFT_RD         14
#define CHEAP_TFT_WR         15
#define CHEAP_TFT_CD_RS      16      // needed by ts - labelled "RS" on board
#define CHEAP_TFT_CS         17      // needed by ts
#define CHEAP_TFT_RESET      18


#if WITH_CHEAP_TFT
    // Cheap Ardino 3.5" 320x480 TFT's
    // Uses my modified version of LCDWIKI, which
    // - has HARDWIRED (though arbitray) pin numbers
    //   in myLCDWIKI_KBV/LCDWikiStuff_for_teensy.cpp
    // - and very slightly modified version of myILI9431_t3,
    //   which broke the font definition out of ILI9341_t3.h
    //   for use in myLCDWIKI_GUI/LCDWIKI_GUI.cpp which
    //   implements fonts.

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
    
    #define TFT_BLACK   0x0000
    #define TFT_BLUE    0x001F
    #define TFT_RED     0xF800
    #define TFT_GREEN   0x07E0
    #define TFT_CYAN    0x07FF
    #define TFT_MAGENTA 0xF81F
    #define TFT_YELLOW  0xFFE0
    #define TFT_WHITE   0xFFFF
    
    // need a calibration routine
    
    int minx=240;
    int maxx=920;
    int miny=90;
    int maxy=860;

#endif


#if WITH_TOUCH
    // Cheap Ardino 3.5" 320x480 TFT's
    
    #include <stdint.h>
    #include <Arduino.h>
    #include "TouchScreen.h"    // modified to (at least) reset pinModes
    
    // Do not confuse the CD flash drive pins for the touch screen.
    // This device uses direct measurements of resistance and very
    // touchy, pun intended, modified version of TouchScreen.h which:
    // - uses digitalWrite instead of portReg stuff
    // - softened up the validity checks
    // - reset all the pinModes when finished
    
    // define   my definition    // pin number   // arduino pin number and notes    // general notes
    #define YP  CHEAP_TFT_CD_RS  // 17=A4        // A2 maps to LCD_RS on arduino    // must be an analog pin, use "An" notation!
    #define XM  CHEAP_TFT_CS     // 16=A2        // A3 maps to LCD_CS on arduino    // must be an analog pin, use "An" notation!
    #define YM  CHEAP_TFT_DATA0  // 14=A0        // 8  maps to LCD_D0 on arduino    // can be a digital pin
    #define XP  CHEAP_TFT_DATA1  // 13           // 9  maps to LCD_D1 on arduino    // can be a digital pin
    
    // DO NOT USE THE "ohm" modifier which only affects the math.
    
    TouchScreen ts = TouchScreen(XP, YP, XM, YM);   // , 300);
#endif


#if WITH_ROTARY

    #define ROTARY_IRQS   0
        // IRQs did NOT work at all like I'd hoped
        // they are far too noisy, and fast, at least for breadboard
        // circuit.  May try again when soldered.
    
    #define DEBUG_ROTARY  1
    
    #define ROTARY_1A   10     // 2     // mashed up pin assignments
    #define ROTARY_1B   9      // 3 
    #define ROTARY_2A   12     // 4                          
    #define ROTARY_2B   11     // 6                          
    #define ROTARY_3A   4      // 9     // this one is wired differently than the others
    #define ROTARY_3B   6      // 10
    #define ROTARY_4A   3      // 11
    #define ROTARY_4B   2      // 12
    
    int pollA[4] = {0,0,0,0};
    int rotaryA[4] = {ROTARY_1A,ROTARY_2A,ROTARY_3A,ROTARY_4A};
    int rotaryB[4] = {ROTARY_1B,ROTARY_2B,ROTARY_3B,ROTARY_4B};
    int rotary_vslue[4] = {0,0,0,0};
    
    void initRotary()
    {
        for (int i=0; i<4; i++)
        {
            pinMode(rotaryA[i],INPUT_PULLDOWN);
            pinMode(rotaryB[i],INPUT_PULLDOWN);
        }
        
        #if ROTARY_IRQS        
            attachInterrupt(rotaryA[0],rotaryIRQ0,CHANGE);
            attachInterrupt(rotaryA[1],rotaryIRQ1,RISING);
            attachInterrupt(rotaryA[2],rotaryIRQ2,RISING);
            attachInterrupt(rotaryA[3],rotaryIRQ3,RISING);
        #endif
    }
    
    #if ROTARY_IRQS
        void rotaryIRQ0()   { rotaryIRQ(0); }
        void rotaryIRQ1()   { rotaryIRQ(1); }
        void rotaryIRQ2()   { rotaryIRQ(2); }
        void rotaryIRQ3()   { rotaryIRQ(3); }

        void rotaryIRQ(int i)
    #else
        void pollRotary()
        {
            for (int i=0; i<4; i++)
                pollRotary(i);
        }

        void pollRotary(int i)
    #endif
    {
        int aval = digitalRead(rotaryA[i]);
        if (pollA[i] == aval)
            return;
            
        // only do something if A has changed
        
        pollA[i] = aval;
        // delay(1);
        
        // precision optimization
        // the indents on mine are one full cycle
        // so the number ALWAYS changes by 2 on a single click
        // if you want a detent to equal one inc/dec, include the
        // next line
        
        #if 0   // only check every other pulse
            if (aval)
        #endif
        
        {
            int bval = digitalRead(rotaryB[i]);
            if (aval == bval)
                rotary_vslue[i]++;
            else // if (aval && bval)
                rotary_vslue[i]--;
            #if DEBUG_ROTARY
                display(0,"rotaryIRQ(%d) aval=%d bval=%d   value=%d",i,aval,bval,rotary_vslue[i]);
            #endif
        }
    }

#endif


//-------------------------------------------
// not currently used in project
//-------------------------------------------

#if WITH_TFT

    // The pins with their "standard" and alternate mappings are as follows
    
    //  1. VCC	      VIN	      Power: 3.6 to 5.5 volts
    //  2. GND	      GND	      
    //  3. CS	      10	      Alternate Pins: 9, 15, 20, 21
    //  4. RESET	  +3.3V	      
    //  5. D/C	      9	          Alternate Pins: 10, 15, 20, 21
    //  6. SDI(MOSI)  11 (DOUT)	  
    //  7. SCK	      13 (SCK)	  
    //  8. LED	      VIN	      Use 100 ohm resistor
    //
    //  MISO only needed for diagnostics? and touch screen
    //  Can get by with an 8 pin connector
    //
    //  9. SDO(MISO)  12 (DIN)	  
    // 10. T_CLK      13 (SCK)	  
    // 11. T_CS	      8           Alternate: any digital pin  CONFLICT WITH 8, trying 35
    // 12. T_DIN      11 (DOUT)	  
    // 13. T_DO	      12 (DIN)	  
    // 14. T_IRQ      2	          Optional: can use any digital pin   
    
    #include "SPI.h"
    #include "ILI9341_t3.h"
    
    #include <font_Arial.h>
    // #include <font_ArialBold.h>
    // #include <font_ArialItalic.h>
    // #include <font_ArialBoldItalic.h>    
   
    // only named pins used by tft display
    // others are default SPI library, vcc, ground, etc
    
    #define TFT_CS 10
    #define TFT_DC  9
    
    ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
    
#endif


#if WITH_TOUCH_XPT2046
    #include <XPT2046_Touchscreen.h>
    #include <SPI.h>

    #define CS_PIN  10
    // MOSI=11, MISO=12, SCK=13
    
    XPT2046_Touchscreen ts(CS_PIN);

    //#define TIRQ_PIN  2
    //XPT2046_Touchscreen ts(CS_PIN);  // Param 2 - NULL - No interrupts
    //XPT2046_Touchscreen ts(CS_PIN, 255);  // Param 2 - 255 - No interrupts
    //XPT2046_Touchscreen ts(CS_PIN, TIRQ_PIN);  // Param 2 - Touch IRQ Pin - interrupt enabled polling
#endif
    



//-----------------------------------------------------------
// setup
//-----------------------------------------------------------

#if WITH_SYSTEM
    expSystem *s_pTheSystem;
#endif


void setup()
{
    Serial.begin(115200);

    #if WITH_MIDI_HOST
        // Wait 1.5 seconds before turning on USB Host.  If connected USB devices
        // use too much power, Teensy at least completes USB enumeration, which
        // makes isolating the power issue easier.
    
        delay(1500);
        display(0,"Initializing USB Hos ....",0);
        delay(10);
        myusb.begin();
        midi1.setHandleNoteOn(myNoteOn);
        midi1.setHandleNoteOff(myNoteOff);
    #endif    

    display(0,"teensyExpression %s started ..",VERSION);
    
    #if WITH_SYSTEM    
        initLEDs();
        LEDFancyStart();
    #endif

    #if WITH_CHEAP_TFT
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
        mylcd.setFont(Arial_32);
        mylcd.Set_Text_Size(4);
        mylcd.Print_String("teensyExpression", 55, 60);

        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.setFont(Arial_24);
        mylcd.Set_Text_Size(2);
        mylcd.Print_String("version ", 60, 110);
        mylcd.print(VERSION);
        mylcd.print(" initializing ...");
    #endif
    
    #if WITH_TFT
        // display is 320x240
        tft.begin();
        tft.setRotation(3);
        tft.fillScreen(ILI9341_BLACK);
        tft.setTextColor(ILI9341_WHITE);
        // tft.setTextSize(3);
        tft.setCursor(30, 20);
        tft.setFont(Arial_24);
        tft.print("teensyExpression");
        tft.setCursor(40, 60);
        tft.setFont(Arial_16);
        tft.setTextColor(ILI9341_YELLOW);
        tft.print("version ");
        tft.print(VERSION);
        tft.print("initializing ...");
    #endif
    
    
    #if WITH_SYSTEM        
        for (int x=0; x<100; x++)
        {
            float bright = ((float)x)/100.00 * 70;
            setLEDBrightness((int)bright);
            showLEDs();    
            
            #if WITH_CHEAP_TFT
                float use_x = (x * 360.00) / 101;
                int use_w = 5;
                mylcd.Fill_Rect(use_x+50,180,use_w,30, TFT_BLUE);
            #endif
            
            #if WITH_TFT
                int use_x = x * 260 / 101;
                int use_w = 3;
                tft.fillRect(use_x+30, 120, use_w, 30, ILI9341_BLUE);
            #endif
            
            delay(15);
        }
        
        delay(200);

        #if WITH_CHEAP_TFT
            mylcd.Fill_Rect(40,110,400,100,0);
            mylcd.Print_String("Ready", 180, 110);
            mylcd.setDefaultFont();
            mylcd.Set_Text_Size(2);
        #endif
        
        #if WITH_TFT
            tft.fillRect(0, 60, 320, 180, ILI9341_BLACK);
            tft.setCursor(110, 60);
            tft.println("Ready");
        #endif
        
        #if WITH_SYSTEM
            clearLEDs();
            showLEDs();
        #endif
        
    #endif

    #if WITH_MIDI_HOST && WITH_TFT
        delay(1200);
        tft.fillScreen(ILI9341_BLACK);
        tft.setFont(Arial_10);
        tft.setCursor(0,0);
    #endif        

    #if WITH_SYSTEM
        s_pTheSystem = new expSystem;
        s_pTheSystem->addConfig(new oldRigConfig(s_pTheSystem));
        s_pTheSystem->addConfig(new testConfig(s_pTheSystem));
        s_pTheSystem->begin();
    #endif
    
    #ifdef USE_SERIAL_TO_RPI
        Serial3.begin(115200);
        display(0,"starting Serial3 to rPi",0);
        Serial3.println("teensy expression Serial3 to rPi started");
    #endif
    
    #if WITH_TOUCH_XPT2046
        ts.begin();
        ts.setRotation(3);
    #endif
    
    #if WITH_ROTARY
        initRotary();
    #endif
}





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
        s_pTheSystem->task();
    #endif
    
    #if 0 && WITH_CHEAP_TFT
        static int counter = 0;
        mylcd.Set_Text_Cursor(170,200);
        mylcd.print(counter++,DEC);
    #endif

    #if WITH_TOUCH_XPT2046
        if (ts.touched())
        {
            TS_Point p = ts.getPoint();
            display(0,"p=%-5d  x=%-5d  y=%-5d",p.z,p.x,p.y);
            delay(100);

        }
    #endif
    
    
    #if WITH_TOUCH
        #if WITH_CHEAP_TFT
            static elapsedMillis clear_it = 0;
            if (clear_it > 3000)
            {
                mylcd.Fill_Screen(0);
                clear_it = 0;
            }
        #endif        
        
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
            
            #if WITH_CHEAP_TFT
                int use_x = ((1-myx) * 480);
                int use_y = ((1-myy) * 320);
                mylcd.Print_String("o",use_x,use_y);
            #endif
            
            #if 1
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
        
        // delay(8);
    #endif
    
    #if WITH_ROTARY
        #if !ROTARY_IRQS
            pollRotary();
        #endif
        
        static int last_rotary_value[4] = {0,0,0,0};
        for (int i=0; i<4; i++)
        {
            if (last_rotary_value[i] != rotary_vslue[i])
            {
                last_rotary_value[i] = rotary_vslue[i];
                display(0,0,"loop(): rotary[%d] changed to %d",i,rotary_vslue[i]);
                #if WITH_CHEAP_TFT
                    mylcd.Set_Text_Cursor(300,20 + i*50);
                    mylcd.print(rotary_vslue[i],DEC);
                #endif
            }
        }
    #endif
    
}






//---------------------------------------
// midi HOST handlers
//---------------------------------------


#if WITH_MIDI_HOST

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
