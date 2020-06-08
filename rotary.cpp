#include "defines.h"

#if WITH_ROTARY     // empty compile if not
    
#include "rotary.h"
#include <myDebug.h>
    
#define DEBUG_ROTARY  0

int pollA[NUM_ROTARY] = {0,0,0,0};
int rotaryA[NUM_ROTARY] = {ROTARY_1A,ROTARY_2A,ROTARY_3A,ROTARY_4A};
int rotaryB[NUM_ROTARY] = {ROTARY_1B,ROTARY_2B,ROTARY_3B,ROTARY_4B};
int rotary_vslue[4] = {0,0,0,0};


void initRotary()
{
    for (int i=0; i<4; i++)
    {
        pinMode(rotaryA[i],INPUT_PULLDOWN);
        pinMode(rotaryB[i],INPUT_PULLDOWN);
        pollA[i] = digitalRead(rotaryA[i]);
            // init to current state
    }
}

int getRotaryValue(int i)
{
    return rotary_vslue[i];
}

bool pollRotary(int i)
{
    int aval = digitalRead(rotaryA[i]);
    if (pollA[i] == aval)
        return false;
        
    // only do something if A has changed
    
    pollA[i] = aval;
    
    // precision optimization
    // the indents on mine are one full cycle
    // so the number ALWAYS changes by 2 on a single click
    // if you want a detent to equal one inc/dec, include the
    // next line
    
    #if 0   // only check every other pulse
        if (aval)
    #endif
    
    {
        int bval = digitalRead(rotaryB[i]);
        if (aval == bval)
            rotary_vslue[i]++;
        else // if (aval && bval)
            rotary_vslue[i]--;
            
        #if DEBUG_ROTARY
            display(0,"rotary(%d) aval=%d bval=%d   value=%d",i,aval,bval,rotary_vslue[i]);
        #endif
        
        return true;
    }
    
    return false;
}




#endif // WITH_ROTARY     // empty compile if not
