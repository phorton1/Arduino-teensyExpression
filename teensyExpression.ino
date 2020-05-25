// leds.show() with 30 takes about 2.2 ms
// initial design for touch screen ... LCD uses 

#include <myDebug.h>

#define WITH_EXPRESSION  0
#define WITH_BUTTONS     1
#define WITH_LEDS        1
#define WITH_LIQUID      0       // Small 2x16 LCD with I2C port
#define WITH_TFT         0       // Teensy 320x400 ILI9341/XPT2046 touch screen
   

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


#if WITH_BUTTONS
    #include "rawButtonArray.h"
    
    void inoHandleButtonEvent(void *obj, int row, int col, int event)
    {
        display(0,"row(%d) col(%d) event(%s)",row,col,rawButtonArray::buttonEventName(event));
    }

    rawButtonArray buttons(0,inoHandleButtonEvent);
    
#endif


#if WITH_TFT
    // The pins with their "standard" and alternate mappings are as follows
    
    //  1. VCC	      VIN	      VIN	   Power: 3.6 to 5.5 volts
    //  2. GND	      GND	      GND	
    //  3. CS	      10	      21	   Alternate Pins: 9, 15, 20, 21
    //  4. RESET	      +3.3V	      +3.3V	
    //  5. D/C	      9	          20	   Alternate Pins: 10, 15, 20, 21
    //  6. SDI(MOSI)	  11 (DOUT)	  7	
    //  7. SCK	      13 (SCK)	  14	
    //  8. LED	      VIN	      VIN	   Use 100 ohm resistor
    //  9. SDO(MISO)	  12 (DIN)	  12	
    // 10. T_CLK	      13 (SCK)	  14	
    // 11. T_CS	      8	          8	       Alternate: any digital pin
    // 12. T_DIN	      11 (DOUT)	  7	
    // 13. T_DO	      12 (DIN)	  12	
    // 14. T_IRQ	      2	          2	       Optional: can use any digital pin   
    
    #include "SPI.h"
    #include "ILI9341_t3.h"
   
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



#if WITH_LIQUID
    #include <Wire.h>                 
    #include <LiquidCrystal_I2C.h>
    
    // default pins for Wire library output
    //
    //   18    SDA0
    //   18    SCL0
    
    
    #define LCD_ADDRESS  0x27
    // #define LCD_ADDRESS  (B0100<3)        // PCF8574/PCF8574T 
    // #define LCD_ADDRESS  (B0111<3)       // PCF8574A
       // address minus low three bits set by jumper
       
    // the example had 0x27, which works!!
    // b00101 110  which does not match either of the above
    
    LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
       // Configure LiquidCrystal_I2C library with 0x27 address,
       // 16 columns and 2 rows
    
    #if 1
        // can be 0..7 custom characters ... these are the 5 bits per row
    
        #if 1
            byte temp_char[8] = {B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111};
        #else
            byte temp_char[8] = {B01110, B01010, B01010, B01110, B01110, B11111, B11111, B01110};
        #endif
        byte hum_char[8] = {B00100, B01110, B11111, B00100, B10001, B00100, B10001, B00100};
        byte bell_Char[8] = {B00100, B01110, B01110, B01110, B11111, B11111, B00100, B00000};
        byte arrow_Char[8] = {B00000, B00000, B10000, B10000, B10111, B10011, B10101, B01000};
    #endif
#endif



#if WITH_LEDS
    #include <WS2812Serial.h>
    
    #define NUM_LEDS  30
    #define LED_PIN   5
    
    // Usable pins:
    //   Teensy LC:   1, 4, 5, 24
    //   Teensy 3.2:  1, 5, 8, 10, 31   (overclock to 120 MHz for pin 8)
    //   Teensy 3.5:  1, 5, 8, 10, 26, 32, 33, 48
    //   Teensy 3.6:  1, 5, 8, 10, 26, 32, 33
    
    byte drawingMemory[NUM_LEDS*3];         //  3 bytes per LED
    DMAMEM byte displayMemory[NUM_LEDS*12]; // 12 bytes per LED
    WS2812Serial leds(NUM_LEDS, displayMemory, drawingMemory, LED_PIN, WS2812_GRB);
    
    #if 0
        #define RED    0xFF0000
        #define GREEN  0x00FF00
        #define BLUE   0x0000FF
        #define YELLOW 0xFFFF00
        #define PINK   0xFF1088
        #define ORANGE 0xE05800
        #define WHITE  0xFFFFFF
    
    #else // Less intense...
        #define RED    0x160000
        #define GREEN  0x001600
        #define BLUE   0x000016
        #define YELLOW 0x101400
        #define PINK   0x120009
        #define ORANGE 0x100400
        #define WHITE  0x101010
        #endif
    
    #define NUM_COLORS   6
    unsigned colors[NUM_COLORS] = {RED,GREEN,BLUE,YELLOW,PINK,ORANGE};
    int led_num = 0;
    int color_num = 0;
    elapsedMillis led_time = 0;
   
