#ifndef __rawButtonArray_h
#define __rawButtonArray_h

#include <Arduino.h>

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
            m_event_state = 0;
            m_press_time = 0;
            m_debounce_time = 0;
        }
        
        ~rawButton() {}

        int m_event_mask;
        int m_event_state;
        unsigned m_press_time;
        unsigned m_debounce_time;
        
};




class rawButtonArray
{
    public:
        
        rawButtonArray(void *pObj, handleButtonEventFxn *callback);
        
        static const char *buttonEventName(int event);
        
        int getButtonEventMask(int row, int col)
            { return m_buttons[row][col].m_event_mask; }
        void setButtonEventMask(int row, int col, int mask)
            { m_buttons[row][col].m_event_mask = mask;}
        
        void task();
        
    private:
        
        void *m_pObj;
        handleButtonEventFxn *m_callback;
        int m_data[NUM_BUTTON_ROWS];
        rawButton m_buttons[NUM_BUTTON_ROWS][NUM_BUTTON_COLS];
};


#endif // !__myButtonArray_h