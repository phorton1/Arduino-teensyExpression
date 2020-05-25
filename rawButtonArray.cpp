#include "myDebug.h"
#include "rawButtonArray.h"

// The notion of "rows" and "columns", and the origin of the grid,
// depends on the connections from the grid of switches to the
// 74hc595 and and 74hc165.

// This set of defines is for the buttons starting at the bottom right,
// with outputs on columns and inputs on rows.

//  pins in H file for visibility
//
// #define BUTTON_OUT_CLOCK_PIN        3
// #define BUTTON_OUT_LATCH_PIN        4
// #define BUTTON_OUT_DATA_PIN         6
// #define BUTTON_IN_LOAD_PIN          7
// #define BUTTON_IN_CLOCK_ENABLE_PIN  15
// #define BUTTON_IN_CLOCK_PIN         16
// #define BUTTON_IN_DATA_PIN          17


static rawButtonArray *s_pThis = 0;

    
#define LONG_PRESS_TIME    1200
#define DOUBLE_CLICK_TIME  600
    // You want this as small as possible if DISTINCT_DOUBLE_CLICK
    // is defined, as this introduces a "lag" on regular CLICK events.
    
// Schemes ....

#define DISTINCT_DOUBLE_CLICK 1
    // if defined, we delay the SINGLE CLICK until we are sure
    // we are out of the zone

#if USE_BUTTON_SCAN_TIMER
    #define BUTTON_TIMER_INTERVAL      10000   // 10000 us = 10 ms == 100 times per second
        // larger than DEBOUNCE_MILLIS ms means the debounce code is meaningless,
        // and it's probably not needed anyways with any reasonable values
#endif

#define DO_DEBOUNCE        0
    // does not seem to be necessary.
    // in timer version, a period of time is implicit, so this should be turned off
    // in loop() version, maybe we are getting by because of display() calls
    // so *this* might still be needed.

#if DO_DEBOUNCE
    #define DEBOUNCE_MILLIS    5
#endif


rawButtonArray::rawButtonArray(void *pObj, handleButtonEventFxn *callback)
{
    s_pThis = this;
    m_pObj = pObj;
    m_callback = callback;
}
    
    
void rawButtonArray::begin()
{
    pinMode(BUTTON_OUT_LATCH_PIN,OUTPUT);
    pinMode(BUTTON_OUT_CLOCK_PIN,OUTPUT);
    pinMode(BUTTON_OUT_DATA_PIN,OUTPUT);
    pinMode(BUTTON_IN_LOAD_PIN,OUTPUT);
    pinMode(BUTTON_IN_CLOCK_ENABLE_PIN,OUTPUT);
    pinMode(BUTTON_IN_CLOCK_PIN,OUTPUT);
    pinMode(BUTTON_IN_DATA_PIN,INPUT_PULLUP);

    digitalWrite(BUTTON_OUT_CLOCK_PIN, HIGH);
    digitalWrite(BUTTON_OUT_LATCH_PIN, HIGH);
    digitalWrite(BUTTON_IN_LOAD_PIN, HIGH);
    digitalWrite(BUTTON_IN_CLOCK_ENABLE_PIN, HIGH);
    digitalWrite(BUTTON_IN_CLOCK_PIN, HIGH);
    
    display(0,"rawButtonArray::begin()",0);
#if DO_DEBOUNCE
    display(0,"    WITH DEBOUNCE!",0);
#else
    display(0,"    NO DEBOUNCE!",0);
#endif
#if USE_BUTTON_SCAN_TIMER
    display(0,"    USING TIMER",0);
    m_timer.begin(timer_handler,BUTTON_TIMER_INTERVAL);
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


#if USE_BUTTON_SCAN_TIMER
    // static
    void rawButtonArray::timer_handler()
    {
        s_pThis->task();
    }
#endif


void rawButtonArray::task()
{
    if (!m_callback)
    {
        my_error("NO CALLBACK ROUTINE in rawButtonArray",0);
        return;
    }
    
    // scan the button array for new raw data
    // this loop is done in reverse order because I hooked
    // the pins for rows4..0 to the 595 output pins 0..4
    // as the wires most naturally fit coming out of the pedal
    
    unsigned data[8];

    #define TIME_BUTTON_SCAN  0

    #if TIME_BUTTON_SCAN
        elapsedMillis scan_time;
        for (int j=0; j<1000; j++)
        {
    #endif
    
    for (int row=NUM_BUTTON_ROWS-1; row>=0; row--)
    {
        // send out one bit at a time, starting with a one,
        // then subsequntly zeros (not using shiftOut());
        // This is faster, plus there was a bug with shiftOut...
        // it just did not work as advertised with MSB, LSB, and all masks.

        digitalWrite(BUTTON_OUT_LATCH_PIN, LOW);                 // disable the latch
        digitalWrite(BUTTON_OUT_DATA_PIN, row==0 ? HIGH : LOW);
        digitalWrite(BUTTON_OUT_CLOCK_PIN, LOW);                 // pulse the clock pin
        delayMicroseconds(5);
        digitalWrite(BUTTON_OUT_CLOCK_PIN, HIGH);                
        digitalWrite(BUTTON_OUT_LATCH_PIN, HIGH);                // latch out the data
        
        #ifdef OBSOLETE_BUTTON_CODE   // old code using shiftOut
            digitalWrite(BUTTON_OUT_LATCH_PIN, LOW);
            shiftOut(BUTTON_OUT_DATA_PIN, BUTTON_OUT_CLOCK_PIN, LSBFIRST, mask);
            digitalWrite(BUTTON_OUT_LATCH_PIN, HIGH);
        #endif
        
        digitalWrite(BUTTON_IN_LOAD_PIN, LOW);
        delayMicroseconds(5);
        digitalWrite(BUTTON_IN_LOAD_PIN, HIGH);
        delayMicroseconds(5);
        
        digitalWrite(BUTTON_IN_CLOCK_PIN, HIGH);
        digitalWrite(BUTTON_IN_CLOCK_ENABLE_PIN, LOW);
        data[row] = shiftIn(BUTTON_IN_DATA_PIN, BUTTON_IN_CLOCK_PIN, LSBFIRST);
        digitalWrite(BUTTON_IN_CLOCK_ENABLE_PIN, HIGH);
    }
    
    // I do a separate loop through rows to keep the scan and processing
    // code cleaner separate.  Each button has it's own debounce time.
    
    
    unsigned time = millis();
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            rawButton *pButton = &m_buttons[row][col];
            
            #if DO_DEBOUNCE
                if (time > pButton->m_debounce_time)
            #endif
            {
                bool is_pressed = data[row] & (1 << (NUM_BUTTON_COLS-col-1));
                
                // if state changed, save new bit to data
                // and process the button
                
                if (pButton->m_pressed != is_pressed)
                {
                    display(0,"BUTTON(%d,%d) %s",row,col,is_pressed?"DOWN":"UP");
                    pButton->m_pressed = is_pressed;
                    #if DO_DEBOUNCE
                        pButton->m_debounce_time = time + DEBOUNCE_MILLIS;
                    #endif

                    if (is_pressed)     // button pressed
                    {
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
        }
    }
    
    #if TIME_BUTTON_SCAN
        }
        display(0,"1000 button scans took: %d ms",(int)scan_time);
    #endif
    
}


