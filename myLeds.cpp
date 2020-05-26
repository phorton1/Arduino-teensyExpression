
#include "myLeds.h"
#include "rawButtonArray.h"
//#include <Arduino.h>
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

void setLEDBrightness(unsigned i)   // 0..100
{
    brightness = i;
    showLEDs(true);
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

