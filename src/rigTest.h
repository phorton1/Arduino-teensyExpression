#ifndef __rigTest_h__
#define __rigTest_h__

#include "expSystem.h"


class rigTest : public expWindow
{
    public:

        rigTest() {}

    private:

        virtual const char *name()          { return "Demonstration"; }
        virtual const char *short_name()    { return "Demo"; }

        virtual void begin(bool warm);
        virtual void updateUI();
};


#endif // !__rigTest_h__