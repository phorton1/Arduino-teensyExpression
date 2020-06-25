#ifndef __defines_h__
#define __defines_h__

#define VERSION   "1.2"


#define NUM_BUTTON_COLS     5
#define NUM_BUTTON_ROWS     5

#define THE_SYSTEM_BUTTON   4

#define DEFAULT_CONFIG_NUM  1
#define DEFAULT_BRIGHTNESS  30



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


#define PIN_BUTTON_OUT0         24
#define PIN_BUTTON_OUT1         25
#define PIN_BUTTON_OUT2         26
#define PIN_BUTTON_OUT3         27
#define PIN_BUTTON_OUT4         28
#define PIN_BUTTON_IN0          29
#define PIN_BUTTON_IN1          30
#define PIN_BUTTON_IN2          31
#define PIN_BUTTON_IN3          32
#define PIN_BUTTON_IN4          33


#define ROTARY_1A   4     // mashed up pin assignments
#define ROTARY_1B   6
#define ROTARY_2A   2
#define ROTARY_2B   3
#define ROTARY_3A   10    // this one is wired differently than the others
#define ROTARY_3B   9
#define ROTARY_4A   11
#define ROTARY_4B   12


#define PIN_EXPR1    23  // A6
#define PIN_EXPR2    22  // A7
#define PIN_EXPR3    21  // A8
#define PIN_EXPR4    20  // A9


//-----------------------------------------
// CHEAP TFT and TOUCH SCREEN
//-----------------------------------------
// Always defined as of now
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


// define   my definition    // pin number   // arduino pin number and notes    // general notes
#define YP  CHEAP_TFT_CD_RS  // 17=A4        // A2 maps to LCD_RS on arduino    // must be an analog pin, use "An" notation!
#define XM  CHEAP_TFT_CS     // 16=A2        // A3 maps to LCD_CS on arduino    // must be an analog pin, use "An" notation!
#define YM  CHEAP_TFT_DATA0  // 14=A0        // 8  maps to LCD_D0 on arduino    // can be a digital pin
#define XP  CHEAP_TFT_DATA1  // 13           // 9  maps to LCD_D1 on arduino    // can be a digital pin


// common buttons

#define BUTTON_MOVE_UP          12
#define BUTTON_MOVE_LEFT        16
#define BUTTON_MOVE_RIGHT       18
#define BUTTON_MOVE_DOWN        22
#define BUTTON_SELECT           17


// ansi colors

#define ansi_color_black 	        30
#define ansi_color_red 	     	    31
#define ansi_color_green 	        32
#define ansi_color_brown 	 	    33
#define ansi_color_blue 	        34
#define ansi_color_magenta 	 	    35
#define ansi_color_cyan 	        36
#define ansi_color_grey 	        37
#define ansi_color_light_gray  	    90
#define ansi_color_light_red 	    91
#define ansi_color_light_green 	    92
#define ansi_color_yellow 		    93
#define ansi_color_light_blue  	    94
#define ansi_color_light_magenta    95
#define ansi_color_light_cyan 	    96
#define ansi_color_white  		    97


//----------------------------------
// Pedals
//----------------------------------
// for the time being there is one set of values for the pedals
// across all patches.  An idea is then to have "pedal sets"
// that can be shared between different patches while still
// allowing for multiple definitions.

#define NUM_PEDALS  4

#define PEDAL_CURVE_TYPE_LINEAR         0       // default and only currently implemented one
#define PEDAL_CURVE_TYPE_ASYMPTOTIC     1
#define PEDAL_CURVE_TYPE_SCURVE         2

#define MAX_PEDAL_CURVE_POINTS          2       // number of movable points (with weights)


//--------------------------------------------------------------------------------
// SETTINGS (PREFERENCES) OPTIONS
//--------------------------------------------------------------------------------
// These define are also the locations in EEPROM of these items

// #define EEPROM_BRIGHTNESS       0
// #define EEPROM_PATCH_NUM        1
// #define EEPROM_SPOOF_FTP        2

