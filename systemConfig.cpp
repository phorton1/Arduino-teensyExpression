
#include <myDebug.h>
#include "defines.h"
#include "systemConfig.h"
#include "myLeds.h"
#include "pedals.h"
#include "rotary.h"
#include <EEPROM.h>



#define COL_BRIGHTNESS_DOWN     0
#define COL_BRIGHTNESS_UP       1
#define COL_EXIT_CANCEL         3
#define COL_EXIT_DONE           4

#define ROW_CONFIGS             1


#define ROTARY_FOR_BRIGHTNESS   1

#define BRIGHNESS_INC_DEC       (100/INCS_PER_REV)      // one full revolution for 100
#define BRIGHTNESS_STEEP        2.3
#define BRIGHTNESS_OFFSET       0.10
    // defines for ad-hoc curve for brighness    




#define WITH_CURVE_EDITOR   1

#if WITH_CURVE_EDITOR

    #include "curves.h"
    
    #define NUM_TO_ROW(n)    (n / NUM_BUTTON_ROWS)
    #define NUM_TO_COL(n)    (n % NUM_BUTTON_COLS)
    #define RC_TO_NUM(r,c)   ((r)*NUM_BUTTON_COLS + (c))
    
    #define CURVE_BUTTON_CHOOSE_CURVE       20
    #define CURVE_BUTTON_CHOOSE_POINT       15
    #define CURVE_BUTTON_MOVE_UP            12
    #define CURVE_BUTTON_MOVE_DOWN          22
    #define CURVE_BUTTON_MOVE_LEFT          16
    #define CURVE_BUTTON_MOVE_RIGHT         18
    #define CURVE_BUTTON_CENTER             17
    
    #define NUM_CURVE_BUTTONS       7
    
    typedef struct
    {
        int row;
        int col;
        int command;
        int color;
    } curveButton_t;
    
    
    curveButton_t  curve_button[NUM_CURVE_BUTTONS] = {
        { 4, 0, CURVE_COMMAND_SELECT_NEXT_CURVE,  LED_BLUE   },
        { 3, 0, CURVE_COMMAND_SELECT_NEXT_POINT,  LED_CYAN   },
        { 2, 2, CURVE_COMMAND_MOVE_UP          ,  LED_GREEN  }, 
        { 4, 2, CURVE_COMMAND_MOVE_DOWN        ,  LED_GREEN  }, 
        { 3, 1, CURVE_COMMAND_MOVE_LEFT        ,  LED_GREEN  },
        { 3, 3, CURVE_COMMAND_MOVE_RIGHT       ,  LED_GREEN  },
        { 3, 2, CURVE_COMMAND_INC_PARAM        ,  LED_RED    }
    };
    
    unsigned move_time = 0;
    int move_command = 0;
    

    void enableCurveButtons(rawButtonArray *ba)
    {
        for (int i=0; i<NUM_CURVE_BUTTONS; i++)
        {
            curveButton_t *cb = &curve_button[i];
            int event = 0;
            int color = 0;
            if (curve_command_can(cb->command))
            {
                event = BUTTON_EVENT_PRESS | BUTTON_EVENT_RELEASE;
                color = cb->color;
            }
            ba->setButtonEventMask(cb->row,cb->col,event);
            setLED(cb->row,cb->col,color);
        }
    }
    
    
    bool handleCurveButton(rawButtonArray *ba, int row, int col, int event)
    {
        for (int i=0; i<NUM_CURVE_BUTTONS; i++)
        {
            curveButton_t *cb = &curve_button[i];
            if (row == cb->row && col == cb->col)
            {
                if (event == BUTTON_EVENT_PRESS)
                {
                    curve_command(cb->command);
                    enableCurveButtons(ba);
                    if (i > 1)
                    {
                        move_time = millis();
                        move_command = cb->command;
                    }
                }
                else
                {
                    setLED(cb->row,cb->col,cb->color);
                    move_time = 0;
                    draw_curve(true);
                }
                showLEDs();
                return true;
            }
        }
        return false;
    }
    
#endif




