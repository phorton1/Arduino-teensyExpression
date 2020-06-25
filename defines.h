#ifndef __defines_h__
#define __defines_h__

#define VERSION   "1.2"

#define NUM_PEDALS          4
#define NUM_BUTTON_COLS     5
#define NUM_BUTTON_ROWS     5
#define NUM_MIDI_PORTS      8

#define THE_SYSTEM_BUTTON   4

#define NUM_BUTTONS        (NUM_BUTTON_COLS * NUM_BUTTON_ROWS)
#define BUTTON_NUM(r,c)    ((r) * NUM_BUTTON_COLS + (c))
#define BUTTON_ROW(i)      ((i) / NUM_BUTTON_COLS)
#define BUTTON_COL(i)      ((i) % NUM_BUTTON_COLS)


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

#define PEDAL_CURVE_TYPE_LINEAR         0       // default and only currently implemented one
#define PEDAL_CURVE_TYPE_ASYMPTOTIC     1
#define PEDAL_CURVE_TYPE_SCURVE         2

#define MAX_PEDAL_CURVE_POINTS          2       // number of movable points (with weights)


//--------------------------------------------------------------------------------
// SETTINGS (PREFERENCES) OPTIONS
//--------------------------------------------------------------------------------
// These define are also the locations in EEPROM of these items

#define PREF_BRIGHTNESS         0           // 1..100 - default(40)
#define PREF_PATCH_NUM          1           // 0..254 - default(1)
#define PREF_DEBUG_PORT         2           // off, USB, Serial - default(USB)
#define PREF_SPOOF_FTP          3           // off, on - default(off)
#define PREF_FTP_PORT           4           // off, Host, Remote, default(Host)

//-----------------------------
// default patch settings
//-----------------------------
// these are the default settings for patches that don't use patchSettings
// every patch change must at least calls setPatchSettings(0)
// POLY_MODE and SPLITS are specific to patches.  Since patch
// settings also include POLY_MODE and SPLITS, we define the offsets here.

#define OFFSET_PATCH_USE_DEFAULT 0          // 0,1 - default = 1 (on patchSettings)
#define OFFSET_FTP_POLY_MODE     1          // 0=mono, 1=poly, default=mono (on patchSettings)

#define OFFSET_FTP_TOUCH_SENS    2          // 0..9 - default(4)
#define OFFSET_FTP_DYN_RANGE     3          // 0..10 - default(10), we map to 0x0A..0x14 (10..20) in usage
#define OFFSET_FTP_DYN_OFFSET    4          // 0..20 - default(10)
#define OFFSET_PERF_FILTER       5          // off, on - default(off), filters all but notes and bends from channel 0
#define OFFSET_PERF_FILTER_BENDS 6          // off, on - default(off), filters bends too.
#define OFFSET_PERF_SPLITS       7          // off, 1+5, 2+3, 3+3, 4+2, 5+1  (number of strings in first split if any)

#define PREF_FTP_PATCH_SETTING   3          // fake skip offset 0 and 1
#define PREF_FTP_TOUCH_SENS      (PREF_FTP_PATCH_SETTING + OFFSET_FTP_TOUCH_SENS)
#define PREF_FTP_DYN_RANGE       (PREF_FTP_PATCH_SETTING + OFFSET_FTP_DYN_RANGE)
#define PREF_FTP_DYN_OFFSET      (PREF_FTP_PATCH_SETTING + OFFSET_FTP_DYN_OFFSET)
#define PREF_PERF_FILTER         (PREF_FTP_PATCH_SETTING + OFFSET_PERF_FILTER)
#define PREF_PERF_FILTER_BENDS   (PREF_FTP_PATCH_SETTING + OFFSET_PERF_FILTER_BENDS)


//-----------------------------
// midi monitor settings
//-----------------------------
// there are a bunch of default setting for the monitor
//     that each of the  ports can override and set individually
// different midi ports can be directed to different serial ports as well
// this section is initially implemented with whatever capabilities I had
//     when I changed this.  These should be reworked to a sensible set
//     for general use ...


#define BYTES_PER_PORT_MONITOR     10       // includes one unused byte

#define PREF_DEFAULT_MIDI_MONITOR  (PREF_FTP_PATCH_SETTING + OFFSET_PERF_SPLITS)        // 10

#define PREF_MONITOR_PORT0         (PREF_DEFAULT_MIDI_MONITOR + BYTES_PER_PORT_MONITOR) // 20

#define PREF_MONITOR_DEVICE_IN0    PREF_MONITOR_PORT0                                   // 20
#define PREF_MONITOR_DEVICE_IN1    (PREF_MONITOR_PORT0 + 1*BYTES_PER_PORT_MONITOR)      // 30
#define PREF_MONITOR_DEVICE_OUT0   (PREF_MONITOR_PORT0 + 2*BYTES_PER_PORT_MONITOR)      // 40
#define PREF_MONITOR_DEVICE_OUT1   (PREF_MONITOR_PORT0 + 3*BYTES_PER_PORT_MONITOR)      // 50
#define PREF_MONITOR_HOST_IN0      (PREF_MONITOR_PORT0 + 4*BYTES_PER_PORT_MONITOR)      // 60
#define PREF_MONITOR_HOST_IN1      (PREF_MONITOR_PORT0 + 5*BYTES_PER_PORT_MONITOR)      // 70
#define PREF_MONITOR_HOST_OUT0     (PREF_MONITOR_PORT0 + 6*BYTES_PER_PORT_MONITOR)      // 80
#define PREF_MONITOR_HOST_OUT1     (PREF_MONITOR_PORT0 + 7*BYTES_PER_PORT_MONITOR)      // 90

