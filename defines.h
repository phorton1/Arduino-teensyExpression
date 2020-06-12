#ifndef __defines_h__
#define __defines_h__

#define VERSION   "1.2"


#define NAME_MIDI_DEVICE_AS_FISHMAN 0
    // There is a scheme to name the MIDI device as the FTP Triple Play
    // including providing the correct Device and Product IDs, and naming
    // the end points to TP guitar, but I was still unable to spoof it.
    // Closest I have come is to use loopMidi ... but it does not do
    // senstivity ("audio not found").  So the names are not, per se
    // as important.  I cannot remember how I got sensitivity (B7 1F xy
    // where x is string and y is senstivity) working.
    
    
// these defines are generally "ON" but are allowed to be
// turned off piecemeal for debugging ...

#define WITH_SYSTEM           1
#define WITH_CHEAP_TFT        1         // Cheap Ardino 3.5" 320x480 TFT's
#define WITH_TOUCH            1         // Cheap Arduino Resistive touch screen
#define WITH_ROTARY           1
#define WITH_PEDALS           1

// these defines are iffy
// there are program options to turn them on or off at runtime as well
// and here we define the defaults for those

#define WITH_MIDI_HOST        1
#define WITH_SERIAL_PORT   1

#if WITH_MIDI_HOST
    #define DEFAULT_MIDI_HOST  0
    extern int midi_host_on;
#endif

#if WITH_SERIAL_PORT
    #define DEFAULT_SERIAL_PORT 0
    extern int serial_port_on;
#endif

#define DEFAULT_CONFIG_NUM  1
#define DEFAULT_BRIGHTNESS  60

    
//----------------------------------------------------------------------
// PIN USAGE
//----------------------------------------------------------------------
// left/right  main               alternate             
//   pin
//    GND
// L  0        unused1 RX1 used by LEDS             
// L  1        unused2 TX1 used by LEDS             
// L  2        ROTARY_2A                        
// L  3        ROTARY_2B
// L  4        ROTARY_1A                            
// L  5        LEDS_OUT (Serial1)                   
// L  6        ROTARY_1B                            
// L  7        SERIAL_RX3    We are using Serial3 as our "second" SERIAL_IO_PORT      
// L  8        SERIAL_TX3         
// L  9        ROTARY_3B
// L 10        ROTARY_3A
// L 11        ROTARY_4A
// L 12        ROTARY_4B
//   3.3V
// L 24        BUTTON_OUT0           
// L 25        BUTTON_OUT1           
// L 26        BUTTON_OUT2           
// L 27        BUTTON_OUT3           
// L 28        BUTTON_OUT4           
// L 29        BUTTON_IN0            
// L 30        BUTTON_IN1            
// L 31 A12    BUTTON_IN2            
// L 32 A13    BUTTON_IN3            
//      5V
//      AGND
//      GND
// R 23 A9     EXPR1                                           
// R 22 A8     EXPR2                 
// R 21 A7     EXPR3                 
// R 20 A6     EXPR4                 
// R 19 A5     CHEAP_TFT_DATA0                         
// R 18 A4     CHEAP_TFT_RESET
// R 17 A3     CHEAP_TFT_CS            
// R 16 A2     CHEAP_TFT_CD(RS)          
// R 15 A1     CHEAP_TFT_WR              
// R 14 A0     CHEAP_TFT_RD                           
// R 13        CHEAP_TFT_DATA1               
//      3,3V
// R    A22    x - unused analog only                             
// R    A21    x - unused analog only                             
// R 39 A20    CHEAP_TFT_DATA7                              
// R 38 A19    CHEAP_TFT_DATA6                                                       
// R 37 A18    CHEAP_TFT_DATA5                                                                                   
// R 36 A17    CHEAP_TFT_DATA4                                                          
// R 35 A16    CHEAP_TFT_DATA3                                                          
// R 34 A15    CHEAP_TFT_DATA2                                                          
// R 33 A14    BUTTON_IN4                               

// The pins for the TFT were not well laid on in the circuit,
// causing a twisted cable, and some late fixes (13 and 19 to
// CHEAP_DATA1 and 0, right on top of unused A21 and A22 pins
// which cannot be used for digital io.
//
// The pins for the ROTARY were munged ... one of em is
// different, and the order is weird
//
// I had to add 10K resistors to the 1/4" pedal jacks to
// the insertion switch to ground them when not in use!

#if WITH_ROTARY
    #define ROTARY_1A   4     // mashed up pin assignments
    #define ROTARY_1B   6     
    #define ROTARY_2A   2                          
    #define ROTARY_2B   3                          
    #define ROTARY_3A   10    // this one is wired differently than the others
    #define ROTARY_3B   9     
    #define ROTARY_4A   11    
    #define ROTARY_4B   12    
#endif


