#ifndef __buttons_h__
#define __buttons_h__

#include "defines.h"

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
//     there is no DOUBLE_CLICK .. it sucks

#define BUTTON_EVENT_PRESS          0x0001
#define BUTTON_EVENT_RELEASE        0x0002
#define BUTTON_EVENT_CLICK          0x0010
#define BUTTON_EVENT_LONG_CLICK     0x0040      //  actually happens while "pressed"
    // individual event types

#define BUTTON_MASK_TOGGLE          0x0100
#define BUTTON_MASK_RADIO           0x0200
#define BUTTON_MASK_REPEAT          0x0400
#define BUTTON_MASK_TOUCH           0x0800

#define BUTTON_GROUP_MASK           0xF000

#define BUTTON_GROUP(n)             ( ((n) & 0x7) << 12 )
#define BUTTON_GROUP_OF(i)          ( ((i) >> 12) & 0x7 )


#define BUTTON_STATE_PRESSED       0x0001
    // is the button currently pressed
#define BUTTON_STATE_TOUCHED       0x8000
    // Has the button been pressed since the last clear() ?
#define BUTTON_STATE_SELECTED      0x4000
    // used for buttons that can be toggled on and off
    // as well as for buttons in radio groups


class arrayedButton
{
    public:

        arrayedButton();
        ~arrayedButton() {}

        void initDefaults();

        int m_event_mask;
        int m_event_state;
        unsigned m_press_time;
            // == 0 is used as "event handled"
        unsigned m_debounce_time;
        elapsedMillis m_repeat_time;

        int m_default_color;
        int m_selected_color;
        int m_touch_color;

        bool isSelected()       { return m_event_state & BUTTON_STATE_SELECTED; }

};


#define BUTTON_TYPE_CLICK       (BUTTON_EVENT_CLICK)
#define BUTTON_TYPE_TOGGLE      (BUTTON_EVENT_CLICK | BUTTON_MASK_TOGGLE)
#define BUTTON_TYPE_RADIO(n)    (BUTTON_EVENT_CLICK | BUTTON_MASK_RADIO | BUTTON_GROUP(n))


class buttonArray
{
    public:

        buttonArray();


        void init();        // called once
        void clear();       // called on new windows
        void task();

        static const char *buttonEventName(int event);
        arrayedButton *getButton(int num)            { return &m_buttons[num / NUM_BUTTON_COLS][num % NUM_BUTTON_COLS]; }
        arrayedButton *getButton(int row, int col)   { return &m_buttons[row][col]; }

        void setButtonType(int num, int mask, int default_color=-1, int selected_color=-1, int touch_color=-1);
        void setEventState(int num, int state);

        void clearRadioGroup(int group);
        void select(int num, int value);
            //  -1 == pressed (internal use only)
            //  0 == deselect, 1 == select

    private:

        int m_data[NUM_BUTTON_ROWS];
        arrayedButton m_buttons[NUM_BUTTON_ROWS][NUM_BUTTON_COLS];


};



extern buttonArray theButtons;



#endif // !__buttons_h__