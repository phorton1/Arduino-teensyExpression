#ifndef __configOptions_h_
#define __configOptions_h_

#include "configSystem.h"

#define OPTION_TYPE_FACTORY_RESET   0x8000
    // used to differentiate return from modal callback in onEndModal in configSystem.cpp
#define OPTION_TYPE_NEEDS_REBOOT    0x4000
    // sets static reboot_needed bit if setValue or incValue called on option,
    // which is used by configSystem to reboot on long click THE_SYSTEM_BUTTON
#define OPTION_TYPE_CONFIG_NUM      0x0002
    // used to flag special behavior in configSystem.cpp to set the patch
    // quick buttons when BUTTON_SELECT is pressed.


typedef void (*setterFxn)(int i);
    // a pointer to a function will be called
    //    pref options - on setValue()
    //         i.e. for Brightness, setLEDBrightness(value) is called,
    //    non-pref options with no children
    //         pointer to a dialog box factory method that will be called
    //         with the option number within the menu


class configOption
    // items with pref_nums >=0 are tightly bound to prefs,
    // calling getPref() and setPref() for value manipulation,
    // assuming that if the max is > 254 it's a uint16.
{
    public:

        configOption();
        configOption(
            configOption *parent,
            const char *title,
            int type=0,
            int pref_num=-1,            // items with pref_nums >=0 are tightly bound to prefs
            setterFxn setter=0);         // will be called for pref options with the changed value

    protected:

        friend class configSystem;
        void init();    // recursively called at configSystem::begin(!warm)
        static bool reboot_needed();

        int   getNum()                { return option_num; }
        bool  needsValueDisplay()     { return display_value != getValue(); }
        void  clearDisplayValue()     { display_value = getValue(); }
        const char *getTitle()        { return title; }
        bool  hasValue()              { return m_pref_num >= 0 ? 1 : 0; }

        void  setValue(int i);        // enforces min/max
        void  incValue(int inc_dec);  // wraps
        const char *getValueString();

        virtual bool  isEnabled()             { return 1; }

        int           num_children;
        configOption *pParent;
        configOption *pNextOption;
        configOption *pPrevOption;
        configOption *pFirstChild;
        configOption *pLastChild;

        setterFxn   m_setter_fxn;

    private:

        // friend class configSystem;

        const char *title;
        int         type;
        int         option_num;
        int         m_pref_num;


        int  display_value;
        int  selected;
        int  display_selected;
        int  display_enabled;


    private:

        int getValue();
        void init_cold(configOption *parent, const char *tit, int typ, int pref_num, setterFxn setter);
            // ctor initialization
};


#define ENABLED_CONFIG(class_name,condition)  \
    class class_name : public configOption    \
    {                                         \
    public:                                   \
                                              \
       class_name(                            \
            configOption *parent,             \
            const char *title,                \
            int type=0,                       \
            int pref_num=-1,                  \
            setterFxn setter=0) :             \
                configOption(                 \
                    parent,                   \
                    title,                    \
                    type,                     \
                    pref_num,                 \
                    setter) {}                \
                                              \
        virtual bool  isEnabled()             \
            { return (condition); }           \
    }


#endif  // !__configOptions_h_