#define OFFSET_MIDI_MONITOR               0     // off, USB, Serial, 255=default    default(USB)
#define OFFSET_MONITOR_SHOW_FILTERED      1     // off, on                 default(off)
    // only applies to messages from the host port
#define OFFSET_MONITOR_SYSEX              2     // off, on, Detail         default(2=Detail)
#define OFFSET_MONITOR_ACTIVESENSE        3     // off, on                 default(0==off)
#define OFFSET_MONITOR_PERFORMANCE_CCS    4     // off, on,                default(1=on)
// ftp input and output port only
#define OFFSET_MONITOR_FTP_TUNING_MSGS    5     // off, on                 default(1==on)
#define OFFSET_MONITOR_FTP_NOTE_INFO      6     // off, on                 default(1==on)
#define OFFSET_MONITOR_FTP_VOLUME         7     // off, on                 default(1==on)
#define OFFSET_MONITOR_FTP_BATTERY        8     // off, on                 default(1==on)


#define DEFAULT_PREF_MONITOR            (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MIDI_MONITOR)
#define DEFAULT_PREF_SHOW_FILTERED      (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_SHOW_FILTERED)
#define DEFAULT_PREF_SYSEX              (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_SYSEX)
#define DEFAULT_PREF_ACTIVESENSE        (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_ACTIVESENSE)
#define DEFAULT_PREF_PERFORMANCE_CCS    (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_PERFORMANCE_CCS)
#define DEFAULT_PREF_FTP_TUNING_MSGS    (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_FTP_TUNING_MSGS)
#define DEFAULT_PREF_FTP_NOTE_INFO      (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_FTP_NOTE_INFO)
#define DEFAULT_PREF_FTP_VOLUME         (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_FTP_VOLUME)
#define DEFAULT_PREF_FTP_BATTERY        (PREF_DEFAULT_MIDI_MONITOR + OFFSET_MONITOR_FTP_BATTERY)


#define PORT_PREF_MONITOR(p)            (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MIDI_MONITOR)
#define PORT_PREF_SHOW_FILTERED(p)      (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_SHOW_FILTERED)
#define PORT_PREF_SYSEX(p)              (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_SYSEX)
#define PORT_PREF_ACTIVESENSE(p)        (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_ACTIVESENSE)
#define PORT_PREF_PERFORMANCE_CCS(p)    (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_PERFORMANCE_CCS)
#define PORT_PREF_FTP_TUNING_MSGS(p)    (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_FTP_TUNING_MSGS)
#define PORT_PREF_FTP_NOTE_INFO(p)      (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + DEFAULT_PREF_FTP_NOTE_INFO)
#define PORT_PREF_FTP_VOLUME(p)         (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_FTP_VOLUME)
#define PORT_PREF_FTP_BATTERY(p)        (PREF_MONITOR_PORT0 + (p)*BYTES_PER_PORT_MONITOR + OFFSET_MONITOR_FTP_BATTERY)


//--------------------------------
// pedals
//--------------------------------

#define PREF_PEDAL_CALIB_MIN_OFFSET     0       // default 0
#define PREF_PEDAL_CALIB_MAX_OFFSET     2       // default 1023
#define PREF_PEDAL_VALUE_MIN_OFFSET     4       // default 0
#define PREF_PEDAL_VALUE_MAX_OFFSET     5       // default 127 (except for pedal 1, loop, which is 92)
#define PREF_PEDAL_CURVE_TYPE_OFFSET    6       // 0=linear, 1=asymptotic, 2=scurve - default(0) == num_points

#define PREF_PEDAL_POINTS_OFFSET        8       // = 8, and there are 2 of what follows

#define PEDAL_POINTS_OFFSET_X           0
#define PEDAL_POINTS_OFFSET_Y           2
#define PEDAL_POINTS_OFFSET_WEIGHT      4
#define PEDAL_POINT_PREF_SIZE           6

#define PREF_BYTES_PER_PEDAL   (PREF_PEDAL_POINTS_OFFSET + MAX_PEDAL_CURVE_POINTS * PEDAL_POINT_PREF_SIZE)   // 20

#define PREF_PEDAL0            (PREF_MONITOR_PORT0 + 8*BYTES_PER_PORT_MONITOR)      // 100
#define PREF_PEDAL(i)          (PREF_PEDAL0 + (i)*PREF_BYTES_PER_PEDAL)             // 100, 120, 140, 160

#define NUM_EEPROM_USED        (PREF_PEDAL0 + NUM_PEDALS*PREF_BYTES_PER_PEDAL)     // 180


#endif  // !__defines_h__
