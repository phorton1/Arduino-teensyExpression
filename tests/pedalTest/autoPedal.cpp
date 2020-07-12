#include <myDebug.h>
#include "autoPedal.h"
#include "ampMeter.h"
#include <EEPROM.h>


#define dbg_pedal           0
#define dbg_moves           0
#define dbg_factory_calib   0

#define PEDAL_IN_PIN                A7    // Analog input from Pedal poteniometer
#define PEDAL_OUT_PIN1              9     // PWM output to Motor Controller (L293, L9110 or similar)
#define PEDAL_OUT_PIN2              10

// default calibration values

#define DEFAULT_CALIB_MIN           80
#define DEFAULT_CALIB_MAX           1000
#define EEPROM_CALIB_MIN            10
#define EEPROM_CALIB_MAX            12

#define FACTORY_CALIB_NONE          0x0000
#define FACTORY_CALIB_MAX           0x0001
#define FACTORY_CALIB_MIN           0x0002
#define FACTORY_CALIB_MIN_FORCE     0x0004

#define FACTORY_CALIB_MAX_SAFETY_MARGIN  10         // 0..1023
#define FACTORY_CALIB_MIN_SAFETY_MARGIN  10         // 0..1023

// amp meter

#define PEDAL_DEFAULT_OVERLOAD            300
#define PEDAL_DEFAULT_AVERAGE_OVERLOAD    150
    // lower values also protect the mechanism


// default movement values

#define FORCE_START_MOVE      255
#define FORCE_CONTINUE_MOVE   180
#define FORCE_SLOW_MOVE       80
#define STALLED_MILLIAMPS     160

#define MOVE_SETTLE_TIME            50000           // micros
#define PEDAL_CHANGE_THRESHOLD      2               // 0..1023
#define MOVE_TIMEOUT                4000000         // if it can't be done in two seconds, bail!


int readEPROM16(int location, int def_value=0, const char *name="")
{
    uint16_t ret_val = def_value;
    uint16_t low = EEPROM.read(location);
    uint16_t high = EEPROM.read(location+1);
    if (high!=255 || low!=255)
    {
        ret_val = low | (high<<8);
        if (*name)
            display(dbg_pedal,"got EEPROM %s %d == 0x%04x",name,ret_val,ret_val);

    }
    return ret_val;
}


void writeEPROM16(int location, int value, const char *name="")
{
    if (*name)
        display(dbg_pedal-1,"wrote EEPROM %s %d == 0x%04x",name,value,value);
    EEPROM.write(location,value & 0xff);
    EEPROM.write(location+1,(value >> 8) & 0xff);
}



//-----------------------------
// implementation
//-----------------------------

autoPedal thePedal(PEDAL_IN_PIN,PEDAL_OUT_PIN1,PEDAL_OUT_PIN2);


autoPedal::autoPedal(int in_pin, int out_pin1, int out_pin2)
{
    m_in_pin = in_pin;
    m_out_pin1 = out_pin1;
    m_out_pin2 = out_pin2;
    pinMode(m_in_pin,INPUT);
    pinMode(m_out_pin1,OUTPUT);
    pinMode(m_out_pin2,OUTPUT);
    analogWrite(m_out_pin1,0);
    analogWrite(m_out_pin2,0);
    theAmpMeter.setOverload(PEDAL_DEFAULT_OVERLOAD);
    theAmpMeter.setAveragOverload(PEDAL_DEFAULT_AVERAGE_OVERLOAD);
}


void autoPedal::start()
{
    // Increase pin 9 & 10 clock PWM clock rate, resulting in
    // smoother movements and more predictable behavior.

    TCCR1B = TCCR1B & B11111000 | B00000010;
        // for PWM frequency of 3921.16 Hz

    m_calib_min = readEPROM16(EEPROM_CALIB_MIN,DEFAULT_CALIB_MIN,"calib_min");
    m_calib_max = readEPROM16(EEPROM_CALIB_MAX,DEFAULT_CALIB_MAX,"calib_max");

    clear();
}


