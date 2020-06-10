#ifndef __configOptions_h_
#define __configOptions_h_

#include "systemConfig.h"
#include "myLeds.h"


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
    
        configOption()
        {
            init(0,"",0,0,0);
        }
        
        configOption(configOption *parent, const char *title, int type)
        {
            init(parent,title,type,0,0);
        }
    
        configOption(configOption *parent, const char *title, int type, int min, int max)
        {
            init(parent,title,type,min,max);
        }
    
        virtual void init()
            // called recursively during begin() on subclasses that
            // are linked to other objects to initialize the value, etc
        {
            value            = 0;
            orig_value       = 0;
            display_value    = -1;
            selected         = 0;
            display_selected = 0;
            
            configOption *opt = pFirstChild;
            while (opt) { opt->init(); opt=opt->pNextOption; }
        }

        
        virtual int   getValue()              { return value; }
        virtual int   getOrigValue()          { return orig_value; }
        virtual bool  needsValueDisplay()     { return display_value != value; }
        virtual void  clearDisplayValue()     { display_value = value; }
        virtual const char *getValueString()  { return ""; }
        virtual const char *getTitle()        { return title; }

        virtual void  setValue(int i)   
        {
            value = i;  // enforces min/max
            if (value > max_value) value = max_value;
            if (value < min_value) value = min_value;
        }
        virtual void incValue(int inc_dec)  // wraps
        {
            value += inc_dec;   // wraps thru min/max
            if (value > max_value) value = min_value;
            if (value < min_value) value = max_value;
        }
        
    protected:
         
        friend class systemConfig;
        
        
        const char *title;
        int  type;

        int  value;
        int  orig_value;
        int  display_value;
        int  min_value;
        int  max_value;
             
        int  selected;
        int  display_selected;
    
        configOption *pParent;
        configOption *pNextOption;
        configOption *pPrevOption;
        configOption *pFirstChild;
        configOption *pLastChild;
    
        navButtonHandler *pNavFunction;
        drawHandler *pDrawFunction;
    
        void setTerminalMode(
            navButtonHandler *navFunction,
            drawHandler *drawFunction)
        {
            pNavFunction  = navFunction;
            pDrawFunction = drawFunction;
        }
        
        
    private:
        
        void init(configOption *parent, const char *tit, int typ, int min, int max)
        {
            title         = tit;
            type          = typ;
            pParent       = parent;
            min_value     = min;
            max_value     = max;

            value         = 0;
            orig_value    = 0;
            display_value = -1;
            selected      = 0;
            display_selected = 0;
    
            pNavFunction = 0;
            pDrawFunction = 0;
            
            pPrevOption = 0;
            
            if (parent)
                pPrevOption = parent->pLastChild;
            if (pPrevOption)
                pPrevOption->pNextOption = this;
            if (parent && !parent->pFirstChild)
                parent->pFirstChild = this;
            if (parent)
                parent->pLastChild = this;
        }

};



class integerOption : public configOption
{
    public:
        
        integerOption(configOption *parent, const char *title, int type, int min, int max) :
            configOption(parent,title,type | OPTION_TYPE_VALUE,min,max)
        {}
    
        virtual const char *getValueString()
        {
            sprintf(buffer,"%d",value);
            return (const char *) buffer;
        }
        
    protected:
        
        char buffer[10];        // biggest number!

};



class brightnessOption : public integerOption
{
    public:
        
        brightnessOption(configOption *parent) :
            integerOption(parent,"Brightness",
                OPTION_TYPE_TERMINAL | OPTION_TYPE_BRIGHTNESS,1,100)
        {}
        
        virtual void init()
        {
            integerOption::init();
            value = getLEDBrightness(); 
            orig_value = value;
            display_value = -1;
        }
        virtual void  setValue(int i)
        {
            integerOption::setValue(i);
            setLEDBrightness(value);
        }
    
};


class configNumOption : public integerOption
{
    public:
        
        configNumOption(configOption *parent) :
            integerOption(parent,"Config",
                OPTION_TYPE_IMMEDIATE | OPTION_TYPE_CONFIG_NUM,1,0)
        {}
    
        virtual void init()
        {
            integerOption::init();
            value = s_pTheSystem->getPrevConfigNum();
            max_value = s_pTheSystem->getNumConfigs() - 1;
            orig_value = value;
            display_value = -1;
        }
        virtual const char *getValueString()
        {
            return s_pTheSystem->getConfig(value)->short_name();
        }
};


class onOffOption : public integerOption
{
    public:
        
        onOffOption(configOption *parent, const char *title) :
            integerOption(parent,title,OPTION_TYPE_IMMEDIATE,0,1)
        {}
        
        virtual const char *getValueString()
        {
            return value ? "ON" : "OFF";
        }
};



class midiHostOption : public onOffOption
{
    public:
        midiHostOption(configOption *parent) :
            onOffOption(parent,"Midi Host") {}
        void init()
        {
            onOffOption::init();
            orig_value = value = midi_host_on;
        }
};


class serialPortOption : public onOffOption
{
    public:
        serialPortOption(configOption *parent) :
            onOffOption(parent,"Serial Port") {}
        void init()
        {
            onOffOption::init();
            orig_value = value = serial_port_on;
        }
};


#endif  // !__configOptions_h_