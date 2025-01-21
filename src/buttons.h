#ifndef __buttons_h__
#define __buttons_h__

#include "defines.h"

// PRESS and RELEASE are sent out as they happen.
// Buttons with PRESS or RELEASE cannot be
//     CLICK or LONG_CLICK.
// CLICK is sent out on the button being released.
// LONG_CLICK is sent out after the button is pressed for a certain amount of time.
// You will only get one of PRESS, CLICK or LONG_CLICK.

#define BUTTON_EVENT_PRESS          0x0001
#define BUTTON_EVENT_RELEASE        0x0002
#define BUTTON_EVENT_CLICK          0x0010
#define BUTTON_EVENT_LONG_CLICK     0x0040
    // types of events to register on, and which are returned in events

// CLICK buttons are lit up in with their pressed color while they are
// pressed until they are released.  By default they return to
// their default/touched/selected color AFTER the user event is called.
// You may use BUTTON_MASK_USER_DRAW to preent that behavior so that your
// user event method can set the final button color.


#define BUTTON_MASK_REPEAT          0x0400
    // for use with BUTTON_EVENT_PRESS only
    // will repeat the user event as long as
    // button is pressed
#define BUTTON_MASK_TOGGLE          0x0100
    // for use with BUTTON_EVENT_CLICK only
    // button will toggle its selected state and
    // change color between it's selected color and
    // it's default/touch color.  Does not
    // make sense with BUTTON_MASK_USER_DRAW
#define BUTTON_MASK_TOUCH           0x0800
    // touch_color will oeveride default
    // color once it has ever been toucned (touch bit)
    // since the last clear()
#define BUTTON_MASK_USER_DRAW       0x1000
    // User is responsible for setting the final
    // color(s) in their event methods.  Incompatible
    // with TOGGLE ..


#define BUTTON_TYPE_CLICK       (BUTTON_EVENT_CLICK)
#define BUTTON_TYPE_LONG_CLICK  (BUTTON_EVENT_LONG_CLICK)
#define BUTTON_TYPE_TOGGLE      (BUTTON_EVENT_CLICK | BUTTON_MASK_TOGGLE)


class buttonArray;
    // forward

class arrayedButton
{
    public:

        arrayedButton();
        ~arrayedButton() {}

        void initDefaults();

        bool isSelected();
        bool isPressed();
        bool hasBeenTouched();

        void addLongClickHandler()  { m_event_mask |= BUTTON_EVENT_LONG_CLICK; }

    private:

        friend class buttonArray;

        int m_event_mask;
        int m_event_state;
        unsigned m_press_time;
        elapsedMillis m_repeat_time;

        int m_default_color;
        int m_pressed_color;
        int m_selected_color;
        int m_touch_color;

};




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

        void setButtonType(
            int num,
            int mask,
            int default_color=-1,
            int selected_color=-1,
            int touch_color=-1,
            int pressed_color=-1);

        void setButtonColor(int num,int color);
            // used to explicitly set the default_color and
            // redisplay the button

        void select(int num, int pressed);
            // 1=pressed, -1=long_click, 0=releaed

    private:

        int m_data[NUM_BUTTON_ROWS];
        arrayedButton m_buttons[NUM_BUTTON_ROWS][NUM_BUTTON_COLS];


};



extern buttonArray theButtons;



#endif // !__buttons_h__