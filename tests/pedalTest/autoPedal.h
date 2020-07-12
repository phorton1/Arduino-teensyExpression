#ifndef __autoPedal_h__
#define __autoPedal_h__

#define MICROS_PER_PEDAL_SAMPLE    200      // how often to sample the pedal pot - try at 5khz
#define PEDAL_SAMPLES_PER_CALC     5        // over sampling rate, this many samples are averaged for each calc

#define MICROS_PER_PEDAL_CALC     (PEDAL_SAMPLES_PER_CALC * MICROS_PER_PEDAL_SAMPLE)


#define PEDAL_STATE_NONE                0      // pedal is free
#define PEDAL_STATE_MOVE                1      // pedal is executing a user setValue() call
#define PEDAL_STATE_FACTORY_CALIB       2     // factory calibration starting
#define PEDAL_STATE_FIND_START_FORCE    3     // determine minimum force to move pedal from initial position
#define PEDAL_STATE_FIND_MOVE_FORCE     4     // determine minimum force to keep pedal moving till stops
#define PEDAL_STATE_FIND_MIN            5      // moving backwards towards zero, get value at stall
#define PEDAL_STATE_FIND_MAX            6      // moving forwards towards max, get value at stall
#define PEDAL_STATE_FIND_MIN_FORCE      7      // moving forwards towards max, get value at stall

#define MOVE_STATE_NONE             0             // pedal is free
#define MOVE_STATE_MOVING           0x0001        // the pedal is moving
#define MOVE_STATE_STALLED          0x0002        // we are stopping the pedal
#define MOVE_STATE_STOPPING         0x0004        // we are stopping the pedal
#define MOVE_STATE_STOPPED          0x0008        // it becomes free AFTER it is stopped for one calc


class autoPedal
{
public:

    autoPedal(int in_pin, int out_pin1, int out_pin2);
        // benign-ish ctor

    void start();
    void clear();

    void task();

    int getValue();    // 0..127
    int getRawValue()  { return m_value; }    // 0..1023
    int getCalibMin();
    int getCalibMax();

    // methods return false if the pedal is active

    bool pedalAvailable();
    bool factoryCalibrate();
    bool setValue(int value);  // 0..127


private:

    // constructon params

    int m_in_pin;
    int m_out_pin1;
    int m_out_pin2;

    // sample measurement variables

    int m_value;               // most recent 0..1023 averaged value and addition to circular buffer
    int m_calib_min;
    int m_calib_max;
    int m_sample_count;
    int m_sample_total;

    uint32_t m_last_calc_time;
    uint32_t m_last_sample_time;

    // pedal state machine

    int m_state;                // state of the pedal (enum)
    int m_move_state;           // state of the pedal (bitwise)
    int m_factory_calib_state;  // sub state of factory calibration

    int m_force;
    int m_direction;
    int m_last_value;

    int m_range;
    int m_desired_value;
    int m_starting_value;
    int m_velocity;

    int m_num_steps;
    uint32_t m_start_time;
    uint32_t m_settle_time;

    // methods

    void stateMachine();
    void setState(int state);

    void stopMove();
    void move(int direction, int force);
    void startMove(int direction, int force);

    void setMoveState(int move_state);
    void addMoveState(int move_state);
    void removeMoveState(int move_state);

};


extern autoPedal thePedal;


#endif // !__autoPedal_h__
