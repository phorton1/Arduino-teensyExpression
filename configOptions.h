#ifndef __configOptions_h_
#define __configOptions_h_

#include "configSystem.h"

#define OPTION_TYPE_FACTORY_RESET   0x8000
#define OPTION_TYPE_NEEDS_REBOOT    0x4000
#define OPTION_TYPE_BRIGHTNESS      0x0001
#define OPTION_TYPE_CONFIG_NUM      0x0002


class configOption
{
    public:

        configOption();
        configOption(configOption *parent, const char *title, int type=0, int pref_num=-1);

    protected:

        friend class configSystem;
        void init();    // recursively called at configSystem::begin(!warm)
        static bool reboot_needed();

        int   getNum()                { return option_num; }
        bool  needsValueDisplay()     { return display_value != getValue(); }
        void  clearDisplayValue()     { display_value = getValue(); }
        const char *getTitle()        { return title; }
        bool  hasValue()              { return m_pref_num != -1; }

        void  setValue(int i);        // enforces min/max
        void  incValue(int inc_dec);  // wraps
        const char *getValueString();

        virtual bool  isEnabled()             { return 1; }

    protected:

        friend class configSystem;

        const char *title;
        int         type;
        int         option_num;
        int         m_pref_num;
        int         num_children;

        configOption *pParent;
        configOption *pNextOption;
        configOption *pPrevOption;
        configOption *pFirstChild;
        configOption *pLastChild;

        int  display_value;
        int  selected;
        int  display_selected;
        int  display_enabled;

    private:

        int getValue();
        void init_cold(configOption *parent, const char *tit, int typ, int pref_num);
            // ctor initialization
};



#endif  // !__configOptions_h_