#include <myDebug.h>
#include "pedals.h"
#include "prefs.h"
#include "expSystem.h"
#include "ftp.h"
#include "midiQueue.h"  // for mySendDeviceControlChange()
#include "commonDefines.h"
#include "rigDefs.h"


#define DEBUG_PEDALS  1

pedalManager thePedals;

//------------------------------------------------------
// Expression pedals are handled by polling,
// where we analogRead each input pedal port (0..1023)
// with a heuristic to track movments starting when the pedal
// moves some amount (given by the HYSTERISIS value here),
// until it stops for SETTLE_TIME milleseconds.
//-----------
// At the end of each Poll if anything changes the
// appropriate behavior takes place
//-----------
// The only difference between a "serial" pedal and a
// "normal" pedal is that the serial data is sent out
// to the rPi over SERIAL_DEVICE, wheras normally it is sent
// to the iPad over USB midi.


#define HYSTERISIS   30
    // in raw 0..1023 units
#define SETTLE_TIME  50
    // time to settle into a direction



//------------------------------------
// pedalManager
//------------------------------------

void pedalManager::init()
{
    m_pedals[PEDAL_SYNTH ].init(PEDAL_SYNTH,  PIN_EXPR1, "Synth",  SYNTH_VOLUME_CHANNEL,   SYNTH_VOLUME_CC);
    m_pedals[PEDAL_LOOP  ].init(PEDAL_LOOP,   PIN_EXPR2, "Loop",   0, 0);
    m_pedals[PEDAL_WAH   ].init(PEDAL_WAH,    PIN_EXPR3, "Wah",    GUITAR_EFFECTS_CHANNEL, GUITAR_WAH_CONTROL_CC);
    m_pedals[PEDAL_GUITAR].init(PEDAL_GUITAR, PIN_EXPR4, "Guitar", GUITAR_VOLUME_CHANNEL,  GUITAR_VOLUME_CC);

    // note that we don't use INPUT_PULLDOWN because there are
    // explicit 10K pulldowns on the PCB
    
    pinMode(PIN_EXPR1,INPUT);
    pinMode(PIN_EXPR2,INPUT);
    pinMode(PIN_EXPR3,INPUT);
    pinMode(PIN_EXPR4,INPUT);

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
    int cc_channel,
    int cc_num)
{
    m_num = num;
    m_pin = pin;
    m_cc_num = cc_num;
    m_cc_channel = cc_channel;
    m_name = name;

    m_raw_value = -1;         // 0..1023
    m_direction = -1;
    m_settle_time = 0;

    m_value = 0;
    m_valid = false;

    m_display_value = 0;
    m_last_display_value = -1;
}


//----------------------------------------------------
// poll one expression pedal
//----------------------------------------------------