void autoPedal::clear()
{
    analogWrite(m_out_pin1,0);
    analogWrite(m_out_pin2,0);

    m_value             = -1;
    m_sample_count      = 0;
    m_sample_total      = 0;
    m_last_calc_time    = 0;
    m_last_sample_time  = 0;

    m_state = PEDAL_STATE_NONE;
    m_factory_calib_state = FACTORY_CALIB_NONE;
    m_move_state = MOVE_STATE_NONE;

    m_force             = 0;
    m_direction         = 0;
    m_last_value        = -1;

    m_range             = 0;
    m_desired_value     = 0;
    m_starting_value    = 0;
    m_velocity          = 0;

    m_num_steps         = 0;
    m_start_time        = 0;
    m_settle_time       = 0;
}



bool autoPedal::pedalAvailable()
{
    if (m_state || m_move_state)
        return false;
    return true;
}


const char *pedalStateName(int state)
{
    if (state == PEDAL_STATE_NONE)              return "NONE";
    if (state == PEDAL_STATE_MOVE)              return "MOVE";
    if (state == PEDAL_STATE_FACTORY_CALIB)     return "FACTORY_CALIB";
    if (state == PEDAL_STATE_FIND_START_FORCE)  return "FIND_START_FORCE";
    if (state == PEDAL_STATE_FIND_MOVE_FORCE)   return "FIND_MOVE_FORCE";
    if (state == PEDAL_STATE_FIND_MIN)          return "FIND_MIN";
    if (state == PEDAL_STATE_FIND_MAX)          return "FIND_MAX";
    if (state == PEDAL_STATE_FIND_MIN_FORCE)    return "FIND_MIN_FORCE";
    return "unknown pedal state";
}


const char *moveStateName(int move_state)
{
    static char buf[200];
    buf[0] = 0;

    if (move_state == MOVE_STATE_NONE)
        return "NONE";
    if (move_state & MOVE_STATE_MOVING)
        sprintf(&buf[strlen(buf)]," MOVING");
    if (move_state & MOVE_STATE_STALLED)
        sprintf(&buf[strlen(buf)]," STALLED");
    if (move_state & MOVE_STATE_STOPPING)
        sprintf(&buf[strlen(buf)]," STOPPING");
    if (move_state & MOVE_STATE_STOPPED)
        sprintf(&buf[strlen(buf)]," STOPPED");
    return buf;
}


void autoPedal::setState(int state)
{
    display(dbg_pedal,"autoPedal::setState(%d) %s",state,pedalStateName(state));
    m_state = state;
}

void autoPedal::setMoveState(int move_state)
{
    display(dbg_pedal,"autoPedal::setMoveState(0x%04x) %s",move_state,moveStateName(move_state));
    m_move_state = move_state;
}

void autoPedal::addMoveState(int move_state)
{
    display(dbg_pedal,"autoPedal::addMoveState(0x%04x) %s",move_state,moveStateName(move_state));
    m_move_state |= move_state;
    display(dbg_pedal,"   new_state(0x%04x) %s",m_move_state,moveStateName(m_move_state));
}

void autoPedal::removeMoveState(int move_state)
{
    display(dbg_pedal,"autoPedal::removeMoveState(0x%04x) %s",move_state,moveStateName(move_state));
    m_move_state &= ~move_state;
    display(dbg_pedal,"   new_state(0x%04x) %s",m_state,moveStateName(m_move_state));
}



void autoPedal::stopMove()
{
    uint32_t elapsed = (micros() - m_start_time) / 1000;
    display(dbg_pedal,"stopMove ms(%lu) ma=%d  m_value=%d  user(%d)",
        elapsed,theAmpMeter.milliAmps(),m_value,getValue());

    m_direction = 0;
    m_force = 0;
    m_last_value = -1;
    analogWrite(m_out_pin1,0);
    analogWrite(m_out_pin2,0);

    addMoveState(MOVE_STATE_STOPPING);
}


