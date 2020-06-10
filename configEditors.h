#ifndef __configEditors_h_
#define __configEditors_h_

#include "configOptions.h"

#define WIDTH           480
#define HEIGHT          320

#define BUTTON_MOVE_UP          12
#define BUTTON_MOVE_LEFT        16
#define BUTTON_MOVE_RIGHT       18
#define BUTTON_MOVE_DOWN        22
#define BUTTON_SELECT           17



// integer editor

extern void navInteger(configOption *caller, int num);
extern void drawInteger(configOption *caller);


#endif  // !__configEditors_h_