#endif   



#if WITH_EXPRESSION

    #define NUM_PEDALS   4
    
    #define PIN_EXPR1    20  // A6
    #define PIN_EXPR2    21  // A7
    #define PIN_EXPR1    22  // A8
    #define PIN_EXPR2    23  // A9
    
    #define CALIBRATE_TIME              6000
    #define CALIBRATE_BLINK_ON_TIME       20
    #define CALIBRATE_BLINK_OFF_TIME     150
    
    typedef struct
    {
        int     pin;
        const   char *name;
        int     calib_min;
        int     calib_max;
        
        int     cable;
        int     channel;
        int     cc_num;
        
        int     raw_value;
        
    }   expressionPedal_t;

    expressionPedal_t pedals[NUM_PEDALS] = {
        { PIN_EXPR1, "Pedal1", 9, 1010, 0, 1, 0x0F, 0},
        { PIN_EXPR2, "Pedal2", 9, 1010, 0, 1, 0x0E, 0},
    };

    int calibrate_pedal = -1;
    elapsedMillis calibrate_time = 0;    
    
#endif



//-----------------------------------------------------------
// setup
//-----------------------------------------------------------

void setup()
{
    Serial.begin(115200);
    delay(1200);
    display(0,"teensyExpression 1.2 started ..",0);
    
    #if WITH_BUTTONS
        for (int r=0; r<NUM_BUTTON_ROWS; r++)
            for (int c=0; c<NUM_BUTTON_COLS; c++)
                buttons.setButtonEventMask(r,c,
                
                #if 0
                    BUTTON_ALL_EVENTS);
                #else
                    0 |
                    BUTTON_EVENT_PRESS          | 
                    BUTTON_EVENT_RELEASE        | 
                    BUTTON_EVENT_CLICK          | 
                    (r==2 ? BUTTON_EVENT_DOUBLE_CLICK : 0)   | 
                    BUTTON_EVENT_LONG_CLICK     |
                    0 );
                #endif
        buttons.begin();
    #endif
    
    #if WITH_LEDS
        leds.begin();
        for (int i=0; i<NUM_LEDS; i++)
        {
            leds.setPixel(i, 0);
        }
        
        #if 0
            led_time = 0;
            for (int i=0; i<100; i++)
            {
                leds.show();
            }
            int tm = led_time;
            display(0,"100 leds.show() calls took %d ms",tm);
        #endif
        
        leds.show();
    #endif
    
    
    #if WITH_LIQUID
        lcd.init();                        // Initialize I2C LCD module
        #if 1
            lcd.createChar(0, temp_char);
            lcd.createChar(6, hum_char);
            // lcd.createChar(8, bell_Char);
            lcd.createChar(11, arrow_Char);
        #endif
        lcd.backlight();                   // Turn backlight ON
        lcd.setCursor(0, 0);               // Go to column 0, row 0
        lcd.print("Hello, world!");
        lcd.setCursor(0, 1);               // Go to column 0, row 1
        #if 1
            lcd.write(0);
            lcd.write(6);
            lcd.write(8);
            lcd.write(11);
        #else
            lcd.print("Arduino I2C LCD");
        #endif
    #endif
    
    #if WITH_TFT
        tft.begin();
        tft.fillScreen(ILI9341_BLACK);
        tft.setTextColor(ILI9341_YELLOW);
        tft.setTextSize(2);
        tftDiagnostics();
    #endif
    
}


//-----------------------------------------
// loop
//-----------------------------------------

void loop()
{
    #if WITH_BUTTONS
        #if !USE_BUTTON_SCAN_TIMER
            buttons.task();
        #endif
    #endif
    
    #if WITH_EXPRESSION
        expressionTask();
    #endif
    
    #if WITH_LEDS
        if (led_time > 1000)  
        {
            for (int i=0; i<NUM_LEDS; i++)
            {
            leds.setPixel(i, colors[color_num]);
            }
            leds.show();
            color_num = (color_num+1) % NUM_COLORS;
            led_time = 0;
            }
    #endif
    
    #if WITH_TFT
        for(uint8_t rotation=0; rotation<4; rotation++)
        {
            tft.setRotation(rotation);
            testText();
            delay(1000);
        }      
    #endif
}



#if WITH_EXPRESSION
    void expressionTask()
    {
    
    }
#endif


//--------------------------------------------
// TFT stuff
//--------------------------------------------

#if WITH_TFT

