#include <myDebug.h>
#include "testConfig.h"
#include "myTFT.h"
#include "myLeds.h"

bool draw_needed = false;


// virtual
void testConfig::begin()
{
    expConfig::begin();
    draw_needed = true;
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
    
    showLEDs();
}



#define LINE_HEIGHT  20
int ypos = 0;

void printCentered(const char *s)
{
    mylcd.setFont(Arial_16);
    mylcd.printf_justified(
        0,
        ypos,
        480,
        LINE_HEIGHT,
        LCD_JUST_CENTER,
        TFT_WHITE,
        TFT_BLACK,
        "%s",
        s);
    ypos += LINE_HEIGHT;
}


// virtual
void testConfig::updateUI()
{
    if (draw_needed)
    {
        draw_needed = false;

        mylcd.setFont(Arial_28);

        mylcd.printf_justified(
            0,
            60,
            480,
            30,
            LCD_JUST_CENTER,
            TFT_YELLOW,
            TFT_BLACK,
            "teensyExpression");

        mylcd.setFont(Arial_16);

        mylcd.printf_justified(
            0,
            100,
            480,
            20,
            LCD_JUST_CENTER,
            TFT_YELLOW,
            TFT_BLACK,
            "version %s",
            VERSION);
            
        ypos = 145;
        
        #if defined(USB_SERIAL)
            printCentered("USB_SERIAL");
        #elif defined(USB_MIDI)
            printCentered("USB_MIDI");
        #elif defined(USB_MIDI_SERIAL)
            printCentered("USB_MIDI_SERIAL");
        #else
            printCentered("NO MIDI OR SERIAL!!");
        #endif
        

        #if WITH_ROTARY
            printCentered("WITH ROTARY");
        #else
            printCentered("NO ROTARY!!");
        #endif


        #if WITH_PEDALS
            printCentered("WITH PEDALS");
        #else
            printCentered("NO PEDALS!!");
        #endif
    
        
        #if WITH_TOUCH
            printCentered("WITH TOUCH");
        #else
            printCentered("NO TOUCH!!");
        #endif

        
        #if WITH_MIDI_HOST
            if (midi_host_on)
                printCentered("MIDI_HOST ON");
            else
                printCentered("MIDI_HOST OFF");
        #else
            printCentered("NO MIDI_HOST!!");
        #endif

        
        #if WITH_SERIAL_PORT
            if (serial_port_on)
                printCentered("SERIAL_PORT ON");
            else
                printCentered("SERIAL_PORT OFF");
        #else
            printCentered("NO SERIAL_IO_PORT!!");
        #endif
    
        
    }        
}
