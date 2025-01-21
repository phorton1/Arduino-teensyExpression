#include <myDebug.h>
#include "buttons.h"
#include "expSystem.h"
#include "myLeds.h"

#define dbg_btn 1


#define BUTTON_STATE_PRESSED       0x0001
    // is the button currently pressed
    // long term: has the button been pressed since the last clear() ?
#define BUTTON_STATE_SELECTED      0x4000
    // used for buttons that can be toggled on and off
#define BUTTON_STATE_TOUCHED       0x8000


#define LONG_PRESS_TIME    800
#define DOUBLE_CLICK_TIME  360

#define DEBOUNCE_MILLIS    50



buttonArray theButtons;
int row_pins[NUM_BUTTON_COLS] = {PIN_BUTTON_OUT0,PIN_BUTTON_OUT1,PIN_BUTTON_OUT2,PIN_BUTTON_OUT3,PIN_BUTTON_OUT4};
int col_pins[NUM_BUTTON_ROWS] = {PIN_BUTTON_IN0,PIN_BUTTON_IN1,PIN_BUTTON_IN2,PIN_BUTTON_IN3,PIN_BUTTON_IN4};
volatile bool in_button = 0;


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
    m_repeat_time = 0;
    m_default_color = LED_BLUE;
    m_pressed_color = LED_WHITE;
    m_selected_color = LED_CYAN;
    m_touch_color = LED_YELLOW;
}

bool arrayedButton::isPressed()
{
	return m_event_state & BUTTON_STATE_PRESSED;
}
bool arrayedButton::isSelected()
{
	return m_event_state & BUTTON_STATE_SELECTED;
}
bool arrayedButton::hasBeenTouched()
{
	return m_event_state & BUTTON_STATE_TOUCHED;
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


void buttonArray::setButtonColor(int num,int color)
{
    arrayedButton *pb = &m_buttons[num / NUM_BUTTON_COLS][num % NUM_BUTTON_COLS];
    pb->m_default_color = color;
    setLED(num,pb->m_default_color);
	showLEDs(true);
}



void buttonArray::setButtonType(int num, int mask, int default_color, int selected_color, int touch_color, int pressed_color)
{
    arrayedButton *pb = &m_buttons[num / NUM_BUTTON_COLS][num % NUM_BUTTON_COLS];
    pb->m_event_mask = mask;
    pb->m_default_color = default_color == -1 ? LED_BLUE : default_color;
    pb->m_pressed_color = pressed_color == -1 ? LED_WHITE : pressed_color;
    pb->m_selected_color = selected_color == -1 ? LED_CYAN : selected_color;
    pb->m_touch_color = touch_color == -1 ? LED_YELLOW : touch_color;
   	if (!(mask & BUTTON_MASK_USER_DRAW))
        setLED(num,pb->m_default_color);
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



void buttonArray::select(int num, int pressed)
	// Update the state of toggle buttons
    // pressed  1 == freshly pressed
    // pressed  0 == released (for CLICK or RELEASE)
	// pressed -1 == long click reached
{
    arrayedButton *button = &m_buttons[num / NUM_BUTTON_COLS][num % NUM_BUTTON_COLS];
    int mask = button->m_event_mask;
    int state = button->m_event_state;
	bool leds_changed = false;

    display(dbg_btn,"select(%d,%d) mask=%04x state=%04x",num,pressed,mask,state);

	// set presses to the button's pressed_color except for REPEAT buttons

	if (pressed == 1 && !(mask & BUTTON_MASK_REPEAT))
	{
		setLED(num,button->m_pressed_color);
		leds_changed = true;
	}

	// user drawn buttons dont do anything else

	if ((pressed != 1) &&
		!(mask & BUTTON_MASK_USER_DRAW))
	{
		int color = button->m_default_color;
		if (mask & BUTTON_MASK_TOUCH)
			color = button->m_touch_color;
		if (pressed == 0 && mask & BUTTON_MASK_TOGGLE)
		{
			bool new_selected = !(state & BUTTON_STATE_SELECTED);
			if (new_selected)
				button->m_event_state |= BUTTON_STATE_SELECTED;
			else
				button->m_event_state &= ~BUTTON_STATE_SELECTED;
			if (new_selected)
				color = button->m_selected_color;
		}
		setLED(num,color);
		leds_changed = true;
	}

	if (pressed != 1)
		button->m_press_time = 0;

	if (leds_changed)
		showLEDs();
}



void buttonArray::task()
{
    unsigned time = millis();
	static unsigned last_time = 0;
	if (time < last_time + DEBOUNCE_MILLIS)
		return;

	// re-entrancy protection
	
	if (in_button)
		return;
	in_button = 1;

	last_time = time;
	

	// read all the buttons at once

	bool down[NUM_BUTTON_ROWS * NUM_BUTTON_COLS];
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        digitalWrite(row_pins[row],1);
		delayMicroseconds(20);	// let voltage stabilize
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            int num = row * NUM_BUTTON_COLS + col;
            down[num] = digitalRead(col_pins[col]);
		}
        digitalWrite(row_pins[row],0);
	}

	// then process them

    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            // only act on registered buttons

            arrayedButton *pButton = &m_buttons[row][col];
            int mask = pButton->m_event_mask;
            if (!mask) continue;

            // if state changed, process the button

            int num = row * NUM_BUTTON_COLS + col;
            bool is_pressed = down[num];
            bool was_pressed = pButton->m_event_state & BUTTON_STATE_PRESSED;
            if (is_pressed != was_pressed)
            {
                display(dbg_btn,"BUTTON(%d,%d) %-6s   mask=%04x  state=%04x",
                    row,col,is_pressed?"DOWN":"UP",mask,pButton->m_event_state);

                //---------------------------
                // pressed
                //---------------------------

                if (is_pressed)     // button pressed
                {
                    pButton->m_event_state |= BUTTON_STATE_PRESSED | BUTTON_STATE_TOUCHED;
					select(num,1);
                    if (mask & BUTTON_EVENT_PRESS)
                    {
                        display(dbg_btn,"BUTTON_EVENT_PRESS(%d,%d)",row,col);
                        theSystem.buttonEvent(row, col, BUTTON_EVENT_PRESS);
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
                    if (pButton->m_press_time)
                    {
                        select(num,0);

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
					select(num,-1);
                    theSystem.buttonEvent(row, col, BUTTON_EVENT_LONG_CLICK);
                }

            }   // pressed and not handled yet
        }   // for each col
    }   // for each row

	in_button = 0;

}	// buttonManager::task();





