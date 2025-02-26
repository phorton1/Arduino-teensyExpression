#include <myDebug.h>
#include "rotary.h"
#include "defines.h"
#include "expSystem.h"


#define DEBUG_ROTARY  0

// these default values are to match the rotary controls to
// the rPi looper input, output, thru, and mix volumes
// the loop volume is controlled by a foot pedal

#define DEFAULT_INPUT_VOL    94     // codec input
#define DEFAULT_THRU_VOL     63     // thru
#define DEFAULT_MIX_VOL      63     // loop
#define DEFAULT_OUT_VOL      127    // output


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
    {0, ROTARY_1A, ROTARY_1B, 0, 127, DEFAULT_INC_DEC, DEFAULT_INPUT_VOL},
    {0, ROTARY_2B, ROTARY_2A, 0, 127, DEFAULT_INC_DEC, DEFAULT_OUT_VOL },
    {0, ROTARY_3A, ROTARY_3B, 0, 127, DEFAULT_INC_DEC, DEFAULT_THRU_VOL  },
    {0, ROTARY_4B, ROTARY_4A, 0, 127, DEFAULT_INC_DEC, DEFAULT_MIX_VOL  }
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
        pinMode(rotary[i].pinA,INPUT);  // _PULLDOWN);
        pinMode(rotary[i].pinB,INPUT);  // _PULLDOWN);
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
        int show_val = rotary[i].value;
        display(0,"rotary(%d) aval=%d bval=%d   value=%d",i,aval,bval,show_val);
    #endif

    return true;
}


void pollRotary()
{
    for (int i=0; i<NUM_ROTARY; i++)
        if (_pollRotary(i))
        {
            theSystem.rotaryEvent(i,rotary[i].value);
        }
}

