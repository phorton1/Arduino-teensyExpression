
#include "curves.h"
#include "myDebug.h"
#include "expSystem.h"


#define X_OFFSET 160
#define Y_OFFSET 45
#define CHART_MAX  255


typedef struct
{
    const char *name;
    int vert_only;
    int x;
    int y;
    int param;
}   curve_point_t;


typedef struct
{
    const char *name;
    int num_points;
    curve_point_t points[NUM_POINTS];
} curve_data_t;


curve_data_t curves[NUM_CURVES] = {
    { "linear",     2, {
        {"min",     1,  0,   0,   0},
        {"max",     1,  127, 127, 1},
        {"",        0,  0,   0,   0},
        {"",        0,  0,   0,   0}}},
    { "asympt",     3, {
        {"min",     1,  0,   0,   0},
        {"mid",     0,  64,  64,  0},
        {"max",     1,  127, 127, 0},
        {"",        0,  0,   0,   0}}},
    { "scurve",     4, {
        {"min",     1,  0,   0,   0},
        {"left",    0,  43,  43,  0},
        {"right",   0,  86,   86,   0},
        {"max",     1,  127, 127, 0}}}
};
        
    

int redraw_all = 0;
int redraw_curve = 0;

int curve_num = 0;
int cur_point = 1;
curve_data_t cur_curve =
    { "linear",     2, {
        {"min",     1,  0,   0,   0},
        {"max",     1,  127, 127, 1},
        {"",        0,  0,   0,   0},
        {"",        0,  0,   0,   0}}};


bool curve_command_can(int i)
{
    if (i == CURVE_COMMAND_SELECT_NEXT_CURVE)
        return true;
    if (i == CURVE_COMMAND_SELECT_NEXT_POINT)
        return curve_num != -1;
    if ((curve_num != -1) && (cur_point != -1))
    {
        // min & max can only move up and down
        
        curve_point_t *cp = &cur_curve.points[cur_point];
        if (cp->vert_only && (
            (i == CURVE_COMMAND_MOVE_LEFT) || 
            (i == CURVE_COMMAND_MOVE_RIGHT)))
            return false;
        
        // points can never move to the right of, or above points to the right
        
        if (cur_point < cur_curve.num_points - 1)
        {
            if ((i == CURVE_COMMAND_MOVE_RIGHT) &&
                (cp->x + 1 >= cur_curve.points[cur_point+1].x))
                return false;
            if ((i == CURVE_COMMAND_MOVE_UP) &&
                (cp->y + 1 >= cur_curve.points[cur_point+1].y))
                return false;
       }
       
       // or to the left of or below points to their left

        if (cur_point)
        {
            if ((i == CURVE_COMMAND_MOVE_LEFT) &&
                (cp->x - 1 <= cur_curve.points[cur_point-1].x))
                return false;
            if ((i == CURVE_COMMAND_MOVE_DOWN) &&
                (cp->y - 1 <= cur_curve.points[cur_point-1].y))
                return false;
       }

       // they cant move out of the box
       
       if (i == CURVE_COMMAND_MOVE_LEFT && cp->x <= 0)
            return false;
       if (i == CURVE_COMMAND_MOVE_RIGHT && cp->x >= 127)
            return false;
       if (i == CURVE_COMMAND_MOVE_DOWN && cp->y <= 0)
            return false;
       if (i == CURVE_COMMAND_MOVE_UP && cp->y >= 127)
            return false;
       
       return true;
    }
    return false;
}


