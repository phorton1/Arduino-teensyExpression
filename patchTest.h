#ifndef __patchTest_h__
#define __patchTest_h__

#include "expSystem.h"


class patchTest : public expWindow
{
    public:
        
        patchTest() {}
        
    private:
        
        virtual const char *name() { return "My Midi Controller"; }
        virtual const char *short_name()    { return "Demo Config"; }

        virtual void begin();
        virtual void updateUI();
};


#endif // !__patchTest_h__