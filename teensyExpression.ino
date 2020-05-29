// leds.show() with 30 takes about 2.2 ms
// initial design for touch screen ... LCD uses 

#include <myDebug.h>
#include "myLeds.h"
#include "expSystem.h"
#include "oldRigConfig.h"
#include "testConfig.h"

#define WITH_TFT              1      // Teensy 320x400 ILI9341/XPT2046 touch screen
#define WITH_MIDI_HOST        1


// note I re-ordered to BUTTON_OUT pins
// and moved the touch IRQ to 14

//----------------------------------------------------------------------
// DETAILS
//----------------------------------------------------------------------
// PINS FOR ALL              LCD PIN
//
//     2    TOUCH_IRQ           2     Can use any digital pin
//     3    BUTTON_OUT_LATCH
//     4    BUTTON_OUT_CLOCK
//     5    LEDS_OUT                  1, 5, -8, -10, (31)
//     6    BUTTON_OUT_DATA
//     7    BUTTON_IN_LOAD
//     8    TOUCH_CS           11     Can use any digital pin 
//     9    LCD_DC              5
//    10    LCD_CS              3
//    11    LCD_SDI (MOSI)      6
//          TOUCH_DIN          12
//    12    LCD_SDO (MISO)      9
//          TOUCH_DO           13
//    13    LCD_SCK             7
//          TOUCH_CLK          10
//    14    BUTTON_DIN                A0
//    15    BUTTON_IN_CLOCK_ENABLE    A1
//    16    BUTTON_IN_CLOCK           A2
//    17    BUTTON_IN_DATA            A3
//   -18    LIQUID_SDA0               A4     
//   -19    LIQUID_SCL0               A5
//    20    PIN_EXPR1                 A6
//    21    PIN_EXPR2                 A7
//    22    PIN_EXPR1                 A8
//    23    PIN_EXPR2                 A9



#if WITH_TFT
    // The pins with their "standard" and alternate mappings are as follows
    
    //  1. VCC	      VIN	      VIN	   Power: 3.6 to 5.5 volts
    //  2. GND	      GND	      GND	
    //  3. CS	      10	      21	   Alternate Pins: 9, 15, 20, 21
    //  4. RESET	  +3.3V	      +3.3V	
    //  5. D/C	      9	          20	   Alternate Pins: 10, 15, 20, 21
    //  6. SDI(MOSI)  11 (DOUT)	  7	
    //  7. SCK	      13 (SCK)	  14	
    //  8. LED	      VIN	      VIN	   Use 100 ohm resistor
    //  9. SDO(MISO)  12 (DIN)	  12	
    // 10. T_CLK      13 (SCK)	  14	
    // 11. T_CS	      8	          8	       Alternate: any digital pin
    // 12. T_DIN      11 (DOUT)	  7	
    // 13. T_DO	      12 (DIN)	  12	
    // 14. T_IRQ      2	          2	       Optional: can use any digital pin   
    
    #include "SPI.h"
    #include "ILI9341_t3.h"
    
    #include <font_Arial.h>
    #include <font_ArialBold.h>
    #include <font_ArialItalic.h>
    #include <font_ArialBoldItalic.h>    
   
    // We Connect
    // CS to 10
    // DC to 9
    // SDI to 11
    // SCK to 13
    // SDO to 12
    
    // only named pins in program 
    
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

expSystem *s_pTheSystem;


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

    display(0,"teensyExpression 1.0 started ..",0);
    
    initLEDs();
    LEDFancyStart();
    
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
        tft.print("version 1.0 initializing ...");
        
        for (int x=0; x<260; x++)
        {
            float bright = ((float)x)/260.00 * 70;
            setLEDBrightness((int)bright);
            showLEDs();    

            tft.fillRect(x+30, 120, 1, 30, ILI9341_BLUE);
            delay(15);
        }
        
        delay(200);
        tft.fillRect(0, 60, 320, 180, ILI9341_BLACK);
        tft.setCursor(110, 60);
        tft.println("Ready");
        clearLEDs();
        showLEDs();    
        delay(1200);
        tft.fillScreen(ILI9341_BLACK);
        tft.setFont(Arial_10);
        tft.setCursor(0,0);
        
    #endif

    s_pTheSystem = new expSystem;
    s_pTheSystem->addConfig(new oldRigConfig(s_pTheSystem));
    s_pTheSystem->addConfig(new testConfig(s_pTheSystem));
    s_pTheSystem->begin();
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

    s_pTheSystem->task();
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
