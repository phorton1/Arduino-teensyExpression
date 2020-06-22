#include <myDebug.h>
#include "myTouchScreen.h"
#include "defines.h"
#include <TouchScreen.h>


#define dbg_touch 0


myTouchScreen theTouchScreen;
TouchScreen libraryTouchScreen = TouchScreen(XP, YP, XM, YM);   // , 300);



void myTouchScreen::get(int *z, int *x, int *y)
{
    *z = 0;
    *x = 0;
    *y = 0;
    
    TSPoint p = libraryTouchScreen.getPoint();
    if (p.z > z_threshold &&
        p.x > minx && 
        p.x < maxx && 
        p.y > miny && 
        p.y < maxy) 
    {
        *z = p.z;
        
        float myx = ((float)(p.x - minx)) / ((float)(maxx - minx));
        float myy = ((float)(p.y - miny)) / ((float)(maxy - miny));
        *x = ((1-myx) * 480);
        *y = ((1-myy) * 320);

        display(dbg_touch,"z=%d \tx=%d \ty=%d \t\t%d\t%d",p.z,p.x,p.y,*x,*y);
    }
}

