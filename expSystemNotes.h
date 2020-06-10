
// The upper right magenta <exit/save> button retains its functionality
// The upper right optional <cancel> button retains it's functionality
// The entire configuration can be saved or aborted (there is no cancel)

//
// KEYPAD
//
//                  up
//             
//           left  select  right
//
//                  down
//


// <Brightness>           70
//       <up> <down> and <select> enabled
//       
//       buttons: NUMERICAL SPINNER <up> <down>
//       touch:   SLIDER BAR with OK BUTTON
//       rotary:  value
//
// <Config>          Old Rig  (short name)   
//       <up> <down> and <select> enabled
//
//       buttons:  STRING SELECTION spinner
//       touch:    STRING SELECTION LIST with OK button
//       rotary:   PER-INDENT
//
// Calibrate Touch
//       <select> enabled
//
//       buttons:  <select> cancels the process
//       MODAL whole screen through operation ...
//       rotary:  nothing
//       









// The entire program consists of the "expression system".
//
// TIME SLICING AND INTERRUPTS
//
// (1) The expSystem implements a timer interrupt loop, 100-200 times per
// second that performs the time critical bulk of the functionality, polling
// the button array and expression pedals, responding to changes to them,
// updating the LEDs and sending out midi messages.
//
// (2) It also has a task() method that is called from loop() to perform
// less time critical UI tasks, like updating the TFT screen.  The TFT
// touch processing is separately driven by interrupts at a lower priority
// than the main timer loop.
//
// CONFIGURATIONS
//
// The program consists of a number of "configurations" which define
// distinct sets of behaviors for the entire controller. For example,
// there may be one configuration that is dedicated to running the
// "old" iPad rig, that more or less maps to the previous Softstep/MPD218
// behaviors, and a second, completely different configuration, that
// maps to the rPi based looper.
//
// Having multiple configurations allows me to have a "stable" working
// rig, while still developing other "experimental" rigs, so another
// example would be to have a second configuration for the "old" iPad
// rig, that expands on the basic behaviors while I experiment, but
// still allows me to use the stable one at gigs.
//
// There is one configuration that is "special" .. configuration #0
// is dedicated to adjusting system wide parameters, like the LED
// brigtness, and selecting the "active" configuration.
//
// When the TFT is introduced, some of the functionality of the
// configurations *may* be duplicated to the TFT/touch screen,
// and/or removed from the configurations, depending on the
// robustness and practicallity of the TFT/touchscreen.
//
// SECTIONS
//
// Each configuration is broken up into "sections".  Sections
// essentially map to a set of buttons that have similar or
// grouped behavior.
//
// For example, in the "old" iPad rig configuration, there are
// three sections:
//
//      (1) the "synth" section, consisting of the top three
//      rows of buttons, that send out PC (program change)
//      messages for the current synth patch. Only one synth
//      patch can be selected at a time, highlighted in BLUE,
//
//      (2) the "guitar effects" section, on row 3 (zero based)
//      that consists of buttons that toggle on and off GREEN
//      that send CC (or NOTE) messages 0x00 and 0x7f (or NOTE_ON
//      and NOTE_OFF messages with those velocities) as the
//      buttons are toggled.
//
//      (3) the "loop" section that controls Quantiloop via
//      CC 0x00 (down) and CC 0x7f (up) messages, that has its
//      own distinctive coloring and behavior, including mapping
//      a LONG PRESS on the bottom right button to CLEAR_ALL.
//
// Configuration 0 consists of three sections (at this time)
//      (1) one that allows you to adjust the brightness,
//      (2) one that allows you to pick the "sctive" 1 based
//      configuration, and (3) one that allows you to CANCEL,
//      ACCEPT, and/or ACCEPT and SAVE AS DEFAULT the changes
//      that you have made.
//
// All "user" (1 based) configuration SHALL map a long press
//      on the upper right button to change to purple and
//      go to configuration 0.  Configuration 0 will exit
//      back to the given "selected" active configuration.
//
// iPad Initialization (sending specific midi messages upon
//      changing configurations, and upon initial bootup) is
//      a potential issue that will intially be ignored. It is
//      expected that some number of user actions (i.e. long press
//      the CLEAR_ALL button, select a patch, and toggle all the
//      guitar effects on and off) will initially be required
//      on configuration changes, but that they will later be
//      automated as the situation becomes clearer.
//
// A section is a more or less self contained entity, registering
//      for events on a group of buttons and acting on those events.
//      The base configuration class begin() method will clear all
//      button event mappings and turn off all LEDs so that a
//      section only needs concern itself with the positive event
//      registrations and behaviors it is concerned about.
//
//
// MODES
//
//   Sections and configurations *may* implement MODES. That is
//   why we did not use that word for the CONFIGURATIONS. It is
//   beyond the scope of this initial description to describe
//   how MODES might be generically represented and implemented.
//
// UI BUTTONS
//
//   It is tempting to introduce the notition of a uiButton that
//   has a state, knows it's color, knows it's groupings and inter
//   button behavior, but that is also not part of the initial
//   description.

