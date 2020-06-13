#ifndef __test_config_h__
#define __test_config_h__

#include "expSystem.h"


class testConfig : public expConfig
{
    public:
        
        testConfig() {}
        
    private:
        
        virtual const char *name() { return "My Midi Controller"; }
        virtual const char *short_name()    { return "Demo Config"; }

        virtual void begin();
        virtual void updateUI();
};


#endif // !__test_config_h__