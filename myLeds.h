
#ifndef __myleds_h__
#define __myleds_h__

#if 1
    #define RED    0xFF0000
    #define GREEN  0x00FF00
    #define BLUE   0x0000FF
    #define YELLOW 0xFFFF00
    #define PURPLE 0xFF1088
    #define ORANGE 0xE05800
    #define WHITE  0xFFFFFF
#else   // Less intense...
    #define RED    0x160000
    #define GREEN  0x001600
    #define BLUE   0x000016
    #define YELLOW 0x101400
    #define PURPLE 0x120009
    #define ORANGE 0x100400
    #define WHITE  0x101010
#endif

extern void initLEDs();
extern void clearLEDs();
extern int getLEDBrightness();
extern void setLEDBrightness(int brightness);   // 0..100
extern void setLED(int row, int col, unsigned color);
extern void showLEDs(bool force=false);
extern void LEDFancyStart();


    
#endif  // !__myleds_h__

