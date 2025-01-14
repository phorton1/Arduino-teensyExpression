#include <myDebug.h>
#include "pedals.h"
#include "prefs.h"
#include "expSystem.h"
#include "oldRig_defs.h"
#include "ftp.h"
#include "midiQueue.h"  // for mySendDeviceControlChange()
#include "commonDefines.h"


// 2020-09-20 prh I am not thrilled with the encapsulation of pedals at this time.
//
// The situation is fairly complex.  It is made more complex firstly by
// my notion of "rigs" which can have theoretically different pedal
// configurations, i.e. different CC's between the "old rig" and the
// "new rig". This is coupled with the fact that you want the
// pedals to keep working when you change to the configuration "rig".
//
// So, there is a global state for the pedals maintained in this object,
// which can be "poked" by changing "rigs".
//
// Another level of complexity is the experimental "AUTO_PEDAL" implementation,
// for my motorized foot pedals with haptic feedback.  In this case, the teensy
// port stops measuring the voltage (of a standard expression pedal) and starts
// to use a one wire bidirectional serial protocol, receiving NUMBERS from the pedal,
// which can also REACT to numbers sent to it, to go to a particular position, or
// activate some haptic feedback feature or mode.
//
// Then there is "midi over serial" to the rPi looper (only for Looper volume pedal)
// which has a hardwired "CC" and, instead of sending midi over USB, sends serial data,
// to the teensy 3.2 inside the Looper which then relays it to the rPi.
//
// Also note that each pedal, even with all that, has a set of three distinct
// programmable "curves", and all of the configuration *should* be
// stored in EEPROM though NONE of the CC's are stored there at this
// time (they are all in defines or spread throughout various code)
//
// Finally, with the introduction of the songMachine, there is a
// "higher level" value which can be poked by the songMachine,
// and will redisplay properly in expSystem UNTIL the pedal is
// moved (and the actual underlying value changes) ...


#define DEBUG_PEDALS  0

pedalManager thePedals;

//------------------------------------------------------
// "normal" expression pedals are handled by polling,
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


//--------------------------------------------------------
// My "automatic" pedals communicate through the same teensy
// ports using a simple one wire serial protocol.  In that
// case, the pin is hooked up to an isr that receives a byte
// to/from the arduino inside the pedal.
//
// Here we define the isr routines for each pedal, should they
// be used.

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
    for (int i=0; i<4; i++)     // kludgy default for quantiloop relative volumes
        m_relative_loop_volume[i] = 100;

    m_pedals[PEDAL_SYNTH ].init(PEDAL_SYNTH,  PIN_EXPR1, "Synth",  SYNTH_VOLUME_CHANNEL,   SYNTH_VOLUME_CC);
    m_pedals[PEDAL_LOOP  ].init(PEDAL_LOOP,   PIN_EXPR2, "Loop",   QUANTILOOP_CHANNEL,   QUANTILOOP_CC_LOOP_VOLUME);
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

    m_display_value = 0;
    m_last_display_value = -1;

    setPedalMode();
}


//-------------------------------------------
// "automatic pedal(s)"
//-------------------------------------------
// "Automatic Pedals" are an experimental feature that
// uses a hardware expression pedal I am building, which is
// in developement, that contains a servo/motor to provide
// haptic feedback, and which can "return" to a given position,
// much like an automated slider on a modern mixer.
//
// These pedal use a specific purpose built one-wire protocol
// to send the pedal value to this device, and can accept, at
// this time, commands to calibrate themselves, and/or goto a
// specific position.

void expressionPedal::setPedalMode()
{
    m_auto_value = 0;
    m_mode = getPrefPedalMode(m_num);
    m_in_auto_calibrate = 0;

    if (m_mode & PEDAL_MODE_AUTO)
    {
        display(1,"AUTO_PEDAL(%d) pin=%d",m_num,m_pin);
        pinMode(m_pin,INPUT);
        attachInterrupt(digitalPinToInterrupt(m_pin), pedal_isrs[m_num], RISING );
    }
    else
    {
        display(1,"REGULAR_PEDAL(%d)",m_num);
        pinMode(m_pin,INPUT_PULLDOWN);
    }
}

void expressionPedal::autoCalibrate()
{
    if (m_mode & PEDAL_MODE_AUTO)
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
    if (m_mode & PEDAL_MODE_AUTO)
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


//-----------------------------------------
// Automatic Pedal "One Wire" Protocol
//-----------------------------------------

#define TEENSY_DELAY            100
#define TEENSY_START_IN_DELAY   (6 * TEENSY_DELAY / 5)
#define TEENSY_END_OUT_DELAY    (4 * TEENSY_DELAY / 5)


void expressionPedal::teensyReceiveByte()
    // called as an ISR on a RISING edge from the pedal.
    // reads a byte from the pedal, with timings derived
    // empirically to match the signals from the Arduino
    // autoPedal.ino sketch.
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
    if (!(m_mode & PEDAL_MODE_AUTO))
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





//----------------------------------------------------
// poll one expression pedal
//----------------------------------------------------

void expressionPedal::poll()
{
    if (m_in_auto_calibrate)
        return;

    bool raw_changed = false;
    bool is_auto = m_mode & PEDAL_MODE_AUTO;

    int raw_value =  is_auto ? m_auto_value : analogRead(m_pin);
    unsigned time = millis();
    int use_hysterisis = is_auto ? 0 : HYSTERISIS;

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

    // if the Looper is in oldRig "relative volume mode"
    // orchestrate the four messages to be sent here.
    // the CCs are currently SEQUENTIAL constants in oldRig_defs.h

    else if (num == PEDAL_LOOP && m_relative_loop_volume_mode)
    {
        for (int i=0; i<4; i++)
        {
            float vol = value;
            float rel_vol = m_relative_loop_volume[i];
            float new_value = (vol/127.0) * rel_vol;
            int cc = QUANTILOOP_CC_TRACK_VOLUME1 + i;

            mySendDeviceControlChange(
                cc,
                new_value,
                pedal->getCCChannel());
        }
    }

    // default behavior, use constants from array

    else if (pedal->m_mode & PEDAL_MODE_SERIAL)
    {
        // ONLY ONE PEDAL IS SERIAL AT THIS TIME
        // IT IS THE LOOP VOLUME PEDAL
        // AND IS HARDWIRED HERE TO SEND CC 0x67

        // 2023-08-05 changed to use existing defines

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