systemConfig::systemConfig(expSystem *pSystem) :
    expConfig(pSystem)
{
    m_orig_brightness   = 0;
    m_orig_config_num   = 0;
    
    m_brightness        = 0;
    m_config_num        = 0;
    
    m_last_changed      = -1;
    m_last_brightness   = -1;
    m_last_config_num   = -1;
}




bool systemConfig::changed()
{
    return
        m_brightness != m_orig_brightness ||
        m_config_num != m_orig_config_num;
}





// virtual
void systemConfig::begin()
{
    expConfig::begin();
    
    m_last_changed      = -1;
    m_last_brightness   = -1;
    m_last_config_num   = -1;
    m_cancel_enabled    = 0;

    m_orig_brightness = m_brightness = getLEDBrightness();
    m_orig_config_num = m_config_num = m_pSystem->getPrevConfigNum();
    
    setRotary(ROTARY_FOR_BRIGHTNESS,1,100,BRIGHNESS_INC_DEC);
    setRotaryValue(ROTARY_FOR_BRIGHTNESS, m_brightness);
    
    rawButtonArray *ba = m_pSystem->getRawButtonArray();
    
    ba->setButtonEventMask(0,COL_BRIGHTNESS_DOWN,   BUTTON_EVENT_CLICK);
    ba->setButtonEventMask(0,COL_BRIGHTNESS_UP,     BUTTON_EVENT_CLICK);
    ba->setButtonEventMask(0,COL_EXIT_DONE,         BUTTON_EVENT_CLICK | BUTTON_EVENT_LONG_CLICK);
    
    setLED(0,COL_BRIGHTNESS_DOWN,   LED_RED);        
    setLED(0,COL_BRIGHTNESS_UP,     LED_GREEN);      
    setLED(0,COL_EXIT_DONE,         LED_PURPLE);     
    
    enableButtons();
    
    for (int i=0; i<m_pSystem->getNumConfigs()-1; i++)
    {
        ba->setButtonEventMask(ROW_CONFIGS,i,BUTTON_EVENT_CLICK);
        setLED(1,i, i == m_config_num-1 ? LED_WHITE : LED_BLUE);
    }
    
    #if WITH_CURVE_EDITOR
        enableCurveButtons(ba);
        draw_curve(true);
    #endif
    
    showLEDs();
    
    // testing curves
    // just show they are complicated.
    //
    // void showBrightnessCurve(float steep, float offset);
    // showBrightnessCurve(BRIGHTNESS_STEEP,BRIGHTNESS_OFFSET);
    
}


void systemConfig::enableButtons()
{
    bool is_changed = changed();
    display(0,"enableButtons(%d)",is_changed);
    if (is_changed !=  m_cancel_enabled)
    {
        m_cancel_enabled = is_changed;
        rawButtonArray *ba = m_pSystem->getRawButtonArray();
        if (is_changed)
        {
            ba->setButtonEventMask(0,COL_EXIT_CANCEL,BUTTON_EVENT_CLICK);
            setLED(0,COL_EXIT_CANCEL,LED_YELLOW);
        }
        else
        {
            ba->setButtonEventMask(0,COL_EXIT_CANCEL,0);
            setLED(0,COL_EXIT_CANCEL,0);
        }
    }
}