void autoPedal::startMove(int direction, int force)
{
    display(dbg_pedal,"startMove() dir=%d force=%d m_value=%d  user(%d)",
        direction,force,m_value,getValue());

    m_move_state  = 0;
    m_num_steps   = 0;
    m_start_time  = 0;
    m_settle_time = 0;

    m_last_value     = -1;
    m_start_time     = micros();
    m_starting_value = m_value;

    move(direction,force);
}


void autoPedal::move(int direction, int force)
{
    m_direction = direction;
    m_force     = force;

    if (m_direction > 0)
    {
        analogWrite(m_out_pin1,m_force);
        analogWrite(m_out_pin2,0);
    }
    else if (m_direction < 0)
    {
        analogWrite(m_out_pin1,0);
        analogWrite(m_out_pin2,m_force);
    }
    else
    {
        analogWrite(m_out_pin1,0);
        analogWrite(m_out_pin2,0);
    }
}



//---------------------------------------------
// client API
//---------------------------------------------

int autoPedal::getValue()
{
    int ret_val = map(m_value,m_calib_min,m_calib_max,0,127);
    if (ret_val < 0)   ret_val = 0;
    if (ret_val > 127) ret_val = 127;
    return ret_val;
}


bool autoPedal::factoryCalibrate()
{
    if (!pedalAvailable())
        return false;
    setState(PEDAL_STATE_FACTORY_CALIB);
    return true;
}



//----------------------------------
// task
//----------------------------------

void autoPedal::task()
{
    theAmpMeter.task();
    static bool overload_noted = false;
    if (theAmpMeter.overload())
    {
        if (!overload_noted)
        {
            analogWrite(m_out_pin1,0);
            analogWrite(m_out_pin2,0);
            warning(0,"overload noted in autoPedal::task()",0);
            overload_noted = true;
        }
        return;
    }

    uint32_t now = micros();
    if (m_sample_count && now >= m_last_calc_time + MICROS_PER_PEDAL_CALC)
    {
        m_last_calc_time = now;
        m_value =  m_sample_total / m_sample_count;
        display(dbg_pedal+1,"pedal_value=%d",m_value);
        m_sample_count = 0;
        m_sample_total = 0;
        stateMachine();
    }

    if (now >= m_last_sample_time + MICROS_PER_PEDAL_SAMPLE)
    {
        m_last_sample_time = now;
        m_sample_count++;
        m_sample_total += analogRead(m_in_pin);
    }
}


//----------------------------------
// stateMachine()
//----------------------------------


bool autoPedal::setValue(int value)
{
    if (!pedalAvailable())
    {
        warning(0,"PEDAL UNAVAILABLE IN SETVALUE CALL",0);
        return false;
    }
    int cur_value = getValue();
    if (value == cur_value)
    {
        warning(0,"setValue() - already at value(%d)",value);
    }
    else
    {
        m_desired_value = map(value,0,127,m_calib_min,m_calib_max);
        m_range = m_desired_value - m_value;
        warning(0,"setValue(%d==%d) cur(%d==%d) range=%d",
            value,
            m_desired_value,
            cur_value,
            m_value,
            m_range);

        setState(PEDAL_STATE_MOVE);
        startMove(value < cur_value?-1:1,FORCE_START_MOVE);
    }
    return true;
}



