#include <myDebug.h>
#include "expressionPedals.h"

#define HYSTERISIS   30
    // in raw 0..1023 units
#define SETTLE_TIME  50
    // time to settle into a direction


typedef struct
{
    int     pin;
    const   char *name;
    int     calib_min;
    int     calib_max;

    int     cable;
    int     channel;
    int     cc_num;
    
    int     raw_value;
    int     value;
    
    int     direction;
    unsigned settle_time; 
    
}   expressionPedal_t;

    
expressionPedal_t pedals[NUM_PEDALS] = {
    { 0, "Pedal1", 9, 1010, 0, 1, 0x0F, 0, 0, 0 ,0},
    { 0, "Pedal2", 9, 1010, 0, 1, 0x0E, 0, 0, 0 ,0},
    { 0, "Pedal3", 9, 1010, 0, 1, 0x0F, 0, 0, 0 ,0},
    { 0, "Pedal4", 9, 1010, 0, 1, 0x0E, 0, 0, 0 ,0},
};

int calibrate_pedal = -1;
elapsedMillis calibrate_time = 0;
    
    
void initPedals(int *pins)
{
    for (int i=0; i<NUM_PEDALS; i++)
    {
        pedals[i].pin = *pins++;
        pinMode(pedals[i].pin,INPUT_PULLDOWN);
    }
}
    


bool _pollPedal(int i)
{
    int raw_value = analogRead(pedals[i].pin);
    int pedal_raw = pedals[i].raw_value;
    int direction = pedals[i].direction;
    unsigned settle_time = pedals[i].settle_time;
    unsigned time = millis();
    
    // if not moving, and outside of hysterisis range, start moving
    
    if (!direction)
    {
        if (raw_value > pedal_raw + HYSTERISIS)
        {
            direction = 1;
            pedal_raw = raw_value;
            settle_time = time;
        }
        else if (raw_value < pedal_raw - HYSTERISIS)
        {
            direction = -1;
            pedal_raw = raw_value;
            settle_time = time;
        }
    }
    
    // if stopped moving, reset to default state
    
    else if (time >= settle_time + SETTLE_TIME)
    {
        settle_time = 0;
        direction = 0;
    }
    
    // otherwise, process the input
    
    else if (direction > 0 && raw_value > pedal_raw)
    {
        pedal_raw = raw_value;
        settle_time = time;
    }
    else if (direction < 0 && raw_value < pedal_raw)
    {
        pedal_raw = raw_value;
        settle_time = time;
    }
    
    // set the state back into the record
    
    pedals[i].raw_value = pedal_raw;
    pedals[i].direction = direction;
    pedals[i].settle_time = settle_time;

    // calculate the new value, and determine if it changed
    
    bool changed = false;
    int value = map(pedal_raw,pedals[i].calib_min,pedals[i].calib_max,0,127);
    if (value > 127) value=127;
    if (value < 0) value=0;
    if (value != pedals[i].value)
    {
        changed = true;
        pedals[i].value = value;
        display(0,"pedal(%d) changed to %d",i,value);
    }
    
    // return to caller
    return changed;

}


    
bool pedalTask()
{
    bool changed = false;
    for (int i=0; i<NUM_PEDALS; i++)
    {
        if (_pollPedal(i))
            changed = true;
    }
    return changed;
}
