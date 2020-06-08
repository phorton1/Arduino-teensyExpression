#ifndef _pedals_h_
#define _pedals_h_


#define NUM_PEDALS  4


extern void initPedals();
extern bool pollPedal(int i);
    // returns true if any pedal has changed value
extern int getPedalValue(int i);
    // return the value of the pedal
    


// extern void startCalibrate(int pedal)
    
    
#endif      // !_pedals_h_  