//------------------------------------
// myTFT.h API to myLcd library
//------------------------------------

#pragma once

#include <myLcdDevice.h>    // my modified library
#include <font_Arial.h>
#include <font_ArialBold.h>
#include "defines.h"

#define TFT_WIDTH           480
#define TFT_HEIGHT          320

// these color constants are the same whether using
// myLCD, ILI9486_t3, or ILI9341_t3

#define TFT_RGB_COLOR(r,g,b)  ((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3))

#define TFT_BLACK           0x0000
#define TFT_BLUE            0x001F
#define TFT_RED             0xF800
#define TFT_GREEN           0x07E0
#define TFT_CYAN            0x07FF
#define TFT_MAGENTA         0xF81F
#define TFT_YELLOW          0xFFE0
#define TFT_WHITE           0xFFFF

#define TFT_NAVY            0x000F
#define TFT_DARKGREEN       0x03E0
#define TFT_DARKCYAN        0x03EF
#define TFT_MAROON          0x7800
#define TFT_PURPLE          0x780F
#define TFT_OLIVE           0x7BE0
#define TFT_LIGHTGREY       0xC618
#define TFT_DARKGREY        0x7BEF
#define TFT_ORANGE          0xFD20
#define TFT_GREENYELLOW     0xAFE5
#define TFT_PINK            0xF81F


extern myLcdDevice mylcd;

extern void initMyTFT();

inline void fillRect(int_rect &rect, int color)
{
    mylcd.fillRect(
        rect.xs,
        rect.ys,
        rect.width(),
        rect.height(),
        color);
}