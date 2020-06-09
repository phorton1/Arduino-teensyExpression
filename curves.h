#ifndef __curves_h__
#define __curves_h__

#define NUM_CURVES 3
#define NUM_POINTS 4

#define CURVE_COMMAND_SELECT_NEXT_CURVE     1
#define CURVE_COMMAND_SELECT_NEXT_POINT     2
#define CURVE_COMMAND_MOVE_UP               3
#define CURVE_COMMAND_MOVE_DOWN             4
#define CURVE_COMMAND_MOVE_LEFT             5
#define CURVE_COMMAND_MOVE_RIGHT            6
#define CURVE_COMMAND_INC_PARAM             7

extern bool curve_command_can(int i);
    // can do point related commands?

extern void curve_command(int command);

extern void draw_curve(bool force);
    // everything below y=36 is presumed available


#endif  // !__curves_h__

   