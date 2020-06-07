#ifndef _expressionPedals_h_
#define _expressionPedals_h_


#define NUM_PEDALS  4


extern void initPedals(int *pins);
extern bool pedalTask();
    // returns true if any pedals have changed value

extern int getPedalValue(int i);
    // return the value of the pedal
    


// extern void startCalibrate(int pedal)
    
    
#endif      // !_expressionPedals_h_  