#define PREF_BRIGHTNESS         0           // 1..100 - default(40)
#define PREF_PATCH_NUM          1           // 0..254 - default(1)
#define PREF_DEBUG_PORT         2           // Off, USB, Serial - default(USB)
#define PREF_SPOOF_FTP         10           // Off, On default(off)
#define PREF_FTP_PORT          11           // None, Host, Remote, default(Host)

//-----------------------------
// default patch settings
//-----------------------------
// these are the default settings for patches that don't use patchSettings
// every patch change must at least calls setPatchSettings(0)

#define PREF_FTP_POLY_MODE     12           // 1=Poly, 0=Mono - default(255=take whatever is found on device)
#define PREF_FTP_TOUCH_SENS    13           // 0..9 - default(4)
#define PREF_FTP_DYN_RANGE     14           // 0x0A..0x14 (10..20) weird - default(20), we map to 0..10, default(10)
#define PREF_FTP_DYN_OFFSET    15           // 0..20 - default(10)

#define PREF_PERF_FILTER       30           // off, on - default(off), filters all but notes and bends from channel 0
#define PREF_PERF_FILTER_BENDS 31           // off, on - default(off), filters bends too.
#define PREF_PERF_SPLIT        60           // off, 1+5, 2+4, 3+3, 4+2, 5+1 - experimental - default(off)


//-----------------------------
// midi monitor settings
//-----------------------------

#define PREF_MONITOR_MIDI MONITOR   100     // OFF, USB, Serial        default(USB)
#define PREF_MONITOR_SHOW_FILTERED  101     // OFF, ON                 default(ON)
    // nice if filtered messges showed up as either dim of the same,
    // or grey, or something

#define PREF_MONITOR_SYSEX              110     // OFF, ON, Detail         default(1==ON)
#define PREF_MONITOR_ACTIVESENSE        111     // OFF, ON                 default(0==OFF)
#define PREF_MONITOR_PERFORMANCE CCS    112    //  OFF, ON,                default(1=ON)
    // whatever happened to be there when I implmented this
// ftp only
#define PREF_MONITOR_TUNING MSGS         120    // OFF, ON                 default(1==ON)
#define PREF_MONITOR_NOTE INFO           121    // OFF, ON                 default(1==ON)
#define PREF_MONITOR_VOLUME LEVEL        122    // OFF, ON                 default(1==ON)
#define PREF_MONITOR_BATTERY LEVEL       123    // OFF, ON                 default(1==ON)



#define PREF_PEDAL_CALIB_MIN_OFFSET     0
#define PREF_PEDAL_CALIB_MAX_OFFSET     2
#define PREF_PEDAL_VALUE_MIN_OFFSET     4
#define PREF_PEDAL_VALUE_MAX_OFFSET     5
#define PREF_PEDAL_CURVE_TYPE_OFFSET    6
#define PREF_PEDAL_POINTS_OFFSET        8       // = 8, and there are 2 of what follows

#define PEDAL_POINTS_OFFSET_X           0
#define PEDAL_POINTS_OFFSET_Y           2
#define PEDAL_POINTS_OFFSET_WEIGHT      4
#define PEDAL_POINT_PREF_SIZE           6

#define    PREF_BYTES_PER_PEDAL   (PREF_PEDAL_POINTS_OFFSET + MAX_PEDAL_CURVE_POINTS * PEDAL_POINT_PREF_SIZE)

#define PREF_PEDAL0         200
#define PREF_PEDAL1         (PREF_PEDAL0 + PREF_BYTES_PER_PEDAL)
#define PREF_PEDAL2         (PREF_PEDAL0 + 2*PREF_BYTES_PER_PEDAL)
#define PREF_PEDAL3         (PREF_PEDAL0 + 3*PREF_BYTES_PER_PEDAL)

#define NUM_EEPROM_USED         (PREF_PEDAL0 + 4*PREF_BYTES_PER_PEDAL)


#endif  // !__defines_h__
