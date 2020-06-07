#include "myDebug.h"
#include "rawButtonArray.h"
#include "myLeds.h"


static rawButtonArray *s_pThis = 0;

int row_pins[5] = {24,25,26,27,28};
int col_pins[5] = {29,30,31,32,33};
    // 32 is used for the LED output pin (on Serial4)
    // to keep pin 5 open which would otherwise use Serial1
    

    
#define LONG_PRESS_TIME    800
#define DOUBLE_CLICK_TIME  360
    // You want this as small as possible if DISTINCT_DOUBLE_CLICK
    // is defined, as this introduces a "lag" on regular CLICK events.
    
// Schemes ....

#define DISTINCT_DOUBLE_CLICK 1
    // if defined, we delay the SINGLE CLICK until we are sure
    // we are out of the zone

#define DO_DEBOUNCE        0
    // does not seem to be necessary.
    // in timer version, a period of time is implicit, so this should be turned off
    // in loop() version, maybe we are getting by because of display() calls
    // so *this* might still be needed.

#if DO_DEBOUNCE
    #define DEBOUNCE_MILLIS    5
#endif

#define BUTTON_STATE_PRESSED  0x0001
#define BUTTON_STATE_REPORTED 0x0002


rawButtonArray::rawButtonArray(void *pObj, handleButtonEventFxn *callback)
{
    s_pThis = this;
    m_pObj = pObj;
    m_callback = callback;

    display(0,"rawButtonArray::begin()",0);

    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        pinMode(row_pins[row],OUTPUT);
        digitalWrite(row_pins[row],0);
    }
    for (int col=0; col<NUM_BUTTON_COLS; col++)
        pinMode(col_pins[col],INPUT_PULLDOWN);            // guessing that pins 7 and 8 doent have pulldowns
    
#if DO_DEBOUNCE
    display(0,"    WITH DEBOUNCE!",0);
#else
    display(0,"    NO DEBOUNCE!",0);
#endif
    
}
    



// static
const char *rawButtonArray::buttonEventName(int event)
{
    if (event == BUTTON_EVENT_PRESS          ) return "PRESS";
    if (event == BUTTON_EVENT_RELEASE        ) return "RELEASE";
    if (event == BUTTON_EVENT_CLICK          ) return "CLICK";
    if (event == BUTTON_EVENT_DOUBLE_CLICK   ) return "DOUBLE_CLICK";
    if (event == BUTTON_EVENT_LONG_CLICK     ) return "LONG_CLICK";
    return "UNKNOWN BUTTON EVENT";
}


void printBinary(unsigned b)
{
    Serial.print("B");
    for (int i=7; i>=0; i--)
        Serial.print(((b>>i) & 1));
}

void printTwoBinary(unsigned b1, unsigned b2)
{
    Serial.print("    b1=");
    printBinary(b1);
    Serial.print("    b2=");
    printBinary(b2);
    Serial.println();
}


void rawButtonArray::task()
{
    if (!m_callback)
    {
        my_error("NO CALLBACK ROUTINE in rawButtonArray",0);
        return;
    }
    
    unsigned time = millis();
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        digitalWrite(row_pins[row],1);
        
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            rawButton *pButton = &m_buttons[row][col];
            
            #if DO_DEBOUNCE
                if (time > pButton->m_debounce_time)
            #endif
            {
                bool is_pressed = digitalRead(col_pins[col]);
                
                // if state changed, save new bit to data
                // and process the button
                
                if ((pButton->m_event_state & BUTTON_STATE_PRESSED) != is_pressed)
                {
                    display(0,"BUTTON(%d,%d) %s",row,col,is_pressed?"DOWN":"UP");
                    
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
                            (m_callback)(m_pObj, row, col, BUTTON_EVENT_PRESS);
                        if ((pButton->m_event_mask & BUTTON_EVENT_DOUBLE_CLICK) &&
                            pButton->m_press_time &&
                            (time < pButton->m_press_time + DOUBLE_CLICK_TIME))
                        {
                            pButton->m_press_time = 0;
                            (m_callback)(m_pObj, row, col, BUTTON_EVENT_DOUBLE_CLICK);
                        }
                        else
                        {
                            pButton->m_press_time = time;
                        }
                    }
                    else    // button released
                    {
                        if (pButton->m_event_mask & BUTTON_EVENT_RELEASE)
                            (m_callback)(m_pObj, row, col, BUTTON_EVENT_RELEASE);
                            
                        #if DISTINCT_DOUBLE_CLICK
                            if (!(pButton->m_event_mask & BUTTON_EVENT_DOUBLE_CLICK))
                                // don't do below code for button that have registered
                                // double click ... 
                        #endif
                        if ((pButton->m_event_mask & BUTTON_EVENT_CLICK) &&
                            pButton->m_press_time)
                            (m_callback)(m_pObj, row, col, BUTTON_EVENT_CLICK);
                    }   
                }
                else if (is_pressed &&
                         (pButton->m_event_mask & BUTTON_EVENT_LONG_CLICK) &&
                         pButton->m_press_time &&
                        (time > pButton->m_press_time + LONG_PRESS_TIME))
                {
                    pButton->m_press_time = 0;
                    (m_callback)(m_pObj, row, col, BUTTON_EVENT_LONG_CLICK);
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
                   (m_callback)(m_pObj, row, col, BUTTON_EVENT_CLICK);
                }
            #endif
            }
        }   // for each col
        
        //delay(200);
        digitalWrite(row_pins[row],0);
        
    }   // for each row
}


