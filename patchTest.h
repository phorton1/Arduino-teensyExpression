#ifndef __patchTest_h__
#define __patchTest_h__

#include "expSystem.h"


class patchTest : public expWindow
{
    public:

        patchTest() {}

    private:

        virtual const char *name()          { return "Demonstration"; }
        virtual const char *short_name()    { return "Demo"; }

        virtual void begin(bool warm);
        virtual void updateUI();
};


#endif // !__patchTest_h__