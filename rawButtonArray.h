#ifndef __rawButtonArray_h
#define __rawButtonArray_h

#include <Arduino.h>

// we output a signal to coumns

#define BUTTON_OUT_LATCH_PIN        3
#define BUTTON_OUT_CLOCK_PIN        4
#define BUTTON_OUT_DATA_PIN         6
#define BUTTON_IN_LOAD_PIN          7
#define BUTTON_IN_CLOCK_ENABLE_PIN  15
#define BUTTON_IN_CLOCK_PIN         16
#define BUTTON_IN_DATA_PIN          17
    // pins in H file for visibility


#define USE_BUTTON_SCAN_TIMER  0
    // does the button scans using an interrupt based timer
    // that pre-empts the main thread.  Care must be taken
    // to not do anything time consuming in the client
    // handleButtonEventFxn() method, which should defer
    // drawing and heavy duty UI to the main thread.
    //
    // It IS envisioned that simple midi messages CAN
    // be sent out in respnose to PRESS events and probably
    // the leds updated .. see discussion of CLICKS vs PRESSes
    

#define NUM_BUTTON_COLS   5
#define NUM_BUTTON_ROWS   5
    // origin top left


// PRESS and RELEASE are sent out as they happen.
//      The basic idea is that there are time critical things you want
//      to do about immediately upon a "PRESS" event, but that the
//      They only require scan processing when the state of the button
//      changes (and is debounced). This may include highlighting a button,
//      and/or sending a midi message.
//
// "XXX_CLICK" events are more UI-like and generally not time critical.
//     It is more or less assumed that you will only register DOUBLE or LONG
//     if you also register CLICK, though PRESS *could* be used with LONG.
//
//     You generally only want ONE of these to happen at a time, even
//        though you might register for more than one of them. So they
//        are implemented that way ... you only get ONE of the three for
//        any given user gesture oh a particular button.
//
//     CLICK event, by itself, only requires prodessing when the
//         state of the button changes. The event is sent right after
//         RELEASE event ..
//     LONG_CLICK adds processing while a button is pressed,
//         so slows the loop down a little bit WHILE the button is
//         pressed.
//     DOUBLE_CLICK is the worst.  There is an internnal define to stop it
//         from sending out both CLICK and THEN the DOUBLE_CLICK events.
//         To do so, it has to suspend sending of CLICK events until it is
//         sure a second, DOUBLE click is not pressed.  This both adds a lag
//         to the CLOCK events, AND takes extra processing in the loop.
//         Best if used sparingly, or not at all.

#define BUTTON_EVENT_PRESS          0x0001
#define BUTTON_EVENT_RELEASE        0x0002
#define BUTTON_EVENT_CLICK          0x0010
#define BUTTON_EVENT_DOUBLE_CLICK   0x0020
#define BUTTON_EVENT_LONG_CLICK     0x0040      //  actually happens while "pressed"
    // individual event types
    
#define BUTTON_ALL_EVENTS           0x00ff
    // for registration only



typedef void handleButtonEventFxn(void *obj, int row, int col, int event);
    // callback method

class rawButton
{
    public:
        
        rawButton()
        {
            m_event_mask = 0;
            m_pressed = 0;
            m_press_time = 0;
            m_debounce_time = 0;
        }
        
        ~rawButton() {}

        int m_event_mask;
        bool m_pressed;
        unsigned m_press_time;
        unsigned m_debounce_time;
        
};




class rawButtonArray
{
    public:
        
        rawButtonArray(void *pObj, handleButtonEventFxn *callback);
        
        static const char *buttonEventName(int event);
        void setButtonEventMask(int row, int col, int mask)
            { m_buttons[row][col].m_event_mask = mask;}
        
        void begin();
        #if !USE_BUTTON_SCAN_TIMER
            void task();
        #endif
        
    private:
        
        void *m_pObj;
        handleButtonEventFxn *m_callback;
        int m_data[NUM_BUTTON_ROWS];
        rawButton m_buttons[NUM_BUTTON_ROWS][NUM_BUTTON_COLS];
        
        #if USE_BUTTON_SCAN_TIMER
            void task();
            static void timer_handler();
            IntervalTimer m_timer;
        #endif
};


#endif // !__myButtonArray_h