void curve_command(int command)
{
    display(0,"curve_command(%d)",command);
    if (command == CURVE_COMMAND_SELECT_NEXT_CURVE)
    {
        curve_num++;
        if (curve_num >= NUM_CURVES)
            curve_num = 0;
                
        memcpy(&cur_curve,&curves[curve_num],sizeof(curve_data_t));
        cur_point = cur_curve.num_points-1;
    }
    if (command == CURVE_COMMAND_SELECT_NEXT_POINT)
    {
        cur_point++;
        if (cur_point >= cur_curve.num_points)
            cur_point = 0;
    }

    if (command == CURVE_COMMAND_MOVE_UP)
    {
         int y = cur_curve.points[cur_point].y + 1;
         if (y > 127) y = 127;
         cur_curve.points[cur_point].y = y;
         redraw_curve = 1;
    }
    if (command == CURVE_COMMAND_MOVE_DOWN)
    {
         int y = cur_curve.points[cur_point].y - 1;
         if (y < 0) y = 0;
         cur_curve.points[cur_point].y = y;
         redraw_curve = 1;
    }
    if (command == CURVE_COMMAND_MOVE_RIGHT)
    {
         int x = cur_curve.points[cur_point].x + 1;
         if (x > 127) x = 127;
         cur_curve.points[cur_point].x = x;
         redraw_curve = 1;
    }
    if (command == CURVE_COMMAND_MOVE_LEFT)
    {
         int x = cur_curve.points[cur_point].x - 1;
         if (x < 0) x = 0;
         cur_curve.points[cur_point].x = x;
         redraw_curve = 1;
    }
    if (command == CURVE_COMMAND_INC_PARAM)
    {
         int param = cur_curve.points[cur_point].param + 1;
         if (param > 127) param = 0;
         cur_curve.points[cur_point].param = param;
         redraw_curve = 1;
    }
    
    draw_curve(false);
    
}




void draw_curve(bool force)
{
    mylcd.setFont(Arial_12);
    mylcd.Set_Draw_color(TFT_WHITE);
    mylcd.Set_Text_colour(TFT_WHITE);
    
    if (force)
    {
        display(0,"redrawing curve %d %d",curve_num,cur_point);
        mylcd.Fill_Rect(0,40,480,280,0);
        mylcd.Set_Text_Cursor(0,50);
        mylcd.print("curve: ");
        mylcd.println(curve_num == -1 ? "none" : cur_curve.name);
        mylcd.print("point: ");
        mylcd.println(cur_point == -1 ? "none" : cur_curve.points[cur_point].name);

        if (cur_point != -1)
        {
            mylcd.Set_Text_Cursor(0,159);
            mylcd.print("x: ");
            mylcd.println(cur_curve.points[cur_point].x,DEC);
            mylcd.print("y: ");
            mylcd.println(cur_curve.points[cur_point].y,DEC);
            mylcd.print("p: ");
            mylcd.println(cur_curve.points[cur_point].param,DEC);
        }
        
        // grid
        
        mylcd.Draw_Line(X_OFFSET,Y_OFFSET,X_OFFSET,Y_OFFSET+CHART_MAX);
        mylcd.Draw_Line(X_OFFSET,Y_OFFSET+CHART_MAX,X_OFFSET+CHART_MAX,Y_OFFSET+CHART_MAX);
        mylcd.Draw_Line(X_OFFSET,Y_OFFSET+CHART_MAX,X_OFFSET+CHART_MAX,Y_OFFSET);
    
        // lines between points
        
        mylcd.Set_Draw_color(TFT_YELLOW);
        for (int i=0; i<cur_curve.num_points-1; i++)
        {
            curve_point_t *p0 = &cur_curve.points[i];
            curve_point_t *p1 = &cur_curve.points[i+1];
            mylcd.Draw_Line(
                X_OFFSET + 2*p0->x ,
                Y_OFFSET + CHART_MAX - 2*p0->y,
                X_OFFSET + 2*p1->x ,
                Y_OFFSET + CHART_MAX - 2*p1->y);
        }
    }
    
    //  points

    if (force || redraw_curve)
    {        
        for (int i=0; i<cur_curve.num_points; i++)
        {
            curve_point_t *p0 = &cur_curve.points[i];
            
            if (i == cur_point)
                mylcd.Draw_Circle(
                    X_OFFSET + 2*p0->x,
                    Y_OFFSET + (CHART_MAX-2*p0->y),
                    7);
            else
                mylcd.Fill_Circle(
                    X_OFFSET + 2*p0->x,
                    Y_OFFSET + (CHART_MAX-2*p0->y),
                    5);
        }
    }

    if (cur_curve.num_points == 3)
    {
        // try to get to the intersection of two lines that are paralell to thes and 10 units apart
        
        
        
    }
    
    redraw_curve = false;

}

