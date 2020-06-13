#include <myDebug.h>
#include "buttons.h"
#include "expSystem.h"
#include "myLeds.h"


#define dbg_btn 1

    
#define LONG_PRESS_TIME    800
#define DOUBLE_CLICK_TIME  360

#define DISTINCT_DOUBLE_CLICK 1
    // if defined, we delay the SINGLE CLICK until we are sure
    // we are out of the zone

#define DO_DEBOUNCE        1
    // does not seem to be necessary.
    // in timer version, a period of time is implicit, so this should be turned off
    // in loop() version, maybe we are getting by because of display() calls
    // so *this* might still be needed.

#if DO_DEBOUNCE
    #define DEBOUNCE_MILLIS    50
#endif

#define BUTTON_STATE_PRESSED  0x0001
#define BUTTON_STATE_REPORTED 0x0002



buttonArray theButtons;
int row_pins[NUM_BUTTON_COLS] = {PIN_BUTTON_OUT0,PIN_BUTTON_OUT1,PIN_BUTTON_OUT2,PIN_BUTTON_OUT3,PIN_BUTTON_OUT4};
int col_pins[NUM_BUTTON_ROWS] = {PIN_BUTTON_IN0,PIN_BUTTON_IN1,PIN_BUTTON_IN2,PIN_BUTTON_IN3,PIN_BUTTON_IN4};



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
    



// static
const char *buttonArray::buttonEventName(int event)
{
    if (event == BUTTON_EVENT_PRESS          ) return "PRESS";
    if (event == BUTTON_EVENT_RELEASE        ) return "RELEASE";
    if (event == BUTTON_EVENT_CLICK          ) return "CLICK";
    if (event == BUTTON_EVENT_DOUBLE_CLICK   ) return "DOUBLE_CLICK";
    if (event == BUTTON_EVENT_LONG_CLICK     ) return "LONG_CLICK";
    return "UNKNOWN BUTTON EVENT";
}



void buttonArray::task()
{
    unsigned time = millis();
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        digitalWrite(row_pins[row],1);
        
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            arrayedButton *pButton = &m_buttons[row][col];
            
            #if DO_DEBOUNCE
                if (time > pButton->m_debounce_time)
            #endif
            {
                bool is_pressed = digitalRead(col_pins[col]);
                
                // if state changed, save new bit to data
                // and process the button
                
                if ((pButton->m_event_state & BUTTON_STATE_PRESSED) != is_pressed)
                {
                    display(dbg_btn,"BUTTON(%d,%d) %s",row,col,is_pressed?"DOWN":"UP");
                    
                    // set or clear the state bit
                    
                    if (is_pressed)
                        pButton->m_event_state |= BUTTON_STATE_PRESSED;
                    else
                        pButton->m_event_state &= ~BUTTON_STATE_PRESSED;
                    
                    #if DO_DEBOUNCE
                        pButton->m_debounce_time = time + DEBOUNCE_MILLIS;
                    #endif

                    if (is_pressed)     // button pressed
                    {
                        if (pButton->m_event_mask)
                        {
                            setLED(row,col,LED_WHITE);
                            showLEDs();
                        }
                        
                        if (pButton->m_event_mask & BUTTON_EVENT_PRESS)
                           theSystem.buttonEvent(row, col, BUTTON_EVENT_PRESS);
                        if ((pButton->m_event_mask & BUTTON_EVENT_DOUBLE_CLICK) &&
                            pButton->m_press_time &&
                            (time < pButton->m_press_time + DOUBLE_CLICK_TIME))
                        {
                            pButton->m_press_time = 0;
                            theSystem.buttonEvent(row, col, BUTTON_EVENT_DOUBLE_CLICK);
                        }
                        else
                        {
                            pButton->m_press_time = time;
                        }
                    }
                    else    // button released
                    {
                        if (pButton->m_event_mask & BUTTON_EVENT_RELEASE)
                            theSystem.buttonEvent(row, col, BUTTON_EVENT_RELEASE);
                            
                        #if DISTINCT_DOUBLE_CLICK
                            if (!(pButton->m_event_mask & BUTTON_EVENT_DOUBLE_CLICK))
                                // don't do below code for button that have registered
                                // double click ... 
                        #endif
                        if ((pButton->m_event_mask & BUTTON_EVENT_CLICK) &&
                            pButton->m_press_time)
                            theSystem.buttonEvent(row, col, BUTTON_EVENT_CLICK);
                    }   
                }
                else if (is_pressed &&
                         (pButton->m_event_mask & BUTTON_EVENT_LONG_CLICK) &&
                         pButton->m_press_time &&
                        (time > pButton->m_press_time + LONG_PRESS_TIME))
                {
                    pButton->m_press_time = 0;
                    theSystem.buttonEvent(row, col, BUTTON_EVENT_LONG_CLICK);
                }
            #if DISTINCT_DOUBLE_CLICK
                else if (
                    (pButton->m_event_mask & BUTTON_EVENT_DOUBLE_CLICK) &&
                    (pButton->m_event_mask & BUTTON_EVENT_CLICK) &&
                    !is_pressed &&
                    pButton->m_press_time &&
                    (time > pButton->m_press_time + DOUBLE_CLICK_TIME))
                {
                    pButton->m_press_time = 0;
                    theSystem.buttonEvent(row, col, BUTTON_EVENT_CLICK);
                }
            #endif
            }
        }   // for each col
        
        //delay(200);
        digitalWrite(row_pins[row],0);
        
    }   // for each row
}


