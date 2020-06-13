
#include "myLeds.h"
#include "defines.h"
#include <WS2812Serial.h>


//---------------------------------
// LEDS
//---------------------------------
// Usable pins:
//
//   Teensy LC:   1, 4, 5, 24
//   Teensy 3.2:  1, 5, 8, 10, 31   (overclock to 120 MHz for pin 8)
//   Teensy 3.5:  1, 5, 8, 10, 26, 32, 33, 48
//   Teensy 3.6:  1, 5, 8, 10, 26, 32, 33

#define LED_PIN          5
    // Note that choosing to use pin 5 eats up Serial1
    // hence why we have to use Serial3 in teensyExpression.ino
    // Pauls Notes:
    //
    //      Non-blocking performance does come with a cost.  15 bytes of memory are required
    //      per LED, rather than the usual 3 bytes with [FastLED](http://fastled.io/) or
    //      [Adafruit NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel).  One of
    //      the [hardware serial ports](https://www.pjrc.com/teensy/td_uart.html) is also
    //      used to transmit the data, making that port unavailable for other uses.
    //      
    //      ## Supported Pins & Serial Ports
    //      
    //      | Port    | Teensy LC   | Teensy 3.2 | Teensy 3.5 | Teensy 3.6 |
    //      | :------ | :---------: | :--------: | :--------: | :--------: |
    //      | Serial1 | 1, 4, 5, 24 | 1, 5       | 1, 5, 26   | 1, 5, 26   |
    //      | Serial2 |             | 10, 31     | 10         | 10         |
    //      | Serial3 |             | 8          | 8          | 8          |
    //      | Serial4 |             |            | 32         | 32         |
    //      | Serial5 |             |            | 33         | 33         |
    //      | Serial6 |             |            | 48         |            |
    //      
    //      Serial2 & Serial3 on Teensy LC are not supported, due to lack of configurable
    //      oversampling needed to run at the high speed required.
    //      
    //      Serial3-Serial6 should be used only with CPU speeds 120 or 180 MHz.
    //      
    //      Serial6 on Teensy 3.6 is not currently supported, due to different hardware
    //      registers.    
    
#define NUM_LEDS         (NUM_BUTTON_ROWS * NUM_BUTTON_COLS)


unsigned drawingMemory[NUM_LEDS];         
byte renderingMemory[NUM_LEDS*3];         //  3 bytes per LED
DMAMEM byte displayMemory[NUM_LEDS*12]; // 12 bytes per LED
WS2812Serial leds(NUM_LEDS, displayMemory, renderingMemory, LED_PIN, WS2812_GRB);

unsigned brightness = 100;
bool leds_changed = 0;

void initLEDs()
{
    leds.begin();
}

int getLEDBrightness()
{
    return brightness;    
}

void setLEDBrightness(int i)   // 0..100
{
    brightness = i;
    showLEDs(true);
}


void setLED(int num, unsigned color)
{
    int row = num / NUM_BUTTON_COLS;
    int col = num % NUM_BUTTON_COLS;
    setLED(row,col,color);
}


void setLED(int row, int col, unsigned color)
    //  0, 1, 2, 3, 4
    //  9, 8, 7, 6, 5
    // 10,11,12,13,14
    // 19,18,17,16,15
    // 20,21,22,23,24
{
    int i;
    if (row & 1)
    {
        i = (row+1) * NUM_BUTTON_COLS - col - 1;
    }
    else
    {
        i = row * NUM_BUTTON_COLS + col; 
    }
    drawingMemory[i] = color;
    leds_changed = 1;
}



void showLEDs(bool force)
{
    if (force || leds_changed)
    {
        for (int i=0; i<NUM_LEDS; i++)
        {
            unsigned c = drawingMemory[i];
            unsigned r = (c >> 16) * brightness;
            unsigned g = ((c >> 8) & 0xff) * brightness;
            unsigned b = (c & 0xff) * brightness;
            r /= 100;
            g /= 100;
            b /= 100;
            leds.setPixel(i,(r<<16)+(g<<8)+b);
        }
        leds.show();
    }
    leds_changed = false;
}

void LEDFancyStart()
{
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            float c = col;
            float r = row;
            
            float red = (c/4) * 255.0;
            float blue = ((4-c)/4) * 255.0;
            float green = (r/4) * 255.0;
            
            unsigned rr = red;
            unsigned gg = green;
            unsigned bb = blue;
            
            setLED(row,col,(rr << 16) + (gg << 8) + bb);
        }
    }
}


void clearLEDs()
{
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            setLED(row,col,0);
        }
    }
}