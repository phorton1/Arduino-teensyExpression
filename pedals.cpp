#include <myDebug.h>
#include "pedals.h"
#include "prefs.h"
#include "expSystem.h"
#include "oldRig_defs.h"

#define IS_AUTO_PEDAL   0
    // set to 5 or something for old behavior


#define HYSTERISIS   30
    // in raw 0..1023 units
#define SETTLE_TIME  50
    // time to settle into a direction
#define DEBUG_PEDALS  1

pedalManager thePedals;

typedef void (*isr_fxn)();

void pedal_isr0()       { thePedals.getPedal(0)->teensyReceiveByte(); }
void pedal_isr1()       { thePedals.getPedal(1)->teensyReceiveByte(); }
void pedal_isr2()       { thePedals.getPedal(2)->teensyReceiveByte(); }
void pedal_isr3()       { thePedals.getPedal(3)->teensyReceiveByte(); }
isr_fxn pedal_isrs[NUM_PEDALS] = {pedal_isr0, pedal_isr1, pedal_isr2, pedal_isr3};


//------------------------------------
// pedalManager
//------------------------------------

void pedalManager::init()
{
    m_pedals[PEDAL_SYNTH ].init(PEDAL_SYNTH,  PIN_EXPR1, "Synth",  SYNTH_VOLUME_CHANNEL,   SYNTH_VOLUME_CC);
    m_pedals[PEDAL_LOOP  ].init(PEDAL_LOOP,   PIN_EXPR2, "Loop",   LOOP_CONTROL_CHANNEL,   LOOP_VOLUME_CC);
    m_pedals[PEDAL_WAH   ].init(PEDAL_WAH,    PIN_EXPR3, "Wah",    GUITAR_EFFECTS_CHANNEL, GUITAR_WAH_CONTROL_CC);
    m_pedals[PEDAL_GUITAR].init(PEDAL_GUITAR, PIN_EXPR4, "Guitar", GUITAR_VOLUME_CHANNEL,  GUITAR_VOLUME_CC);
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
    m_last_value = -1;

    setAuto();
}



void expressionPedal::setAuto()
{
    m_auto_value = 0;
    m_auto = getPrefPedalAuto(m_num);
    m_in_auto_calibrate = 0;

    if (m_auto)
    {
        display(0,"AUTO_PEDAL(%d) pin=%d",m_num,m_pin);
        pinMode(m_pin,INPUT);
        attachInterrupt(digitalPinToInterrupt(m_pin), pedal_isrs[m_num], RISING );
    }
    else
    {
        display(0,"REGULAR_PEDAL(%d)",m_num);
        pinMode(m_pin,INPUT_PULLDOWN);
    }
}

void expressionPedal::autoCalibrate()
{
    if (m_auto)
    {
        m_raw_value = -1;
        m_in_auto_calibrate = 1;
        teensySendByte(0xF1);      // send calibrate command
    }
    else
    {
        my_error("Attempt to autoCalibrate when pedal(%d) is not m_auto",m_num);
    }
}

void expressionPedal::setAutoRawValue(int value)
{
    if (m_auto)
    {
        if (value > 127) value = 127;
        if (value < 0) value = 0;
        teensySendByte(value);      // send calibrate command
    }
    else
    {
        my_error("Attempt to setAutoRawValue(%d==0x%02x) when pedal is not m_auto",value,value,m_num);
    }

}





void expressionPedal::poll()
{
    if (m_in_auto_calibrate)
        return;

    bool raw_changed = false;
    int raw_value = m_auto ? m_auto_value : analogRead(m_pin);
    unsigned time = millis();
    int use_hysterisis = m_auto ? 0 : HYSTERISIS;

    // display(0,"poll(%d) raw_value=%d",m_num,raw_value);

    // if not moving, and outside of hysterisis range, start moving

    if (!m_direction)
    {
        if (raw_value > m_raw_value + use_hysterisis)
        {
            m_direction = 1;
            m_raw_value = raw_value;
            m_settle_time = time;
            raw_changed = 1;
        }
        else if (raw_value < m_raw_value - use_hysterisis)
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
            display(0,"GE THAN MAX_X value=%d",value);
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

        #if 0   // old way
            int calib_min = getPrefPedalCalibMin(m_num);
            int calib_max = getPrefPedalCalibMax(m_num);
            int value_min = getPrefPedalMin(m_num);
            int value_max = getPrefPedalMax(m_num);
            int value = map(m_raw_value,calib_min,calib_max,value_min,value_max);
            if (value > value_max) value = value_max;
            if (value < value_min) value = value_min;
        #endif

        if (value != m_value)
        {
            m_value = value;
            #if DEBUG_PEDALS
                display(0,"pedal(%d) raw(%d) changed to %d",m_num,m_raw_value,m_value);
            #endif
            theSystem.pedalEvent(m_num,m_value);
        }
    }

    m_valid = true;
}



#define TEENSY_DELAY            100
#define TEENSY_START_IN_DELAY   (6 * TEENSY_DELAY / 5)
#define TEENSY_END_OUT_DELAY    (4 * TEENSY_DELAY / 5)


void expressionPedal::teensyReceiveByte()
    // quick and dirty, timings derived empirically to
    // match the arduino code's arbitrary constants.
{
    delayMicroseconds(TEENSY_START_IN_DELAY);
    int value = 0;
    for (int i=0; i<8; i++)
    {
        value = (value << 1) | digitalRead(m_pin);
        delayMicroseconds(TEENSY_DELAY);
    }
    int stop_bit = digitalRead(m_pin);
    digitalWrite(m_pin,0);
        // this appeared to be needed to drive the signal low
        // or else a 2nd interrupt was always triggered
    display(0,"TEENSY RECEIVED pedal=%d byte=0x%02x  dec(%d)  stop=%d",m_num,value,value,stop_bit);
    m_auto_value = value;
    m_in_auto_calibrate = 0;
}


void expressionPedal::teensySendByte(int byte)
{
    if (!m_auto)
    {
        my_error("Attempt to call teensySendByte(0x%02x) when pedal(%d) is not m_auto",byte,m_num);
        return;
    }
    display(0,"teensySendByte(%d, 0x%02x) dec(%d)",m_num,byte,byte);
    pinMode(m_pin,OUTPUT);
    digitalWrite(m_pin,0);                  // start bit
    delayMicroseconds(TEENSY_DELAY);
    digitalWrite(m_pin,1);                  // start bit
    delayMicroseconds(TEENSY_DELAY);

    for (int i=0; i<8; i++)
    {
        digitalWrite(m_pin,(byte >> (7-i)) & 0x01);      // MSb first
        delayMicroseconds(TEENSY_DELAY);
    }
    digitalWrite(m_pin,1);        // stop bit
    delayMicroseconds(TEENSY_END_OUT_DELAY);
    digitalWrite(m_pin,0);        // finished

    pinMode(m_pin,INPUT);
    attachInterrupt(digitalPinToInterrupt(m_pin), pedal_isrs[m_num], RISING );
}
