//-------------------------------
// myLeds.h
//-------------------------------

#pragma once

#define LED_BLACK  0
#define LED_RED    0xFF0000
#define LED_GREEN  0x00FF00
#define LED_BLUE   0x0000FF
#define LED_YELLOW 0xFFFF00
#define LED_PURPLE 0xFF1088
#define LED_ORANGE 0xE05800
#define LED_WHITE  0xFFFFFF
#define LED_CYAN   0x0088FF


#define LED_RGB(r,g,b)  (((r)<<16) | ((g)<<8) | (b))

extern void initLEDs();
extern void clearLEDs();

extern int getLEDBrightness();
extern void setLEDBrightness(int brightness);   // 0..100

extern void setLED(int num, unsigned color);
extern void setLED(int row, int col, unsigned color);
extern void showLEDs(bool force=false);

extern void LEDFancyStart();


