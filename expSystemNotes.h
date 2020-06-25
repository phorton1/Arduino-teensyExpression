// The entire program consists of the "expression system".
//
// TIME SLICING AND INTERRUPTS
//
// (1) the loop() method and things called from it are thought of
//     as the "ui thread" and allowed to take substantial time.
// (2) the midi host has a usb irq handler that immediately sends
//     anything from the host to the usb (teensyduino) device, and
//     is the most important performance consideration in the system.
// (3) The expSystem implements two timer interrupt loops that perform
//     other non-ui processing.  One of them (the critical_timer_handler)
//     is called approx 1000 times a second,  the other one (the non-critical
//     timer_handler) is called about 200 times a seond.
//
//  MIDI HOST EVENT IRQ (myMidihost, midi1)
//
//      sends midi events directly to teensyduino output device
//      and enques them for subsequent processing by rest of system
//
//  CRITICAL_TIMER_HANDLER
//
//      checks teensyduino (input) midi device and if any events found
//      sends them directly to the midi host, and enqueues them for
//      subsequent processing by rest of system
//
//  TIMER_HANDLER
//
//      polls the buttons, rotaries, and pedals generating "events"
//      based on them.   The patches (window) can "handle" those
//      events, otherwise system functionality takes place.
//
//      the timer_handler also dequeues and prodesses midi messages
//      that were enqueued during the midi host IRQ or critical_timer_handler.
//      it *should* defer UI to the ui thread, but the midiQueue currently also
//      acts as the midi event montior at this time.
//
//  UI_THREAD - loop() calls updateUI()
//
//      updates the TFT mostly.
//      *should* implement the midi monitor in expSystem base class.
//
//
// PATCHES vs WINDOWS vs the configSystem object
//
//     "Patches" are "top levell" expWindows that use 1 based numbering
//     The configSystem object overuses patch #0 in current implementation.
//
//     "Windows" are modal dialog windows with a return value mechanism
//     that can be invoked in a stack-like manner.


//------------------------------------------------------------------------
// UI GUIDELINES
//------------------------------------------------------------------------
//
// THE_SYSTEM_BUTTON
//
//     The top right button has been delegated to the system and should be used
//     consistently.  It is always "purple" in patches and the system configuration.
//
//     In a patch, a long press means "go to the system configuration
//     window" (settings) and a short click, if using an FTP controller, goes to
//     ftp tuner/sensitivity window pair.
//
//     In the system configuration window, a long press means "save the settings
//     persistently" and a short click means "return to the patch".  The button
//     next to it is used as cancel on a short click, and reboot system on a
//     long click.
//
//     In a dialog window the system_button should generally be implemented to
//     mean "ok" and should be green.


// MANAGEMENT OF PREFS VS CONFIGURATION-save_global_prefs AND cancel
// VS THE OBJECTS THAT BEHAVE BASED UPON THEM.
//
// There is currently almost a hodgepodge of how things are affected
// and saved in the configuration window.

