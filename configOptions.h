#ifndef __configOptions_h_
#define __configOptions_h_

#include "systemConfig.h"


#define OPTION_TYPE_MENU         0x8000     // item represents a submenu
#define OPTION_TYPE_TERMINAL     0x4000     // item is a terminal node (represents a window-function)
#define OPTION_TYPE_IMMEDIATE    0x2000     // value is changed in place (i.e. enum) with SELECT
#define OPTION_TYPE_VALUE        0x1000     // item has a value 
#define OPTION_TYPE_BRIGHTNESS   0x0800
#define OPTION_TYPE_CONFIG_NUM   0x0400



// externs for interface to terminal node editors
// used in configEditors.cpp

class configOption;
        
extern bool terminal_mode_draw_needed;
extern bool in_terminal_mode;
extern configOption *display_menu;
extern configOption *display_option;        

typedef void navButtonHandler(configOption *caller, int num);
typedef void drawHandler(configOption *caller);





class configOption
{
    public:
    
        configOption();
        configOption(configOption *parent, const char *title, int type);
        configOption(configOption *parent, const char *title, int type, int min, int max);

    // protected:
        // friend class systemConfig;
    
        void init(systemConfig *sysConfig);
            // non-virtual entry point for root node
        virtual void init();
            // called recursively during begin() on subclasses that
            // are linked to other objects to initialize the value, etc
        
        virtual int   getValue()              { return value; }
        virtual int   getOrigValue()          { return orig_value; }
        virtual bool  needsValueDisplay()     { return display_value != value; }
        virtual void  clearDisplayValue()     { display_value = value; }
        virtual const char *getValueString()  { return ""; }
        virtual const char *getTitle()        { return title; }

        virtual void  setValue(int i);      // enforces min/max
        virtual void incValue(int inc_dec); // wraps
        
         
    protected:
        friend class systemConfig;
        
        const char *title;
        int         type;
        
        configOption *pParent;
        configOption *pNextOption;
        configOption *pPrevOption;
        configOption *pFirstChild;
        configOption *pLastChild;

        systemConfig *m_pSysConfig;
        
        int  value;
        int  orig_value;
        int  display_value;
        int  min_value;
        int  max_value;
             
        int  selected;
        int  display_selected;
    
        // terminal mode support
        
        virtual bool beginTerminalMode()  { return false; }
            // implemented classes should return true
            // and must call systemConfig::notifyTerminalModeEnd() when finished
        virtual void terminalNav(int num) {}
        virtual void terminalDraw()       {}
            // For the duration, these methods will be called.

        bool draw_needed;
        bool redraw_needed;
        
    private:
        
        void init_cold(configOption *parent, const char *tit, int typ, int min, int max);
            // ctor initialization
};



class integerOption : public configOption
{
    public:
        integerOption(configOption *parent, const char *title, int type, int min, int max);
        virtual const char *getValueString();
    protected:

        virtual bool beginTerminalMode();
        virtual void terminalNav(int num);
        virtual void terminalDraw();
        
        char buffer[10];        // biggest number!
};



class brightnessOption : public integerOption
{
    public:
        brightnessOption(configOption *parent);
        virtual void init();
        virtual void  setValue(int i);
};


class configNumOption : public integerOption
{
    public:
        configNumOption(configOption *parent);
        virtual void init();
        virtual const char *getValueString();
};


class onOffOption : public integerOption
{
    public:
        onOffOption(configOption *parent, const char *title);
        virtual const char *getValueString();
};



class midiHostOption : public onOffOption
{
    public:
        midiHostOption(configOption *parent);
        virtual void init();
};


class serialPortOption : public onOffOption
{
    public:
        serialPortOption(configOption *parent);
        virtual void init();
};


#endif  // !__configOptions_h_