void tftDiagnostics()
{
   Serial.println("ILI9341 Test!"); 
   
   // read diagnostics (optional but can help debug problems)
   uint8_t x = tft.readcommand8(ILI9341_RDMODE);
   Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
   x = tft.readcommand8(ILI9341_RDMADCTL);
   Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
   x = tft.readcommand8(ILI9341_RDPIXFMT);
   Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
   x = tft.readcommand8(ILI9341_RDIMGFMT);
   Serial.print("Image Format: 0x"); Serial.println(x, HEX);
   x = tft.readcommand8(ILI9341_RDSELFDIAG);
   Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
   
   Serial.println(F("Benchmark                Time (microseconds)"));
   
   Serial.print(F("Screen fill              "));
   Serial.println(testFillScreen());
   delay(200);
   
   Serial.print(F("Text                     "));
   Serial.println(testText());
   delay(600);
   
   Serial.print(F("Lines                    "));
   Serial.println(testLines(ILI9341_CYAN));
   delay(200);
   
   Serial.print(F("Horiz/Vert Lines         "));
   Serial.println(testFastLines(ILI9341_RED, ILI9341_BLUE));
   delay(200);
   
   Serial.print(F("Rectangles (outline)     "));
   Serial.println(testRects(ILI9341_GREEN));
   delay(200);
   
   Serial.print(F("Rectangles (filled)      "));
   Serial.println(testFilledRects(ILI9341_YELLOW, ILI9341_MAGENTA));
   delay(200);
   
   Serial.print(F("Circles (filled)         "));
   Serial.println(testFilledCircles(10, ILI9341_MAGENTA));
   
   Serial.print(F("Circles (outline)        "));
   Serial.println(testCircles(10, ILI9341_WHITE));
   delay(200);
   
   Serial.print(F("Triangles (outline)      "));
   Serial.println(testTriangles());
   delay(200);
   
   Serial.print(F("Triangles (filled)       "));
   Serial.println(testFilledTriangles());
   delay(200);
   
   Serial.print(F("Rounded rects (outline)  "));
   Serial.println(testRoundRects());
   delay(200);
   
   Serial.print(F("Rounded rects (filled)   "));
   Serial.println(testFilledRoundRects());
   delay(200);
   
   Serial.println(F("Done!"));
}


unsigned long testFillScreen()
{
    unsigned long start = micros();
    tft.fillScreen(ILI9341_BLACK);
    tft.fillScreen(ILI9341_RED);
    tft.fillScreen(ILI9341_GREEN);
    tft.fillScreen(ILI9341_BLUE);
    tft.fillScreen(ILI9341_BLACK);
    return micros() - start;
}

unsigned long testText()
{
    tft.fillScreen(ILI9341_BLACK);
    unsigned long start = micros();
    tft.setCursor(0, 0);
    tft.setTextColor(ILI9341_WHITE);  tft.setTextSize(1);
    tft.println("Hello World!");
    tft.setTextColor(ILI9341_YELLOW); tft.setTextSize(2);
    tft.println(1234.56);
    tft.setTextColor(ILI9341_RED);    tft.setTextSize(3);
    tft.println(0xDEADBEEF, HEX);
    tft.println();
    tft.setTextColor(ILI9341_GREEN);
    tft.setTextSize(5);
    tft.println("Groop");
    tft.setTextSize(2);
    tft.println("I implore thee,");
    tft.setTextSize(1);
    tft.println("my foonting turlingdromes.");
    tft.println("And hooptiously drangle me");
    tft.println("with crinkly bindlewurdles,");
    tft.println("Or I will rend thee");
    tft.println("in the gobberwarts");
    tft.println("with my blurglecruncheon,");
    tft.println("see if I don't!");
    return micros() - start;
}

unsigned long testLines(uint16_t color)
{
    unsigned long start, t;
    int           x1, y1, x2, y2,
                  w = tft.width(),
                  h = tft.height();
    
    tft.fillScreen(ILI9341_BLACK);
    
    x1 = y1 = 0;
    y2    = h - 1;
    start = micros();
    for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
    x2    = w - 1;
    for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
    t     = micros() - start; // fillScreen doesn't count against timing
    
    tft.fillScreen(ILI9341_BLACK);
    
    x1    = w - 1;
    y1    = 0;
    y2    = h - 1;
    start = micros();
    for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
    x2    = 0;
    for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
    t    += micros() - start;
    
    tft.fillScreen(ILI9341_BLACK);
    
    x1    = 0;
    y1    = h - 1;
    y2    = 0;
    start = micros();
    for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
    x2    = w - 1;
    for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
    t    += micros() - start;
    
    tft.fillScreen(ILI9341_BLACK);
    
    x1    = w - 1;
    y1    = h - 1;
    y2    = 0;
    start = micros();
    for(x2=0; x2<w; x2+=6) tft.drawLine(x1, y1, x2, y2, color);
    x2    = 0;
    for(y2=0; y2<h; y2+=6) tft.drawLine(x1, y1, x2, y2, color);
    
    return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2)
{
    unsigned long start;
    int           x, y, w = tft.width(), h = tft.height();
    
    tft.fillScreen(ILI9341_BLACK);
    start = micros();
    for(y=0; y<h; y+=5) tft.drawFastHLine(0, y, w, color1);
    for(x=0; x<w; x+=5) tft.drawFastVLine(x, 0, h, color2);
    
    return micros() - start;
}

