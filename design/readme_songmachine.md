# The songMachine

This page describes the **songMachine** and the **Song Language**.

## songMachine

The songMachine *"runs"* programs written in the **Song Language**.  The song language
is case insenstive, and can have line oriented comments in them,
delimited by pound signs **(#)** as in perl or bash scripts.

Please see the **songs** folder for some examples of **.song** files.

The songMachine allows you to automate complicated series of button presses
and behaviors so that you can typically orchestrate a live looped
song with 3 or 4 button presses.

The songMachine gives you **four user programmable buttons**
(the bottom left four buttons on the teensyExpression), and
**two user programmable display areas** on the TFT display.

It abstracts the behavior of the teensyExpression and associated
Looper into a series of **statements** that can (and will) be exectuted
in a deterministic order.

It includes **flow control** constructs, like **GOTO** and **METHOD CALLS**,
and allows you to trigger behaviors based on **button presses** or **loop points**.

### - Flow of Control - blocking and button and loop labels

Execution of the program starts at the beginning of the file and
proceeds until a **loop:** label or **button1-4** label is encountered.

If the machine is pending a *loop* label, the next time the Looper
comes around to a **loop point** that section of code will begin
executing.

If you press one of the four **buttons**, the machine will look
forward until it finds a label for the given button, and will
execute that section of code.

A *button* may *jump over* a loop label, but a loop
event **cannot** jump over a button label.

**METHODS** are treated as separate sub-programs with their
own scope for buttons and loop labels.  A button press
outside of a method **will not** jump *INTO* the method.

*It is beyond the scope of this document, at this time, to
go into excruciating detail about the songMachine behavior.
Hopefully there will be some **videos** forthcoming that
will make it clearer.*

**---------------------------------------------------------------------**

## Song Language (.song files)

### - CONSTANTS

The song language supports the following **constants** as parameters to certain statements

**Colors** apply to either the text in the TFT shown by the DISPLAY sttement, or the
color of buttons in the BUTTON_COLOR statement.

- **RED**
- **GREEN**
- **BLUE**
- **YELLOW**
- **PURPLE**    - this is actually magenta in practice
- **ORANGE**
- **WHITE**
- **CYAN**
- **BLACK**

**Buttons** may optionally be told to *"flash** in the BUTTON_COLOR statement

- **FLASH**

**Looper Clips** may be *muted* or *unmuted* in the LOOPER_CLIP statement

- **MUTE**
- **UNMUTE**

**Guitar Effects** can be turned *ON* or *OFF*

- **ON**
- **OFF**

**Numbers:**

- **Numbers** are generally integers, and limited to 3 digits.
- **Volumes** are numbers in the range of 0 to 127
- **Delays** are measured in 10ths of a second from 0..255,
  allowing for delays of 0 to 25 seconds to be stored in a single byte.
- **button numbers**, **track numbers**, and **clip numbers** are limited to 1 through 4

**Identifiers** and **labels**

All of the words in the langauage are reserved.  The button
and loop labels are followe immeidately by *colons*.

Anything else is a potential user defined identifier.
Identifiers must start with A-Z, or a-z, and may include numbers,
dashes, underscores, and dots in them.

### - Statements


The song language supports the following **statements**L

**Display Statements**

- **BUTTON_COLOR** *button_num,color[,opt_flash]* - set the given button
  to the given color.  May optionally include (comma) **FLASH** to make
  the button flash.
- **DISPLAY** *area_num,"quoted string"[,opt_color]* - The DISPLAY statement
  shows text on the TFT in one of the two display areas (1 or 2).  You
  must provide a quoted string for the text to display, and the statement
  takes an optional **color constant**.   The default color for area
  number 1 is green, and the default color for area 2 is white.

**Volume Statments**

- **LOOP_VOLUME** *value[,opt_fade_time]*
- **SYNTH_VOLUME** *value[,opt_fade_time]*
- **GUITAR_VOLUME** *value[,opt_fade_time]*

  The LOOP, SYNTH, and GUITAR
  VOLUME statements allow you to set the volumes (pedals) to the given
  values. These statement accept an optional **fade time** parameter,
  measured in 10ths of a secondm over which the given volume change will
  take place. A fade_time of zero (0) means that that the volume change will take
  place immediately.  The default fade time, if none is specified is
  2 (0.2 seconds) to avoid pops and clicks from abpruptly changing
  levels.

**Synth Patch Statement**

- **SYNTH_PATCH** *identifier* - the identifier must be the **short name** of one of the
  currently available patches (on any bank).  These names are currently hard-coded in
  rigNew.cpp

**Guitar Effect Statements**

- **GUITAR_EFFECT_NONE** - turns off all guitar effects
- **GUITAR_EFFECT_DISTORT** *on/off*
- **GUITAR_EFFECT_WAH** *on/off*
- **GUITAR_EFFECT_CHORUS** *on/off*
- **GUITAR_EFFECT_ECHO** *on/off*

  The guitar effects may be turned *ON* or *OFF*

**Looper Statements**

- **CLEAR_LOOPER** - clears the Looper returning it to an empty state ready to record a new song.
- **LOOPER_STOP** - stop the Looper normally at the next *loop point*
- **LOOPER_STOP_IMMEDIATE** - stop the Looper immediately
- **LOOPER_TRACK** *track_number* - is the functional equivalant of pressing one of the
  four **Track1-4** buttons.  The behavior of the Looper in response to a Track Button press
  depends on it's running state and the state of the *DUB* button.
  On an empty looper, the first press begins *recording* a song, and the
  second press starts *playing it*.
- **LOOPER_CLIP** *clip_num,MUTE/UNMUTE* - mute, or unmute, the given clip (layer)
  within the **currently selected** track.
- **DUB_MODE** - the DUB mode (in the rPi looper) is a one-shot mode that applies
  to the *next* Track button that is pressed.  In the cannonical example, from an
  empty looper, pressing Track1 once will start recording the first clip on track1, and pressing
  it again will begin playing that clip.   But what if you wanted to immediately record the
  second clip?  You would **first** press the *DUB_MODE* button (execute the DUB_MODE statement)
  and **THEN** upon the second Track1 button press, the Looper will not only start playing the
  1st clip, but will start recording a second one.  The *DUB_MODE* button works between tracks
  too, so that you can, for example, switch to a different track and both start playing it **AND**
  recording another clip in it, on the next track (subject to loop point considerations)
- **LOOP_IMMEDIATE** - loop all clips in the current track, immediately, back to their starts
  (or the last **MARK POINT** if one has been set)
- **LOOPER_SET_START_MARK** - notify the looper to set a **MARK_POINT** which it will
  subsequently loop back to, within the currently playing Track, if any.  This allows
  you to do things like *"repeat the end of that phrase three times"* from **within**
  a given track.  Track MARK_POINTS are cleared when the current or selected track
  is changed.

**Control Flow Statements**

- **DELAY** *tenths_of_a_second* - will introduce a delay in the songMachine.
  This is useful, for instance if you want to fade the guitar down, and **THEN**
  fade the synthesizer up.   Otherwise, the fades would happen simultaneously.
  *DELAY* can also be useful after recording a track before changing volume levels
  to allow for the *inter-clip crossfades* in the (rPi based loopoer) which, by
  default, have a crossfade time of about 30ms.
- **GOTO** *identifier* - will jump to a user defined label (an identifier by
  itself followed immediately by a colon).
- **CALL** *identifier* - will call a defined method
- **METHOD** *identifer* - defines a section of code as a *METHOD* with it's own
   button and loop scope.  Methods may NOT be nested.
- **END_METHOD** required at the end of any methods.

Note that an identifer by itself, followed by a colon is a statement (a label) for use
with the **GOTO** statement, and that the special labels **loop:*, **button1:**, **button3:**,
**button3:**, and **button4:** are reserved and have specific behaviors associated with them.

