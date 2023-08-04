#include "myDebug.h"

#define PIN_BACKWARD A2
#define PIN_FORWARD  A3
#define PIN_VALUE    A4

#define CALIB_MAX   575
#define CALIB_MIN   100
#define CALIB_RANGE (CALIB_MAX - CALIB_MIN)


void setup() 
{
    Serial.begin(115200);

    pinMode(PIN_FORWARD,OUTPUT);
    pinMode(PIN_BACKWARD,OUTPUT);
    pinMode(PIN_VALUE,INPUT);
    
    digitalWrite(PIN_BACKWARD,1);
    digitalWrite(PIN_FORWARD,1);
    
    delay(800);
}


int getValue()
    // ALL measurements used in program are in CALIBRATED UNITS 0..127
{
    int raw_value = analogRead(PIN_VALUE);
    int value = map(raw_value,CALIB_MIN,CALIB_MAX,0,127);
    if (value > 127) value=127;
    if (value < 0) value=0;
    return value;
}



void loop() 
{

#if 0

    // display(0,"value=%d",analogRead(PIN_VALUE));
    display(0,"getValue()=%d",getValue());
    delay(300);

#elif 0       // use 'a' and 'b' keys to move +/i delta upto limits

    int delta = 10;      // 25 steps 0..127
    int window = 1;     // plus or minus this
    
    static int pos = 63;
    static int running = 0;
    int c = Serial.read();
    int v = getValue();
    
    if (c == 'a')
    {
        pos -= delta;
        if (pos < 0) pos = 0;
        display(0,"back(%d)",pos);
        running = 1;
    }
    else if (c == 'b')
    {
        pos += delta;
        if (pos > 127) pos = 127;
        display(0,"forward(%d)",pos);
        running = 1;
    }
    
    if (running)
    {
        if (v > pos+window)
        {
            analogWrite(PIN_BACKWARD,255);
            digitalWrite(PIN_FORWARD,0);
        }
        else if (v < pos+window)
        {
            analogWrite(PIN_FORWARD,255);
            digitalWrite(PIN_BACKWARD,0);
        }
        else
        {
            digitalWrite(PIN_FORWARD,1);
            digitalWrite(PIN_BACKWARD,1);
            running = 0;
        }
    }
    

#elif 0     // spring back to 63 +/- 1

    int v = getValue();
    if (v < 62)
    {
        analogWrite(PIN_FORWARD,255);
        digitalWrite(PIN_BACKWARD,0);
    }
    else if (v > 64)
    {
        analogWrite(PIN_BACKWARD,255);
        digitalWrite(PIN_FORWARD,0);
    }
    else
    {
        digitalWrite(PIN_FORWARD,1);
        digitalWrite(PIN_BACKWARD,1);
    }


#else       // programmed detents (upto 10 so)

    #define DETENT_STRENGTH   255
    #define NUM_DETENTS 1
    #define DETENT_WINDOW 1
    #define FIRST_DETENT_VALUE  127/(NUM_DETENTS + 1)
    
    // #define DETENT_MAGNET ((FIRST_DETENT_VALUE - DETENT_WINDOW - 1) / 2)
    #define DETENT_MAGNET  5
    
    int v = getValue();
    for (int detnum=0; detnum<NUM_DETENTS; detnum++)
    {
        int detent = (detnum+1) * FIRST_DETENT_VALUE;
        if (v > detent-DETENT_WINDOW-DETENT_MAGNET &&
            v < detent-DETENT_WINDOW)
        {
            analogWrite(PIN_FORWARD,DETENT_STRENGTH);
            digitalWrite(PIN_BACKWARD,0);
            return;
        }
        else if (v > detent + DETENT_WINDOW &&
                 v < detent + DETENT_WINDOW + DETENT_MAGNET)
        {
            digitalWrite(PIN_FORWARD,0);
            analogWrite(PIN_BACKWARD,DETENT_STRENGTH);
            return;        
        }
    }
    
    digitalWrite(PIN_FORWARD,1);
    digitalWrite(PIN_BACKWARD,1);

#endif

}
