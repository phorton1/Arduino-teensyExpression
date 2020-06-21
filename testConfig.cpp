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
            
        
    }        
}
