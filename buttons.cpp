#include <myDebug.h>
#include "buttons.h"
#include "expSystem.h"
#include "myLeds.h"


#define dbg_btn 1


#define LONG_PRESS_TIME    800
#define DOUBLE_CLICK_TIME  360

#define DO_DEBOUNCE        1
    // does not seem to be necessary.
    // in timer version, a period of time is implicit, so this should be turned off
    // in loop() version, maybe we are getting by because of display() calls
    // so *this* might still be needed.

#if DO_DEBOUNCE
    #define DEBOUNCE_MILLIS    50
#endif



buttonArray theButtons;
int row_pins[NUM_BUTTON_COLS] = {PIN_BUTTON_OUT0,PIN_BUTTON_OUT1,PIN_BUTTON_OUT2,PIN_BUTTON_OUT3,PIN_BUTTON_OUT4};
int col_pins[NUM_BUTTON_ROWS] = {PIN_BUTTON_IN0,PIN_BUTTON_IN1,PIN_BUTTON_IN2,PIN_BUTTON_IN3,PIN_BUTTON_IN4};


//--------------------------------------
// arrayedButton
//--------------------------------------


arrayedButton::arrayedButton()
{
    // see notes below why event state is not
    // cleared during initDefaults()

    m_event_state = 0;
    initDefaults();
}


void arrayedButton::initDefaults()
{
    m_event_mask = 0;
    m_press_time = 0;
    m_debounce_time = 0;
    m_repeat_time = 0;
    m_default_color = LED_BLUE;
    m_selected_color = LED_CYAN;
    m_touch_color = LED_YELLOW;
}




//--------------------------------------
// buttonArray
//--------------------------------------


buttonArray::buttonArray()
{}


void buttonArray::init()
{
    display(dbg_btn,"buttonArray::init()",0);

    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        pinMode(row_pins[row],OUTPUT);
        digitalWrite(row_pins[row],0);
    }
    for (int col=0; col<NUM_BUTTON_COLS; col++)
        pinMode(col_pins[col],INPUT_PULLDOWN);            // guessing that pins 7 and 8 doent have pulldowns

#if DO_DEBOUNCE
    display(dbg_btn,"    WITH DEBOUNCE!",0);
#endif
}


void buttonArray::clear()
    // we have to be careful about folks calling back into
    // us to do things from button events.  Like starting
    // a new window and resetting all the masks, in the middle
    // of an event.
{
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            m_buttons[row][col].initDefaults();
            setLED(row,col,0);
        }
    }
}



// static
const char *buttonArray::buttonEventName(int event)
{
    if (event == BUTTON_EVENT_PRESS          ) return "PRESS";
    if (event == BUTTON_EVENT_RELEASE        ) return "RELEASE";
    if (event == BUTTON_EVENT_CLICK          ) return "CLICK";
    if (event == BUTTON_EVENT_LONG_CLICK     ) return "LONG_CLICK";
    return "UNKNOWN BUTTON EVENT";
}





void buttonArray::setEventState(int num, int state)
    // retains the state of the pressed bit,
    // but clears the m_press_time (handled)
{
    // display(0,"setEventState num=%d state=%04x",num,state);

    arrayedButton *button = &m_buttons[num / NUM_BUTTON_COLS][num % NUM_BUTTON_COLS];
    state &= ~BUTTON_STATE_PRESSED;
    state |= (button->m_event_state & BUTTON_STATE_PRESSED);
    button->m_event_state = state;
    button->m_press_time = 0;

    int color =
        state & BUTTON_STATE_SELECTED ?
            button->m_selected_color :
        (button->m_event_mask & BUTTON_MASK_TOUCH) &&
        (state & BUTTON_STATE_TOUCHED) ?
            button->m_touch_color :
            button->m_default_color;

    // display(0,"color(num)=%d",color);
    setLED(num,color);
}





void buttonArray::select(int num, int value)
    // -1 == pressed    (this method is NOT called if m_press_time==0)
    //  0 == deselect
    //  1 == select
{
    arrayedButton *button = &m_buttons[num / NUM_BUTTON_COLS][num % NUM_BUTTON_COLS];
    int mask = button->m_event_mask;
    int state = button->m_event_state;

    display(dbg_btn,"select(%d,%d) mask=%04x state=%04x",num,value,mask,state);

    // fake the previous selected bit if from a press
    // and it's a toggle button

    bool selected = value;
    if (value==-1 && (mask & BUTTON_MASK_TOGGLE))
        selected = !(state & BUTTON_STATE_SELECTED);

    // determine the default, unselected, color

    int color = button->m_default_color;
    if (mask & BUTTON_MASK_TOUCH && state & BUTTON_STATE_TOUCHED)
        color = button->m_touch_color;

    if ((state & BUTTON_STATE_SELECTED) != selected)
    {
        if (selected)
        {
            if (mask & BUTTON_MASK_TOGGLE)
            {
                button->m_event_state |= BUTTON_STATE_SELECTED | BUTTON_STATE_TOUCHED;
                color = button->m_selected_color;
            }
            else if (mask & BUTTON_MASK_RADIO)
            {
                int group = BUTTON_GROUP_OF(button->m_event_mask);

                // clear any other selected button in the group

                for (int r=0; r<NUM_BUTTON_ROWS; r++)
                {
                    for (int c=0; c<NUM_BUTTON_COLS; c++)
                    {
                        arrayedButton *b = &m_buttons[r][c];
                        if (b != button &&
                            BUTTON_GROUP_OF(b->m_event_mask) == group &&
                            b->isSelected())
                        {
                            b->m_event_state &= ~BUTTON_STATE_SELECTED;
                            int c2 = b->m_default_color;
                            if (b->m_event_mask & BUTTON_MASK_TOUCH &&
                                b->m_event_state & BUTTON_STATE_TOUCHED)
                                c2 = b->m_touch_color;
                            setLED(r,c,c2);
                        }
                    }
                }

                button->m_event_state |= BUTTON_STATE_SELECTED | BUTTON_STATE_TOUCHED;
                color = button->m_selected_color;
            }
        }
        else
        {
            button->m_event_state &= ~BUTTON_STATE_SELECTED;
        }
    }

    // this may be called by the client from a button event handler.
    // if so, and it effects the current button, then we don't want
    // the rest of the task() code for that button to run ...

    if (value != -1)
        button->m_press_time = 0;

    setLED(num,color);
}