// virtual
void systemConfig::onButtonEvent(int row, int col, int event)
{
    display(0,"systemConfig(%d,%d) event(%s)",row,col,rawButtonArray::buttonEventName(event));
    
    #if WITH_CURVE_EDITOR
        if (handleCurveButton(m_pSystem->getRawButtonArray(),row,col,event))
            return;
    #endif
    
    if (row == 0)
    {
        if (col == COL_BRIGHTNESS_DOWN)
        {
            m_brightness -= 5;
            if (m_brightness < 1) m_brightness = 1;
            display(0,"decrease brightness to %d",m_brightness);
            setRotaryValue(ROTARY_FOR_BRIGHTNESS, m_brightness);
            setLEDBrightness(m_brightness);
            setLED(0,0,LED_RED);
            enableButtons();
            showLEDs();
        }
        else if (col == COL_BRIGHTNESS_UP)
        {
            m_brightness += 5;
            if (m_brightness > 100) m_brightness = 100;
            display(0,"increase brightness to %d",m_brightness);
            setRotaryValue(ROTARY_FOR_BRIGHTNESS, m_brightness);
            setLEDBrightness(m_brightness);
            setLED(0,1,LED_GREEN);
            enableButtons();
            showLEDs();
        }
        else if (col == COL_EXIT_CANCEL)  // abort - don't bother with colors
        {
            setLEDBrightness(m_orig_brightness);
            m_pSystem->activateConfig(m_orig_config_num);
        }
        else if (col == COL_EXIT_DONE)
        {
            if (event == BUTTON_EVENT_LONG_CLICK)
            {
                display(0,"write bright=%d and config=%d to EEPROM",m_brightness,m_config_num);
                EEPROM.write(EEPROM_BRIGHTNESS,m_brightness);
                EEPROM.write(EEPROM_CONFIG_NUM,m_config_num);
            }
            m_pSystem->activateConfig(m_config_num);
        }
    }
    else if (row == 1)
    {
        if (col != m_config_num-1)
        {
            if (m_config_num)
                setLED(1,m_config_num-1,LED_BLUE);
            m_config_num = col + 1;
            enableButtons();
            showLEDs();
        }
    }
}



int brightnessCurve(int x, int min, int max, float steep, float offset)
    // curves are difficult.
    // this works "so so" for brightness
    // it producess an S curve of various steepness
    // that you can move right or left.
    // however, moving it right means that you will
    // also need to scale it, and there are dead values near zero
{
    float x_float = ((float)x) / ((float) max-min);
    x_float *= (1 - offset);
    
    float val = 1/ (    1 + pow( x_float/(1-x_float) , steep)     );
    val = 1 - val;

    int y = min + (max-min) * val;
    
    display(0,"curve(%d,%d,%d,%0.2f,0%2f) = %0.2f = %d",x,min,max,steep,offset,val,y);
    return y;
}



void showBrightnessCurve(float steep, float offset)
{
    #define X_OFFSET    100.00
    #define Y_OFFSET    100.00
    #define CHART_MAX       199.00
    
    mylcd.Set_Draw_color(TFT_WHITE);
    mylcd.Draw_Line(X_OFFSET,Y_OFFSET,X_OFFSET,Y_OFFSET+CHART_MAX);
    mylcd.Draw_Line(X_OFFSET,Y_OFFSET+CHART_MAX,X_OFFSET+CHART_MAX,Y_OFFSET+CHART_MAX);
    
    for (int x=0; x<CHART_MAX; x++)
    {
        int y0 = CHART_MAX - x;     // line
        int yc = CHART_MAX - brightnessCurve(x,0,CHART_MAX,steep,offset);
        
        mylcd.Draw_Pixe(X_OFFSET + x, Y_OFFSET + y0, TFT_WHITE);
        mylcd.Draw_Pixe(X_OFFSET + x, Y_OFFSET + yc, TFT_YELLOW);
    }
}



// virtual
void systemConfig::onRotaryEvent(int num, int val)
{
    // makes use of early, external, curve applied post-facto
    // to linear "jumpy" rotary values ...
    // works "so so" ...
    // still a couple of dead clicks at top and bottom
    
    display(0,"systemConfig::onRotaryEvent(%d) val=%d",num,val);
    if (num == ROTARY_FOR_BRIGHTNESS)
    {
        m_brightness = brightnessCurve(val,1,100,BRIGHTNESS_STEEP,BRIGHTNESS_OFFSET);
        setLEDBrightness(m_brightness);
        enableButtons();
        showLEDs();
    }
}



// virtual
void systemConfig::updateUI()
{
    #if WITH_CURVE_EDITOR
        if (move_time)
        {
            unsigned elapsed = millis() - move_time;
            if (elapsed > 400 && (elapsed % 30 == 0))
            {
                if (curve_command_can(move_command))
                    curve_command(move_command);
                else move_time = 0;
            }
        }
        // else
        //     draw_curve(false);
    #endif    
}




