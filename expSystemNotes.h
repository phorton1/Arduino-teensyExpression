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
//
//     Patches are "top levell" expWindows that use 1 based numbering
//     The configSystem object overuses patch #0 in current implementation.
//     The intention is that there can then be modal dialog "windows" that
//     can be called from either.
//
//     The design and implementation are in flux.





// configSystem  as-is currently
//
//      Brightness          nn
//          has quick keys
//          is only implement "integerOption" with a terminal mode
//      Patch               name
//          has quick keys for patches 1-5
//          and system is currently overusing patch0 for the configSystem
//      Pedals
//          Calibrate Pedals
//               Pedal1
//               Pedal2
//               Pedal3
//               Pedal4
//          Configure Pedals
//               Pedal1
//               Pedal2
//               Pedal3
//               Pedal4
//      System
//          MidiHost        ON/OFF
//          Serial Port     ON/OFF
//          Calibrate Touch
//      Spoof FTP       ON/OFF

// NEW
//
//     Rotaries are considered to be available to patches and
//         are not used in the sysConfig or other 'built-in' UI.
//         The whole notion of "curves" on rotaries may need to be
//         addressed.
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
//                  need to be stored in the EEPROM
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
//              Reset all EEPROM values to 255
//
//
// ABREVIATED:
//
//      Brightness          nn                      default(50)
//      Patch               name                    default(1=OldRig)
//      FTP
//          Spoof FTP       ON/OFF
//          FTP Port        None, Host, Remote
//          Tuner           -> modal window
//          Sensitivity     -> modal window
//          Poly Mode       Mono, Poly              default(1=Poly)
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
//      Midi Monitor (needs scollable config window)
//              Midi Monitor        OFF, USB, Serial        default(USB)
//              Sysex               OFF, ON, Detail         default(1==ON)
//              ActiveSense         OFF, ON                 default(0==OFF)
//              Performance CCs     OFF, ON,                default(1=ON)
//              Tuning Msgs         OFF, ON                 default(1==ON)
//              Note Info           OFF, ON                 default(1==ON)
//              Volume Level        OFF, ON                 default(1==ON)
//              Battery Level       OFF, ON                 default(1==ON)
//      System Debugging    Off, USB, Serial            default(USB)
//            needs scrollable config window
//      Factory Reset
//
// Maybe use the title bar as the "back/save/longpress=save to EEPROM"
//      button for touch screen in configSystem?
// Have to try drag scrolling, but don't have high hopes.


// IMPLEMENT MODAL WINDOWS (perhaps configSystem becomes one)
//
// ALTERNATIVE 1
//
//      IMPLEMENT touch screen calibration in terms of modal window
//      Work out configSystem with touch screen.
//
// ALTERNATIVE 2
//
//     IMPLEMENT scrolling configSystem
//     implement remote FTP stuff
//     implement all options above
//     implement pedal calibration
//     implement at least part of pedal configuration
//          from curves.cpp and curves.h stuff
