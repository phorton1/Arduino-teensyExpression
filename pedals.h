#ifndef _pedals_h_
#define _pedals_h_


#define NUM_PEDALS  4


extern void initPedals();
extern bool pedalTask();
    // returns true if any pedals have changed value

extern int getPedalValue(int i);
    // return the value of the pedal
    


// extern void startCalibrate(int pedal)
    
    
#endif      // !_pedals_h_  