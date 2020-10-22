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

## EXAMPLE CODE


Here is a short snippet of **"songmachine code"** that
may serve as an example.  Hopefully the comments are descriptive.

``` ruby
# INTRO and CHORUS 0

    # the program is executed from the beginning of the file
    # as soon as the song is "loaded"

    start:
        CLEAR_LOOPER                    # clear the looper to start
        LOOP_VOLUME 63                  # set looper to a known volume
        SYNTH_VOLUME 0                  # turn the synth all the way down
        GUITAR_VOLUME 70                # set the guitar to a known volume
        SYNTH_PATCH Bass1               # pick a synth patch for later use
        GUITAR_EFFECT_NONE              # turn off all guitar effects
        BUTTON_COLOR 4,yellow           # make 4th button from bottom left yellow
        BUTTON_COLOR 1,orange,FLASH     # make the first button flashing orange
        DISPLAY 1,"INTRO"               # display the part of the song we are in
        DISPLAY 2,"sing intro"          # display instructions about what to do here

        # the machine "blocks" on button1-4 or loop labels.
        # it advances to the next "buttonN" if that button is
        # pressed, or "loop:" label if the looper cycles through
        # a loop ...

        # In this example you play the intro, and then when ready, press the
        # 1st button to start recording the chorus ....

    button1:                            # when you press button1 the following happens:
        LOOPER_TRACK 1                  # start the looper recording
        BUTTON_COLOR 1,red              # set the button to red to indicate recording
        DISPLAY 1,"CHOR0"               # display the part of the song we are in
        DISPLAY 2,"recording base clip" # display what's going on at this time

# LEAD 0 - Acoustic guitar fill thru base clip

        # once you press the button again, it starts playing the loop,
        # turns on the guitar echo so you can play lead over it ...

    button1:
        LOOPER_TRACK 1                  # end recording
        delay 10                        # 10's of a second to allow for good loop end recording
        GUITAR_EFFECT_ECHO on           # turn on the echo effect
        BUTTON_COLOR 1,green,FLASH      # change button to green flashing
        DISPLAY 1,"A-LEAD"              # display stuff ...
        DISPLAY 2,"play acc lead\nready bass"

        # ready bass - you press this button at the end of the
        # guitar lead but BEFORE the loop happens to fade
        # out the guitar and fade in the bass before recording it.

    button1:                            # press this BEFORE loop ends!
        BUTTON_COLOR 1,red,FLASH        # show red flashing button
        GUITAR_VOLUME 0,20              # bring guitar down over 2 seconds
        SYNTH_VOLUME 85,20              # while bringing synth up over two seconds
        DUB_MODE                        # set dub mode to ...
        LOOPER_TRACK 1                  # start recording on next loop
        DISPLAY 2,"ready record bass"

# VERSE1 and CHORUS1 - while recording bass

        # the next time the looper comes around, the song
        # machine will get the "loop" event and trigger
        # the following section of code ...

    loop:
        BUTTON_COLOR 1,red              # button to red
        DISPLAY 1,"VERSE1"              # what part of song are we in?
        DISPLAY 2,"recording bass1"     # what the heck are you supposed to be doing now!

    ... and so on

```

The above example lets you play an intro, then record a straight guitar part,
then play lead over it, then start recording bass over that, with only two button
presses.  The chosen synth patch, volumes and effects are determined by the code.
All you do is tell the machine **WHEN** to change state ...
