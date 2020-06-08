#ifndef __test_config_h__
#define __test_config_h__

#include "expSystem.h"
#include "myLeds.h"


class testConfig : public expConfig
{
    public:
        
        testConfig(expSystem *pSystem) :
            expConfig(pSystem)
        {}
        
        virtual const char *name() { return "Teensy Expresssion"; }

        
    virtual void begin()
    {
        expConfig::begin();

        // rawButtonArray *ba = m_pSystem->getRawButtonArray();
        
        // ba->setButtonEventMask(0,0,BUTTON_EVENT_CLICK);
        // ba->setButtonEventMask(0,1,BUTTON_EVENT_CLICK);
        // ba->setButtonEventMask(0,3,BUTTON_EVENT_CLICK);
        // ba->setButtonEventMask(0,4,BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK);
        
        for (int row=0; row<NUM_BUTTON_ROWS; row++)
        {
            for (int col=0; col<NUM_BUTTON_COLS; col++)
            {
                float c = col;
                float r = row;
                
                float red = (c/4) * 255.0;
                float blue = ((4-c)/4) * 255.0;
                float green = (r/4) * 255.0;
                
                unsigned rr = red;
                unsigned gg = green;
                unsigned bb = blue;
                
                setLED(row,col,(rr << 16) + (gg << 8) + bb);
            }
        }
        
        showLEDs();
    }

        
    private:
        
        
};


#endif // !__test_config_h__