#include "defines.h"

#if WITH_PEDALS     // empty compile if not

#include "pedals.h"
#include <myDebug.h>
#include "expSystem.h"


#define HYSTERISIS   30
    // in raw 0..1023 units
#define SETTLE_TIME  50
    // time to settle into a direction
#define DEBUG_PEDALS  1

pedalManager thePedals;


//------------------------------------
// pedalManager
//------------------------------------

void pedalManager::init()
{
    m_pedals[0].init(0,PIN_EXPR1,"Synth");
    m_pedals[1].init(1,PIN_EXPR2,"Loop",96);
    m_pedals[2].init(2,PIN_EXPR3,"Wah");
    m_pedals[3].init(3,PIN_EXPR4,"Guitar");
}


void pedalManager::task()
{
    for (int i=0; i<NUM_PEDALS; i++)
        m_pedals[i].poll();
}



//------------------------------------
// expressionPedal
//------------------------------------

void expressionPedal::init(
    int num,
    int pin,
    const char *name,
    int value_max)
{
    m_num = num;
    m_pin = pin;
    m_name = name;
    
    m_calib_min  = 0;      
    m_calib_max  = 1023;   
    m_curve_type = 0;     
    m_value_min  = 0;      
    m_value_max  = value_max;
    
    for (int i=0; i<MAX_PEDAL_CURVE_POINTS; i++)
    {
        m_points[i].x = 0;
        m_points[i].y = 0;
        m_points[i].weight = 0;
    }

    m_raw_value = -1;         // 0..1023
    m_direction = -1;
    m_settle_time = 0; 
        
    m_value = 0;
    m_last_value = -1;
    
    m_cur_point = m_curve_type + 1;
    
    pinMode(m_pin,INPUT_PULLDOWN);
    
}



void expressionPedal::poll()
{
    bool raw_changed = false;
    int raw_value = analogRead(m_pin);
    unsigned time = millis();
    
    // display(0,"poll(%d) raw_value=%d",m_num,raw_value);
    
    // if not moving, and outside of hysterisis range, start moving
    
    if (!m_direction)
    {
        if (raw_value > m_raw_value + HYSTERISIS)
        {
            m_direction = 1;
            m_raw_value = raw_value;
            m_settle_time = time;
            raw_changed = 1;
        }
        else if (raw_value < m_raw_value - HYSTERISIS)
        {
            m_direction = -1;
            m_raw_value = raw_value;
            m_settle_time = time;
            raw_changed = 1;
        }
    }
    
    // if stopped moving, reset to default state
    
    else if (time >= m_settle_time + SETTLE_TIME)
    {
        m_settle_time = 0;
        m_direction = 0;
    }
    
    // otherwise, process the input
    
    else if (m_direction > 0 && raw_value > m_raw_value)
    {
        m_raw_value = raw_value;
        m_settle_time = time;
        raw_changed = 1;
    }
    else if (m_direction < 0 && raw_value < m_raw_value)
    {
        m_raw_value = raw_value;
        m_settle_time = time;
        raw_changed = 1;
    }
    
    
    //-------------------------------
    // calculate value
    //-------------------------------
    
    if (raw_changed)
    {
        int value = map(m_raw_value,m_calib_min,m_calib_max,m_value_min,m_value_max);
        if (value > m_value_max) value = m_value_max;
        if (value < m_value_min) value = m_value_min;
        
        if (value != m_value)
        {
            m_value = value;
            #if DEBUG_PEDALS
                display(0,"pedal(%d) raw(%d) changed to %d",m_num,m_raw_value,m_value);
            #endif
            theSystem.pedalEvent(m_num,m_value);
        }
    }
}



#endif  // WITH_PEDALS     // empty compile if not
