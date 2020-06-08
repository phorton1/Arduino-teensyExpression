#ifndef _rotary_h_
#define _rotary_h_

#define NUM_ROTARY   4

void initRotary();
bool pollRotary(int i);
int getRotaryValue(int i);


#endif  // !_rotary_h_