//
//     If and when I implement "serial midi to the rPi looper"
//         I may need to make that exclusive of sending debugging
//         output to the serial port.
//
//     The MIDI_HOST will always be "ON"
//     The SERIAL_PORT will always be "ON"
//     The midiMonitor (midiQueue) will get, and show my outgoing midi commands
//
//
//      Brightness          nn
//          has quick keys
//          is only implement "integerOption" with a terminal mode
//      Patch               name
//          has quick keys for patches 1-5
//          other patches can be accessed with option
//
//      FTP
//          Spoof FTP       ON/OFF
//              implies there's an FTP device in the host socket
//              but we may still want to know about the FTP device
//              even if we are running as a "teensyExpression" device
//              ON ==> FTP Port is "Host"
//          FTP Port        None, Host, Remote
//
//              This is where it gets fun.
//
//              If we are spoofing FTP, then we may need to be concerned
//                  about the FTP editor munging up our commands or expected
//                  behavior.
//              For now, if FTP Port=="Host" we just send stuff to the dongle,
//                  and ignore the fact that the Editor might mung our commands
//                  or the state of the dongle in the meantime.
//              Remote
//                  periodically send a sysex device ID request (and/or monitor
//                  activeSense) to see if we can find, and interact with, an
//                  FTP dongle.
//
//              One way or the other, we need to know if there is an FTP dongle,
//                  and where it is to enable and make the FTP battery monitor
//                  and the FTP tuner window work.
//
//          Tuner       -> modal window, quick key lower right
//          Sensitivity -> modal window, quick key lower left
//
//              I am torn with having these be tied to a "quick key
//              available in any patch window (i.e. CLICK on the upper
//              right button, and a way to change between them as modal
//              windows).
//
//              Also. as is, (a) the Sensitivity window *should* include
//                  the FTP "Touch Sensitivity" parameter which is NOT a split
//                  parameter, AND have a tendency to think that the
//                  dynamic range (sensitivity) and offset, and touch sensitivity,
//                  will need to be overridable by patches.
//
//             the string sensitivity is stored on the FTP controller,
//                  but the dynamic range, dynamic offset, and touch sensitivity,
//                  need to be stored in the prefs
//
//                  Same with POLY MODE.
//
//          Poly Mode:              "Mono" or "Poly"
//
//                  I need *some* way to play around with this.
//                  Having it buried here as an option is obtuse.
//                  It really should be related to the specific
//
//                  sampletank-patch seleced in my patchOldRig,
//                  and there should be a whole new bank of sampleTank
//                  patch numbers to play with
//
//                  this implies a PerformanceFilter/Layer object if
//                  I'm hosting the FTP vs. SampleTank patches with
//                  six things, and control of them (i.e. volume)
//                  from my program.
//
//                  For now I will just send out the messages and
//                  do SampleTank by hand ?!?
//
//
//      Pedals
//          Calibrate Pedals
//               Pedal1  -> modal window
//               Pedal2  -> modal window
//               Pedal3  -> modal window
//               Pedal4  -> modal window
//          Configure Pedals (default configuration can be overridden by patches?!?)
//               Pedal1  -> modal window
//               Pedal2  -> modal window
//               Pedal3  -> modal window
//               Pedal4  -> modal window
//      Calibrate Touch  -> modal window
//
//      Midi Monitor
//
//           this is an early version of such a beast that
//           should later filter on cables, channels, and other
//           things ala midiOx
//
//              Midi Monitor        OFF, USB, Serial        default(USB)
//              Sysex               OFF, ON, Detail         default(1==ON)
//              ActiveSense         OFF, ON                 default(0==OFF)
//              Performance CCs     OFF, ON,                default(1=ON)
//
//           the following are specific to a known FTP device
//
//              Tuning Msgs         OFF, ON                 default(1==ON)
//              Note Info           OFF, ON                 default(1==ON)
//              Volume Level        OFF, ON                 default(1==ON)
//              Battery Level       OFF, ON                 default(1==ON)
//
//      System Debugging    Off, USB, Serial            default(USB)
//      Factory Reset based on --> modal OK/Cancel dialog and result
//              Reset all pref values to 255
//
//
// ABREVIATED:
//
//      Brightness          nn                      default(50)
//                  PREF_BRIGHTNESS
//      Patch               name                    default(1=OldRig)
//                  PREF_PATCH_NUM
//      FTP
//          Mode            DEFAULT/MONO/POLY       default(255=take whatever is found on device, 1=poly, 0=mono)
//                  PREF_FTP_POLY_MODE
//          Spoof FTP       ON/OFF
//                  PREF_FTP_SPOOF
//          FTP Port        None, Host, Remote
//                  PREF_FTP_PORT
//          Tuner           -> modal window
//          Sensitivity     -> modal window
//                  PREF_FTP_TOUCH_SENS
//                  PREF_FTP_DYN_RANGE
//                  PREF_FTP_DYN_OFFSET
//      Host
//          Perf Filter     ON/OFF                  default=OFF (performance filter)
//              removes anything from cable 1
//              and all but note-on/off and pitch bend from cable 0
//                  PREF_PERF_FILTER
//          Split           None    whatever comes in, goes out
//                          1+5     experimental splits
//                          2+4
//                          3+3
//                          4+2
//                          1+5
//
//      Pedals
//          PREF_PEDAL0
//          PREF_PEDAL1         PREF_PEDAL0 + PREF_BYTES_PER_PEDAL
//          PREF_PEDAL2         PREF_PEDAL0 + 2*PREF_BYTES_PER_PEDAL
//          PREF_PEDAL3         PREF_PEDAL0 + 3*PREF_BYTES_PER_PEDAL
//
//      Follows                 PREF_PEDAL0 + 4*PREF_BYTES_PER_PEDAL
//                              PREF_PEDAL0 + 4 * 20 = PREF_PEDAL0 + 80