unsigned long testRects(uint16_t color)
{
    unsigned long start;
    int n, i, i2,
        cx = tft.width()  / 2,
        cy = tft.height() / 2;
    
    tft.fillScreen(ILI9341_BLACK);
    n     = min(tft.width(), tft.height());
    start = micros();
    for(i=2; i<n; i+=6)
    {
        i2 = i / 2;
        tft.drawRect(cx-i2, cy-i2, i, i, color);
    }
    
    return micros() - start;
}


unsigned long testFilledRects(uint16_t color1, uint16_t color2)
{
    unsigned long start, t = 0;
    int n, i, i2,
        cx = tft.width()  / 2 - 1,
        cy = tft.height() / 2 - 1;
    
    tft.fillScreen(ILI9341_BLACK);
    n = min(tft.width(), tft.height());
    for(i=n; i>0; i-=6)
    {
        i2    = i / 2;
        start = micros();
        tft.fillRect(cx-i2, cy-i2, i, i, color1);
        t    += micros() - start;
        // Outlines are not included in timing results
        tft.drawRect(cx-i2, cy-i2, i, i, color2);
    }
    return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color)
{
    unsigned long start;
    int x, y, w = tft.width(), h = tft.height(), r2 = radius * 2;
    
    tft.fillScreen(ILI9341_BLACK);
    start = micros();
    for(x=radius; x<w; x+=r2)
    {
        for(y=radius; y<h; y+=r2)
        {
            tft.fillCircle(x, y, radius, color);
        }
    }
    
    return micros() - start;
}


unsigned long testCircles(uint8_t radius, uint16_t color)
{
    unsigned long start;
    int x, y, r2 = radius * 2,
        w = tft.width()  + radius,
        h = tft.height() + radius;
    
    // Screen is not cleared for this one -- this is
    // intentional and does not affect the reported time.
    start = micros();
    for(x=0; x<w; x+=r2)
    {
        for(y=0; y<h; y+=r2)
        {
            tft.drawCircle(x, y, radius, color);
        }
    }

    return micros() - start;
}


unsigned long testTriangles()
{
    unsigned long start;
    int n, i,
        cx = tft.width()  / 2 - 1,
        cy = tft.height() / 2 - 1;
    
    tft.fillScreen(ILI9341_BLACK);
    n     = min(cx, cy);
    start = micros();
    for(i=0; i<n; i+=5)
    {
        tft.drawTriangle(
        cx    , cy - i, // peak
        cx - i, cy + i, // bottom left
        cx + i, cy + i, // bottom right
        tft.color565(0, 0, i));
    }

    return micros() - start;
}

unsigned long testFilledTriangles()
{
    unsigned long start, t = 0;
    int i,
        cx = tft.width()  / 2 - 1,
        cy = tft.height() / 2 - 1;
    
    tft.fillScreen(ILI9341_BLACK);
    start = micros();
    for(i=min(cx,cy); i>10; i-=5)
    {
        start = micros();
        tft.fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
        tft.color565(0, i, i));
        t += micros() - start;
        tft.drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
        tft.color565(i, i, 0));
    }

    return t;
}


unsigned long testRoundRects()
{
    unsigned long start;
    int w, i, i2,
        cx = tft.width()  / 2 - 1,
        cy = tft.height() / 2 - 1;
    
    tft.fillScreen(ILI9341_BLACK);
    w     = min(tft.width(), tft.height());
    start = micros();
    for(i=0; i<w; i+=6)
    {
        i2 = i / 2;
        tft.drawRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(i, 0, 0));
    }

    return micros() - start;
}

unsigned long testFilledRoundRects()
{
    unsigned long start;
    int i, i2,
        cx = tft.width()  / 2 - 1,
        cy = tft.height() / 2 - 1;
        
    tft.fillScreen(ILI9341_BLACK);
    start = micros();
    for(i=min(tft.width(), tft.height()); i>20; i-=6)
    {
        i2 = i / 2;
        tft.fillRoundRect(cx-i2, cy-i2, i, i, i/8, tft.color565(0, i, 0));
    }
    
    return micros() - start;
}

#endif // WITH_TFT

