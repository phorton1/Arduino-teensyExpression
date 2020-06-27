#include <myDebug.h>
#include "patchTest.h"
#include "myTFT.h"
#include "myLeds.h"

bool draw_needed = false;


#if 0
    const char *color_name(int i)
        // these are the corrected ansi namaes
    {
        if (i == 0x00) return "black" ;
        if (i == 0x01) return "red"   ;
        if (i == 0x02) return "green" ;
        if (i == 0x03) return "brown"  ;
        if (i == 0x04) return "blue"  ;
        if (i == 0x05) return "mag"   ;
        if (i == 0x06) return "cyan" ;
        if (i == 0x07) return "lgray" ;
        if (i == 0x08) return "gray"  ;
        if (i == 0x09) return "lred" ;
        if (i == 0x0a) return "lgreen";
        if (i == 0x0b) return "yellow" ;
        if (i == 0x0c) return "lblue"  ;
        if (i == 0x0d) return "lmag"  ;
        if (i == 0x0e) return "lcyan";
        if (i == 0x0f) return "white" ;
        return "???";
    }



    void colorTest()
    {
        for (int bg=0; bg<16; bg++)
        {
            for (int fg=0; fg<16; fg++)
            {
                if (fg == 0x08) Serial.println();

                int use_fg = fg > 7 ? 90+fg-8 : 30+fg;
                int use_bg = bg > 7 ? 100+bg-8 : 40+bg;

                Serial.printf("\033[%d;%dm %d,%d %6s:%-6s ",
                    use_fg,
                    use_bg,
                    use_fg,
                    use_bg,
                    color_name(fg),
                    color_name(bg));
            }
            Serial.println();
        }
    }
#endif



// virtual
void patchTest::begin(bool warm)
{
    expWindow::begin(warm);
    draw_needed = true;
    // colorTest();

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
void patchTest::updateUI()
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
            false,
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
