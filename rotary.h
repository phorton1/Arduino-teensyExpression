#ifndef _rotary_h_
#define _rotary_h_

#define NUM_ROTARY   4


#define INCS_PER_REV        40.00
#define DEFAULT_INC_DEC     (128.00 / (0.80 * INCS_PER_REV))
    // 0..127 in 0.8 revolutions
    
    
void initRotary();
void pollRotary();

int getRotaryValue(int i);
void setRotaryValue(int num, int value);
void setRotary(int num, int min_range=0, int max_range=127, float inc_dec=DEFAULT_INC_DEC);
    // default does 0..127 in about 1 turn (40 incs per rev)


#endif  // !_rotary_h_