//             PREF_BYTES_PER_PEDAL   8 + 2 * 6
//
//          PREF_PEDAL_CALIB_MIN_OFFSET     0
//          PREF_PEDAL_CALIB_MAX_OFFSET     2
//          PREF_PEDAL_VALUE_MIN_OFFSET     4
//          PREF_PEDAL_VALUE_MAX_OFFSET     5
//          PREF_PEDAL_CURVE_TYPE_OFFSET    6
//
//          PREF_PEDAL_POINTS_OFFSET        8       // = 8, and there are 2 of what follows
//              PEDAL_POINTS_OFFSET_X       0
//              PEDAL_POINTS_OFFSET_Y       2
//              PEDAL_POINTS_OFFSET_WEIGHT  4
//              PEDAL_POINT_PREF_SIZE       6
//              MAX_PEDAL_CURVE_POINTS      2 (from pedals.h)

//          Calibrate Pedals
//               Pedal1  -> modal window
//               Pedal2  -> modal window
//               Pedal3  -> modal window
//               Pedal4  -> modal window
//          Configure Pedals (default configuration can be overridden by patches?!?)
//               Pedal1  -> modal window
//               Pedal2  -> modal window
//               Pedal3  -> modal window
//               Pedal4  -> modal window
//      Calibrate Touch  -> modal window

//      Midi Monitor (needs scollable config window)
//          PREF_MONITOR_xx
//              Midi Monitor        OFF, USB, Serial        default(USB)
//              Show Filtered       OFF, ON                 default(ON)
//              Sysex               OFF, ON, Detail         default(1==ON)
//              ActiveSense         OFF, ON                 default(0==OFF)
//              Performance CCs     OFF, ON,                default(1=ON)
//              Tuning Msgs         OFF, ON                 default(1==ON)
//              Note Info           OFF, ON                 default(1==ON)
//            ftp only:
//              Volume Level        OFF, ON                 default(1==ON)
//              Battery Level       OFF, ON                 default(1==ON)
//      System Debugging            Off, USB, Serial        default(USB)
//          PREF_DEBUG_PORT
//      Factory Reset


// Have to try drag scrolling, but don't have high hopes.


//     IMPLEMENT scrolling configSystem
//     implement remote FTP stuff
//     implement all options above
//     implement pedal calibration
//     implement at least part of pedal configuration
//          from curves.cpp and curves.h stuff



// The config options work in one of two ways:
//     type (A)
//          take a copy of an actual system value,
//          modify the original value, and restore it
//          if the user cancels
//
//     type (B)
//          modify a value in memory, and only set
//          into the actual system value if the user
//          accepts the changes.
//
//


//     Rotaries are considered to be available to patches and
//         are not used in the sysConfig or other 'built-in' UI.
//         The whole notion of "curves" on rotaries may need to be
//         addressed.