void buttonArray::task()
{
    unsigned time = millis();
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        digitalWrite(row_pins[row],1);

        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            // only poll registered buttons

            arrayedButton *pButton = &m_buttons[row][col];
            int mask = pButton->m_event_mask;
            if (!mask) continue;

            #if DO_DEBOUNCE
                if (time <= pButton->m_debounce_time)
                    continue;
            #endif

            // if state changed, process the button

            int num = row * NUM_BUTTON_COLS + col;
            bool is_pressed = digitalRead(col_pins[col]);
            bool was_pressed = pButton->m_event_state & BUTTON_STATE_PRESSED;
            if (is_pressed != was_pressed)
            {
                display(dbg_btn,"BUTTON(%d,%d) %-6s   mask=%04x  state=%04x",
                    row,col,is_pressed?"DOWN":"UP",mask,pButton->m_event_state);

                #if DO_DEBOUNCE
                    pButton->m_debounce_time = time + DEBOUNCE_MILLIS;
                #endif

                //---------------------------
                // pressed
                //---------------------------

                if (is_pressed)     // button pressed
                {
                    pButton->m_event_state |= BUTTON_STATE_PRESSED | BUTTON_STATE_TOUCHED;

                    if (mask & BUTTON_EVENT_PRESS)
                    {
                        select(num,-1);
                        showLEDs();
                        display(dbg_btn,"BUTTON_EVENT_PRESS(%d,%d)",row,col);
                        theSystem.buttonEvent(row, col, BUTTON_EVENT_PRESS);
                    }
                    else
                    {
                        setLED(row,col,LED_WHITE);
                        showLEDs();
                    }

                    pButton->m_press_time = time;
                }

                //---------------------------
                // released
                //---------------------------
                // only do something if not handled (m_press_time != 0)

                else    // button released
                {
                    pButton->m_event_state &= ~BUTTON_STATE_PRESSED;
                    if (pButton->m_press_time)  // && !(mask & BUTTON_EVENT_PRESS))
                    {
                        pButton->m_press_time = 0;
                        select(num,-1);
                        showLEDs();
                        if (mask & BUTTON_EVENT_RELEASE)
                        {
                            display(dbg_btn,"BUTTON_EVENT_RELEASE(%d,%d)",row,col);
                            theSystem.buttonEvent(row, col, BUTTON_EVENT_RELEASE);
                        }
                        if (mask & BUTTON_EVENT_CLICK)
                        {
                            display(dbg_btn,"BUTTON_EVENT_CLICK(%d,%d)",row,col);
                            theSystem.buttonEvent(row, col, BUTTON_EVENT_CLICK);
                        }
                    }
                }
            }

            //--------------------------------
            // state did not change
            //--------------------------------
            //
            else if (is_pressed && pButton->m_press_time)
            {
                // repeat generates PRESS events

                int dif = millis() - pButton->m_press_time;
                if ((mask & BUTTON_MASK_REPEAT) && dif > 300)
                {
                    // starts repeating after 300ms
                    // starts at 10 per second and accelerates to 100 per second over one second

                    dif -= 300;
                    if (dif > 1000) dif = 1000;

                    unsigned interval = 100;
                    interval -= 90*dif/1000;

                    if (pButton->m_repeat_time > interval)
                    {
                        display(dbg_btn,"repeat BUTTON_EVENT_PRESS(%d,%d)",row,col);
                        theSystem.buttonEvent(row, col, BUTTON_EVENT_PRESS);
                        pButton->m_repeat_time = 0;
                    }
                }

                // which is generally exclusive of long clicks

                else if ((mask & BUTTON_EVENT_LONG_CLICK) && dif > LONG_PRESS_TIME)
                {
                    display(dbg_btn,"BUTTON_EVENT_LONG_CLICK(%d,%d)",row,col);
                    pButton->m_press_time = 0;
                    theSystem.buttonEvent(row, col, BUTTON_EVENT_LONG_CLICK);
                }

            }   // pressed and not handled yet
        }   // for each col

        digitalWrite(row_pins[row],0);

    }   // for each row
}



void  buttonArray::setButtonType(int num, int mask, int default_color, int selected_color, int touch_color)
{
    arrayedButton *pb = &m_buttons[num / NUM_BUTTON_COLS][num % NUM_BUTTON_COLS];
    pb->m_event_mask = mask;
    pb->m_default_color = default_color == -1 ? LED_BLUE : default_color;
    pb->m_selected_color = selected_color == -1 ? LED_CYAN : selected_color;
    pb->m_touch_color = touch_color == -1 ? LED_YELLOW : touch_color;
   	setLED(num,pb->m_default_color);
}


void buttonArray::clearRadioGroup(int group)
{
    display(0,"clearRadioGroup(%d)",group);
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            arrayedButton *button = &m_buttons[row][col];
            if (group == BUTTON_GROUP_OF(button->m_event_mask))
            {
                button->m_event_state &= ~BUTTON_STATE_TOUCHED;
                    // really clear em ...
                select(row*NUM_BUTTON_COLS+col,0);
            }
        }
    }
    showLEDs();
}
