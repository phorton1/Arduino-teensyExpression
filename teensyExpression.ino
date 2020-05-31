
#include <myDebug.h>

#define WITH_SYSTEM           1
#define WITH_TFT              0      // Teensy 320x400 ILI9341/XPT2046 touch screen
#define WITH_MIDI_HOST        0
#define WITH_CHEAP_TFT        1

#define VERSION   "1.2"

#if WITH_SYSTEM
    #include "myLeds.h"
    #include "expSystem.h"
    #include "oldRigConfig.h"
    #include "testConfig.h"
#endif


// Note:  Resistive touch screens are always problematic
//        and this one, even with Pauls most basic example code
//        is no exception.  Since I have plenty of buttons,
//        my decision is to NOT use the touch screen (which
//        also confliced with the use of pin8 for Serial3)
// 
//----------------------------------------------------------------------
// PIN USAGE
//----------------------------------------------------------------------
//
//     unused LEDS    BUTTONS   SERIAL   EXPR     LIQUID    TFT      TOUCH    CHEAP_TFT
//   2                                                                IRQ      CHEAP_DATA0
//   3                                                                         CHEAP_DATA1
//   4                                                                         CHEAP_DATA2
//   5        LEDS_OUT
//   6                                                                         CHEAP_DATA3
//   7                          SERIAL_RX3
//   8                          SERIAL_TX3                             T_CS
//   9                                                      DC                 CHEAP_DATA4
//  10                                                      CS                 CHEAP_DATA5
//  11                                                      SDI(MOSI)  T_DIN   CHEAP_DATA6
//  12                                                      SDO(MISO)  T_DOUT  CHEAP_DATA7
//  13                                                      CLK        T_CLK
//  14 A0 x 
//  15 A1 x    
//  16 A2 x    
//  17 A3 x    
//  18 A4 y                                          SDA0
//  19 A5 y                                          SCL0
//  20 A6                                  EXPR1
//  21 A7                                  EXPR2
//  22 A8                                  EXPR3                 
//  23 A9                                  EXPR4
//  24                 BUTTON_OUT0
//  25                 BUTTON_OUT1
//  26                 BUTTON_OUT2
//  27                 BUTTON_OUT3
//  28                 BUTTON_OUT4
//  29                 BUTTON_IN0
//  30                 BUTTON_IN1
//  31 A12             BUTTON_IN2
//  32 A13             BUTTON_IN3
//  33 A14             BUTTON_IN4
//  34 A15                                                                     CHEAP_RD
//  35 A16                                                                     CHEAP_WR
//  36 A17                                                                     CHEAP_CD(RS)
//  37 A18                                                                     CHEAP_CS
//  38 A19                                                                     CHEAP_RESET
//  39 A20 x   
//     A21 x   
//     A22 x   


#if WITH_CHEAP_TFT
    #include <LCDWIKI_GUI.h> //Core graphics library
    #include <LCDWIKI_KBV.h> //Hardware-specific library
    
    LCDWIKI_KBV mylcd(ILI9486,37,36,35,34,38); //model,cs,cd,wr,rd,reset
    
    #define TFT_BLACK   0x0000
    #define TFT_BLUE    0x001F
    #define TFT_RED     0xF800
    #define TFT_GREEN   0x07E0
    #define TFT_CYAN    0x07FF
    #define TFT_MAGENTA 0xF81F
    #define TFT_YELLOW  0xFFE0
    #define TFT_WHITE   0xFFFF
#endif


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
    //  Not using the touch screen.
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
    #include <font_ArialBold.h>
    #include <font_ArialItalic.h>
    #include <font_ArialBoldItalic.h>    
   
    // only named pins used by tft display
    // others are default SPI library, vcc, ground, etc
    
    #define TFT_CS 10
    #define TFT_DC  9
    
    ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
    
#endif


#if WITH_MIDI_HOST
    #include <USBHost_t36.h>
    
    USBHost myusb;
    USBHub hub1(myusb);
    USBHub hub2(myusb);
    MIDIDevice midi1(myusb);
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
        // display is 320x480
        
        mylcd.Init_LCD();
        mylcd.Set_Rotation(3);
        mylcd.Fill_Screen(0);
        mylcd.Set_Text_Mode(0);
        mylcd.Set_Text_colour(TFT_WHITE);
        mylcd.Set_Text_Back_colour(0); 

        mylcd.Set_Text_Size(4);
        mylcd.Print_String("teensyExpression", 30, 60);
        mylcd.Set_Text_colour(TFT_YELLOW);
        mylcd.Set_Text_Size(2);
        mylcd.Print_String("version", 60, 110);
        mylcd.Print_String(VERSION, 155, 110);
        mylcd.Print_String("initializing ...", 200, 110);
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
