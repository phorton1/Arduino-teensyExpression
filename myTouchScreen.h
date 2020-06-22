#ifndef _myTouchScreen_h_
#define _myTouchScreen_h_

// Resistive touch screen on Cheap Ardino 3.5" 320x480 TFT's
// need an object, calibration routine, etc

class myTouchScreen
{
    public:
        
        myTouchScreen()
        {
            minx = 240;
            maxx = 920;
            miny = 90;
            maxy = 860;
            z_threshold = 50;
        }

        void get(int *z, int *x, int *y);
            // result is only valid if z > 0
            // it will be larger than z_threshold
            
            
    private:
        
        int minx;
        int maxx;
        int miny;
        int maxy;
        int z_threshold;
};


extern myTouchScreen theTouchScreen;


#endif  // !_myTouchScreen_h_