#if WITH_PEDALS
    #define PIN_EXPR1    23  // A6
    #define PIN_EXPR2    22  // A7
    #define PIN_EXPR3    21  // A8
    #define PIN_EXPR4    20  // A9
#endif

//-----------------------------------------
// CHEAP TFT and TOUCH SCREEN
//-----------------------------------------
// Cheap Ardino 3.5" 320x480 TFT's
// Uses my modified version of LCDWIKI, which
//
// - has HARDWIRED (though arbitray) pin numbers
//   in myLCDWIKI_KBV/LCDWikiStuff_for_teensy.cpp
// - and uses ILI9431_t3.h in myLCDWIKI_GUI/LCDWIKI_GUI.cpp
//   to implement fonts
//
// Do not confuse the CD flash drive pins for the touch screen.
// This device uses direct measurements of resistance and very
// touchy, pun intended, modified version of TouchScreen.h which:
//
// - ONLY USE getPoint()  !!!
// - uses digitalWrite instead of portReg stuff
// - softened up the validity checks
// - reset all the pinModes when finished

#if WITH_CHEAP_TFT || WITH_TOUCH
    #define CHEAP_TFT_DATA0     19      // needed by ts
    #define CHEAP_TFT_DATA1     13      // needed by ts
    #define CHEAP_TFT_DATA2     34
    #define CHEAP_TFT_DATA3     35
    #define CHEAP_TFT_DATA4     36
    #define CHEAP_TFT_DATA5     37
    #define CHEAP_TFT_DATA6     38
    #define CHEAP_TFT_DATA7     39
    
    #define CHEAP_TFT_RD         14
    #define CHEAP_TFT_WR         15
    #define CHEAP_TFT_CD_RS      16      // needed by ts - labelled "RS" on board
    #define CHEAP_TFT_CS         17      // needed by ts
    #define CHEAP_TFT_RESET      18
#endif


#if WITH_CHEAP_TFT
    #define TFT_BLACK   0x0000
    #define TFT_BLUE    0x001F
    #define TFT_RED     0xF800
    #define TFT_GREEN   0x07E0
    #define TFT_CYAN    0x07FF
    #define TFT_MAGENTA 0xF81F
    #define TFT_YELLOW  0xFFE0
    #define TFT_WHITE   0xFFFF
    
    // #define TFT_BLACK       0x0000      /*   0,   0,   0 */
    // #define TFT_NAVY        0x000F      /*   0,   0, 128 */
    // #define TFT_DARKGREEN   0x03E0      /*   0, 128,   0 */
    // #define TFT_DARKCYAN    0x03EF      /*   0, 128, 128 */
    // #define TFT_MAROON      0x7800      /* 128,   0,   0 */
    // #define TFT_PURPLE      0x780F      /* 128,   0, 128 */
    // #define TFT_OLIVE       0x7BE0      /* 128, 128,   0 */
    // #define TFT_LIGHTGREY   0xC618      /* 192, 192, 192 */
    // #define TFT_DARKGREY    0x7BEF      /* 128, 128, 128 */
    // #define TFT_BLUE        0x001F      /*   0,   0, 255 */
    // #define TFT_GREEN       0x07E0      /*   0, 255,   0 */
    // #define TFT_CYAN        0x07FF      /*   0, 255, 255 */
    // #define TFT_RED         0xF800      /* 255,   0,   0 */
    // #define TFT_MAGENTA     0xF81F      /* 255,   0, 255 */
    // #define TFT_YELLOW      0xFFE0      /* 255, 255,   0 */
    // #define TFT_WHITE       0xFFFF      /* 255, 255, 255 */
    // #define TFT_ORANGE      0xFD20      /* 255, 165,   0 */
    // #define TFT_GREENYELLOW 0xAFE5      /* 173, 255,  47 */
    // #define TFT_PINK        0xF81F    
#endif


#if WITH_TOUCH
    // define   my definition    // pin number   // arduino pin number and notes    // general notes
    #define YP  CHEAP_TFT_CD_RS  // 17=A4        // A2 maps to LCD_RS on arduino    // must be an analog pin, use "An" notation!
    #define XM  CHEAP_TFT_CS     // 16=A2        // A3 maps to LCD_CS on arduino    // must be an analog pin, use "An" notation!
    #define YM  CHEAP_TFT_DATA0  // 14=A0        // 8  maps to LCD_D0 on arduino    // can be a digital pin
    #define XP  CHEAP_TFT_DATA1  // 13           // 9  maps to LCD_D1 on arduino    // can be a digital pin
#endif





//-----------------------------------
// SETTINGS (PREFERENCES) OPTIONS
//-----------------------------------
//
// These are modifiable by the systemConfig configuration.
// They are used by the rest of the program, including configuration specific settings.


#define EEPROM_BRIGHTNESS       0
#define EEPROM_CONFIG_NUM       1
#define EEPROM_MIDI_HOST        2
#define EEPROM_SERIAL_PORT      3


#endif  // !__defines_h__
