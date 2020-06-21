#include <myDebug.h>
#include "rotary.h"
#include "defines.h"
#include "expSystem.h"

    
#define DEBUG_ROTARY  0


typedef struct
{
    int pollA;      // the last value polled for the A part of the switch
    int pinA;       // the pin for A polling
    int pinB;       // the pin for B polling
    
    int min_range;  // the minimum value
    int max_range;  // the maximum value
    float inc_dec;    // the amount to inc or dec per signal
    
    float value;      // the current value
}   rotary_t;



rotary_t rotary[NUM_ROTARY] =
{
    {0, ROTARY_1A, ROTARY_1B, 0, 127, DEFAULT_INC_DEC, 0.00},
    {0, ROTARY_2A, ROTARY_2B, 0, 127, DEFAULT_INC_DEC, 0.00},
    {0, ROTARY_3A, ROTARY_3B, 0, 127, DEFAULT_INC_DEC, 0.00},
    {0, ROTARY_4A, ROTARY_4B, 0, 127, DEFAULT_INC_DEC, 0.00}
};



void setRotaryValue(int num, int value)
    // note that the stored value is a float
    // but the returned value is a truncated int
{
    if (value > rotary[num].max_range)
        value = rotary[num].max_range;
    if (value < rotary[num].min_range)
        value = rotary[num].min_range;
    rotary[num].value = value;
}


void setRotary(int num, int min_range, int max_range, float inc_dec)
{
    rotary[num].min_range = min_range;
    rotary[num].max_range = max_range;
    rotary[num].inc_dec = inc_dec;
}




void initRotary()
{
    for (int i=0; i<4; i++)
    {
        pinMode(rotary[i].pinA,INPUT_PULLDOWN);
        pinMode(rotary[i].pinB,INPUT_PULLDOWN);
        // init to current state
        rotary[i].pollA = digitalRead(rotary[i].pinA);
    }
}


int getRotaryValue(int i)
{
    return rotary[i].value;
}


bool _pollRotary(int i)
{
    int aval = digitalRead(rotary[i].pinA);
    if (rotary[i].pollA == aval)
        return false;
        
    // only do something if A has changed
    
    rotary[i].pollA = aval;
    
    int bval = digitalRead(rotary[i].pinB);
    if (aval == bval)
    {
        if (rotary[i].value + rotary[i].inc_dec > rotary[i].max_range)
            rotary[i].value = rotary[i].max_range;
        else
            rotary[i].value += rotary[i].inc_dec;
    }
    else
    {
        if (rotary[i].value - rotary[i].inc_dec < rotary[i].min_range)
            rotary[i].value = rotary[i].min_range;
        else
            rotary[i].value -= rotary[i].inc_dec;
    }
        
    #if DEBUG_ROTARY
        display(0,"rotary(%d) aval=%d bval=%d   value=%d",i,aval,bval,rotary_vslue[i]);
    #endif
    
    return true;
}


void pollRotary()
{
    for (int i=0; i<NUM_ROTARY; i++)
        if (_pollRotary(i))
            theSystem.rotaryEvent(i,rotary[i].value);
}