void autoPedal::stateMachine()
{
    m_num_steps++;

    //---------------------------------
    // VALUE CHANGE
    //---------------------------------

    if (m_state)
    {
        if (m_last_value != -1)
        {
            if (// !theAmpMeter.milliAmps() &&
                !(m_move_state & MOVE_STATE_MOVING) && (
                (m_value < m_last_value - PEDAL_CHANGE_THRESHOLD ||
                 m_value > m_last_value + PEDAL_CHANGE_THRESHOLD)))
            {
                addMoveState(MOVE_STATE_MOVING);
            }

            m_velocity = m_value - m_last_value;

            if (m_state == PEDAL_STATE_MOVE)
            {
                int remain = m_desired_value - m_value;
                uint32_t ms = (micros() - m_start_time)/1000;
                display(dbg_moves,"ms(%lu) d(%d) f(%d) cur(%d) last(%d) vel(%d) remain(%d) user(%d)",
                    ms,
                    m_direction,
                    m_force,
                    m_value,
                    m_last_value,
                    m_velocity,
                    remain,
                    getValue());
            }
        }
    }


    //==============================================================
    // state machine
    //==============================================================
    // API moves

    if (m_state == PEDAL_STATE_MOVE)
    {
        if (m_move_state & MOVE_STATE_STOPPED)
        {
            if (!m_settle_time)
            {
                m_settle_time = micros();
            }
            else
            {
                if (micros() > m_settle_time + MOVE_SETTLE_TIME)
                {
                    int m1 = map(m_desired_value,m_calib_min,m_calib_max,0,127);
                    int m2 = map(m_value,m_calib_min,m_calib_max,0,127);
                        // funky behavior in display() with macro? map ...
                    uint32_t elapsed = (micros() - m_start_time) / 1009;
                    warning(dbg_moves,"move finished desired(%d) u(%d) actual(%d) u(%d) v(%d)",
                        m_desired_value,
                        m1,
                        m_value,
                        m2);
                    warning(0,"num_steps(%d) in %lu ms",m_num_steps,elapsed);
                    setState(PEDAL_STATE_NONE);
                    setMoveState(MOVE_STATE_NONE);
                }
            }
        }
        else if (!(m_move_state & MOVE_STATE_STOPPING) && (
            (m_range > 0 && m_value >= m_desired_value) ||
            (m_range < 0 && m_value <= m_desired_value)))
        {
            display(dbg_moves,"stopping m_value(%d) m_desired_value(%d) m_range(%d)",
                m_value,
                m_desired_value,
                m_range);
            stopMove();
        }
        else if (micros() > m_start_time + MOVE_TIMEOUT)
        {
            stopMove();
            setState(PEDAL_STATE_NONE);
            setMoveState(MOVE_STATE_NONE);
            my_error("Move timed out!",0);
            return;
        }
        else if (m_move_state & MOVE_STATE_MOVING)
        {
            int dif = abs(m_desired_value - m_value);
            if (dif < 120 || abs(m_velocity)>8)
            {
                move(m_direction,dif < 30 ?
                    FORCE_SLOW_MOVE :
                    FORCE_CONTINUE_MOVE);
            }
        }
    }

    //------------------------------------
    // factory calibration
    //------------------------------------

    else if (m_state == PEDAL_STATE_FACTORY_CALIB)
    {
        display(dbg_factory_calib,"starting factory calibration",0);
        writeEPROM16(EEPROM_CALIB_MIN,-1,"init_calib_min");
        writeEPROM16(EEPROM_CALIB_MAX,-1,"init_calib_max");

        if (m_value)    // if not on zero, move backwards to min first
        {
            display(dbg_factory_calib,"factory calibration - min",0);
            m_factory_calib_state = FACTORY_CALIB_MIN;
            setState(PEDAL_STATE_FIND_MIN);
            startMove(-1,FORCE_START_MOVE);
        }
        else            // otherwise, move forward to max first
        {
            display(dbg_factory_calib,"factory calibration - max",0);
            m_factory_calib_state = FACTORY_CALIB_MAX;
            setState(PEDAL_STATE_FIND_MAX);
            startMove(1,FORCE_START_MOVE);
        }
    }

    // factory calibration - initial min and max

    else if (m_state == PEDAL_STATE_FIND_MIN ||
             m_state == PEDAL_STATE_FIND_MAX)
    {
        // if at 0 or 1023, don't rely on stalling ...
        if (0 && !(m_move_state & MOVE_STATE_STOPPING))
        {
            if ((m_state == PEDAL_STATE_FIND_MIN && m_value == 0) ||
                (m_state == PEDAL_STATE_FIND_MAX && m_value == 1023))
                    stopMove();
        }

        else if (m_move_state & MOVE_STATE_STOPPED)
        {
            if (!m_settle_time)
            {
                m_settle_time = micros();
            }
            else if (micros() > m_settle_time + MOVE_SETTLE_TIME)
            {
                warning(dbg_factory_calib,"calibration %s=%d",
                    m_state == PEDAL_STATE_FIND_MIN ? "MIN" : "MAX",
                    m_value);
                if (m_state == PEDAL_STATE_FIND_MIN)
                {
                    m_calib_min = m_value;
                    if (m_calib_min < 1023-FACTORY_CALIB_MIN_SAFETY_MARGIN)
                        m_calib_min += FACTORY_CALIB_MIN_SAFETY_MARGIN;
                    writeEPROM16(EEPROM_CALIB_MIN,m_calib_min,"calib_min");
                }
                else
                {
                    m_calib_max = m_value;
                    if (m_calib_max > FACTORY_CALIB_MAX_SAFETY_MARGIN)
                        m_calib_max -= FACTORY_CALIB_MAX_SAFETY_MARGIN;
                    writeEPROM16(EEPROM_CALIB_MAX,m_calib_max,"calib_max");
                }

                if (!(m_factory_calib_state & FACTORY_CALIB_MIN))
                {
                    display(dbg_factory_calib,"factory calibration - min",0);
                    m_factory_calib_state |= FACTORY_CALIB_MIN;
                    setState(PEDAL_STATE_FIND_MIN);
                    startMove(-1,FORCE_START_MOVE);
                }
                else if (!(m_factory_calib_state & FACTORY_CALIB_MAX))
                {
                    display(dbg_factory_calib,"factory calibration - max",0);
                    m_factory_calib_state |= FACTORY_CALIB_MAX;
                    setState(PEDAL_STATE_FIND_MAX);
                    startMove(1,FORCE_START_MOVE);
                }
                else
                {
                    stopMove();
                    setState(PEDAL_STATE_NONE);
                    setMoveState(MOVE_STATE_NONE);
                    m_factory_calib_state = FACTORY_CALIB_NONE;
                    display(dbg_factory_calib,"factory calibration finished",0);
                }
            }
        }
        else if (micros() > m_start_time + MOVE_TIMEOUT)
        {
            int ma = theAmpMeter.milliAmps();
            int average = theAmpMeter.averageMilliAmps();
            int value = m_value;

            stopMove();
            setState(PEDAL_STATE_NONE);
            setMoveState(MOVE_STATE_NONE);
            m_factory_calib_state = FACTORY_CALIB_NONE;
            my_error("Move timed out!  m_value=%d ma=%d average_ma=%d",value,ma,average);
            return;
        }
    }

    m_last_value = m_value;


    //---------------------------------
    // check for stall
    //---------------------------------

    if ((m_move_state & MOVE_STATE_MOVING) &&
        !(m_move_state & MOVE_STATE_STALLED) &&
        (theAmpMeter.milliAmps() > STALLED_MILLIAMPS))
    {
        display(dbg_pedal,"stalled ma=%d (average=%d)",
            theAmpMeter.milliAmps(),
            theAmpMeter.averageMilliAmps());
        addMoveState(MOVE_STATE_STALLED);
    }

    // promote stall to stopping

    if (m_move_state & MOVE_STATE_STALLED &&
       !(m_move_state & MOVE_STATE_STOPPING))
    {
        display(dbg_pedal,"pedal stalled - stopping it",0);
        stopMove();
    }

    // promote stopping to stopped

    if ((m_move_state & MOVE_STATE_STOPPING) &&
        !(m_move_state & MOVE_STATE_STOPPED))
        // && !theAmpMeter.milliAmps())
    {
        display(dbg_pedal,"pedal atopped",0);
        addMoveState(MOVE_STATE_STOPPED);
    }


}
