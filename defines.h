#ifndef __defines_h__
#define __defines_h__

#define VERSION   "1.2"

#define NUM_PORTS           8       // ports defined in midiQueue.h
#define NUM_PEDALS          4
#define NUM_BUTTON_COLS     5
#define NUM_BUTTON_ROWS     5
#define NUM_MIDI_PORTS      8

#define THE_SYSTEM_BUTTON   4

#define NUM_BUTTONS        (NUM_BUTTON_COLS * NUM_BUTTON_ROWS)
#define BUTTON_NUM(r,c)    ((r) * NUM_BUTTON_COLS + (c))
#define BUTTON_ROW(i)      ((i) / NUM_BUTTON_COLS)
#define BUTTON_COL(i)      ((i) % NUM_BUTTON_COLS)

#define PEDAL_SYNTH     0
#define PEDAL_LOOP      1
#define PEDAL_WAH       2
#define PEDAL_GUITAR    3


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

#define ansi_color_black 	            30
#define ansi_color_red 	     	        31
#define ansi_color_green 	            32
#define ansi_color_brown 	 	        33
#define ansi_color_blue 	            34
#define ansi_color_magenta 	 	        35
#define ansi_color_cyan 	            36
#define ansi_color_light_grey 	        37

#define ansi_color_grey  	            90
#define ansi_color_light_red 	        91
#define ansi_color_light_green 	        92
#define ansi_color_yellow 		        93
#define ansi_color_light_blue  	        94
#define ansi_color_light_magenta        95
#define ansi_color_light_cyan 	        96
#define ansi_color_white  		        97

#define ansi_color_bg_black 	        40
#define ansi_color_bg_red 	     	    41
#define ansi_color_bg_green 	        42
#define ansi_color_bg_brown 	 	    43
#define ansi_color_bg_blue 	            44
#define ansi_color_bg_magenta 	 	    45
#define ansi_color_bg_cyan 	            46
#define ansi_color_bg_light_grey 	    47

#define ansi_color_bg_grey  	        100
#define ansi_color_bg_light_red 	    101
#define ansi_color_bg_light_green 	    102
#define ansi_color_bg_yellow 		    103
#define ansi_color_bg_light_blue  	    104
#define ansi_color_bg_light_magenta     105
#define ansi_color_bg_light_cyan 	    106
#define ansi_color_bg_white  		    107


#endif  // !__defines_h__