void expressionPedal::poll()
{
    bool raw_changed = false;
    int raw_value =  analogRead(m_pin);
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

    if (raw_changed || !m_valid)
    {
        int value;

        // get the scaled x, and min and max values

        int scaled_x = getRawValueScaled();
        int curve_type = getPrefPedalCurve(m_num);
        int min_x = getPref8(PREF_PEDAL_CURVE_POINT(m_num,curve_type,0) + PEDAL_POINTS_OFFSET_X);
        int max_x = getPref8(PREF_PEDAL_CURVE_POINT(m_num,curve_type,curve_type+1) + PEDAL_POINTS_OFFSET_X);

        // we are to the left of the MIN, so our value is MIN.Y

        if (scaled_x < min_x)
        {
            value = getPref8(PREF_PEDAL_CURVE_POINT(m_num,curve_type,0) + PEDAL_POINTS_OFFSET_Y);
            display(0,"LESS THAN MIN_X value=%d",value);
        }

        // we are at, or to the right, of the MAX so our value is MAX.Y

        else if (scaled_x >= max_x)
        {
            value = getPref8(PREF_PEDAL_CURVE_POINT(m_num,curve_type,curve_type+1) + PEDAL_POINTS_OFFSET_Y);
            // display(0,"GE THAN MAX_X value=%d",value);
        }

        // loop thru points left (not max) pointstill we are at or to the right of one
        // or we are out of points

        else
        {
            int point_num = 0;
            int right_x = getPref8(PREF_PEDAL_CURVE_POINT(m_num,curve_type,point_num+1) + PEDAL_POINTS_OFFSET_X);

            while (point_num < curve_type &&
                   scaled_x >= right_x)
            {
                point_num++;
                right_x = getPref8(PREF_PEDAL_CURVE_POINT(m_num,curve_type,point_num+1) + PEDAL_POINTS_OFFSET_X);
            }

            // we are now between point_num and point_num+1

            int left_x = getPref8(PREF_PEDAL_CURVE_POINT(m_num,curve_type,point_num) + PEDAL_POINTS_OFFSET_X);
            // right_x = getPref8(PREF_PEDAL_CURVE_POINT(m_num,curve_type,point_num+1) + PEDAL_POINTS_OFFSET_X);
            int left_y = getPref8(PREF_PEDAL_CURVE_POINT(m_num,curve_type,point_num) + PEDAL_POINTS_OFFSET_Y);
            int right_y = getPref8(PREF_PEDAL_CURVE_POINT(m_num,curve_type,point_num+1) + PEDAL_POINTS_OFFSET_Y);

            // display(0,"SCALING %d BETWEEN POINT %d(%d,%d) and %d(%d,%d)",
            //     scaled_x,point_num,left_x,left_y,point_num+1,right_x,right_y);

            float range_x = (right_x - left_x) + 1;
            float range_y = (right_y - left_y) + 1;
            float val_x = (scaled_x - left_x);
            float pct = val_x / range_x;
            float val_y = left_y + pct * range_y + 0.5;
            value = val_y;

            // display(0,"    range_x(%0.2f) range_y(%0.2f) val_x(%0.2f) pct(%0.2f) val_y(%0.2f) VALUE=%d",
            //     range_x,range_y,val_x,pct,val_y,value);
        }

        // actual value changed
        // overwrites the display value

        if (value != m_value)
        {
            m_value = value;
            m_display_value = value;

            #if DEBUG_PEDALS
                display(0,"pedal(%d) raw(%d) changed to %d",m_num,m_raw_value,m_value);
            #endif

            thePedals.pedalEvent(m_num,m_value);
        }
    }

    m_valid = true;
}


float expressionPedal::getRawValuePct()
{
    float min = getPrefPedalCalibMin(m_num);
    float max = getPrefPedalCalibMax(m_num);
    float val = m_raw_value - min;
    if (val < 0.00) val = 0.00;
    float ret_val = val / (max - min);
    if (ret_val > 1.0) ret_val = 1.0;
    return ret_val;
}


int expressionPedal::getRawValueScaled()
{
    float ret_val = getRawValuePct() * 127.00 + 0.5;
    return ret_val;
}



//----------------------------------------------------
// pedalEvent
//----------------------------------------------------

void pedalManager::pedalEvent(int num, int value)
{
	expressionPedal *pedal = getPedal(num);

    // if it is then SYNTH pedal in ftp MONO mode, we send the
    // control messages out to channels 1-6

    if (num == PEDAL_SYNTH && !ftp_poly_mode)
    {
        for (int i=0; i<6; i++)
        {
            mySendDeviceControlChange(
                pedal->getCCNum(),
                value,
                i+1);
        }
    }
    else if (pedal->m_mode & PEDAL_MODE_SERIAL)
    {
        sendSerialControlChange(
            LOOP_CONTROL_BASE_CC + LOOPER_CONTROL_LOOP_VOLUME,
            value,
            "pedals.cpp");
    }
    else
    {
        mySendDeviceControlChange(
            pedal->getCCNum(),
            value,
            pedal->getCCChannel());
	}
}
