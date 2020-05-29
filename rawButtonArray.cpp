#include "myDebug.h"
#include "rawButtonArray.h"
#include "myLeds.h"


static rawButtonArray *s_pThis = 0;

#ifdef TEENSY_36
    #define ROW_PIN_BASE   24
    #define COL_PIN_BASE   29
#endif


    
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


rawButtonArray::rawButtonArray(void *pObj, handleButtonEventFxn *callback)
{
    s_pThis = this;
    m_pObj = pObj;
    m_callback = callback;

    #ifdef TEENSY_36
        for (int row=0; row<NUM_BUTTON_ROWS; row++)
        {
            pinMode(ROW_PIN_BASE+row,OUTPUT);
            digitalWrite(ROW_PIN_BASE+row,0);
        }
        for (int col=0; col<NUM_BUTTON_COLS; col++)
            pinMode(COL_PIN_BASE+col,INPUT_PULLDOWN);
    #else
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
    #endif
    
    display(0,"rawButtonArray::begin()",0);
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
    
    // scan the button array for new raw data
    // this loop is done in reverse order because I hooked
    // the pins for rows4..0 to the 595 output pins 0..4
    // as the wires most naturally fit coming out of the pedal
    

    #define TIME_BUTTON_SCAN  0

    #if TIME_BUTTON_SCAN
        elapsedMillis scan_time;
        for (int j=0; j<1000; j++)
        {
    #endif

    #ifndef TEENSY_36
        unsigned data[8];
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
    #endif
    
    
    
    unsigned time = millis();
    for (int row=0; row<NUM_BUTTON_ROWS; row++)
    {
        #ifdef TEENSY_36
            digitalWrite(ROW_PIN_BASE+row,1);
        #endif
        
        for (int col=0; col<NUM_BUTTON_COLS; col++)
        {
            rawButton *pButton = &m_buttons[row][col];
            
            #if DO_DEBOUNCE
                if (time > pButton->m_debounce_time)
            #endif
            {
                #ifdef TEENSY_36
                    bool is_pressed = digitalRead(COL_PIN_BASE+col);
                #else
                    bool is_pressed = data[row] & (1 << (NUM_BUTTON_COLS-col-1));
                #endif
                
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
                        if (pButton->m_event_mask)
                        {
                            setLED(row,col,WHITE);
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
        
        #ifdef TEENSY_36
            digitalWrite(ROW_PIN_BASE+row,0);
        #endif
        
    }   // for each row
    
    #if TIME_BUTTON_SCAN
        }
        display(0,"1000 button scans took: %d ms",(int)scan_time);
    #endif
    
}


