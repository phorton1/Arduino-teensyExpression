#include <myDebug.h>

// 2020-07-13   This is the best solution to moving the pedal so far.
//
// Note that the pedal is geared 50:1 on a 12V 3000RPM motor, and
// 26.5 degrees = 0.736 revolutions, the main gear is 80, so
// 5.88 teeth are moving past the motor gear, which has 12 teeth,
// and turns just about 1/2 revolution.    Thus for the entire
// control range, I am turning the inside motor about 25 times.
//
// Even with "user" values, I am trying to control down the inside
// motor down to 1/5th of a revolution, an to achieve 0..1023 accuracy,
// I would have to control the motor to 25/1024 == 1/40th of a revolution.
//
// This is clearly not a good design.  Worm gear with clutch is how
// they do it in the big leagues, and it would be slow. Another idea
// I've played with is dueling solenoids.   One way or the other,
// it's a thorny issue.
//
// I'm checking this in with the idea that when the teensyExpression box
// wants to change the value, the arduino stop sending continuous control
// output messages, and sends a confirmation, of sorts, when it is done
// moving.  The teensy can far better control scaled values and things
// like overshoot mean that you definitely DONT want the live output
// of a mechanized move connected to a live audio output.


#define DATA_OUT_PIN     5
#define DATA_IN_PIN      3
    // The nano only supports pin interrupts on pins 2&3
    // we need the other one (pin 2) for the software serial
    // in ampMeter.cpp

#define JUST_COMMS   0
#define MOVE_PEDAL_TO_RECEIVED_BYTE   1
#define SEND_CONTINUOUS_CONTROL       1
#define CONTROL_SEND_TIME             10     // no more than once every 50 ms


#if !JUST_COMMS
    #include "ampMeter.h"

    #define PEDAL_IN_PIN                A7
    #define PEDAL_OUT_PIN1              10     // PWM output to Motor Controller (L293, L9110 or similar)
    #define PEDAL_OUT_PIN2              9

    #define PEDAL_READS_PER_SAMPLE      10

    int cur_sample = 0;
    int working_sample = 0;
    int num_pedal_reads = 0;

    int last_send_value = 0;
    uint32_t last_send_time = 0;

    #define MAX_OUTPUT   160
    #define MIN_OUTPUT   80
    #define OUTPUT_THRESHOLD  20
    #define SUCCESS_THRESHOLD 7

    float kp = 1.2;
    float ki = 0.004;
    float kd = 4;

    bool running = false;
    int in_point = 0;

    int32_t input = 0;
    int32_t setpoint = 0;
    int32_t output = 0;
    int32_t err_sum = 0;
    int32_t last_err = 0;
    int32_t last_time = 0;
    int finish_count = 0;
    bool overload_noted = 0;
    uint32_t move_time = 0;
    int32_t move_start = 0;
    #define MOVE_TIMEOUT    2000


    void computePID()
    {
        int32_t now = millis();
        if (now < last_time + 2)
            return;

        int32_t delta_time = now - last_time;
        int32_t err = setpoint - input;
        err_sum += (err * delta_time);
        float delta_err = (((float)err) - ((float)last_err)) / ((float)delta_time);

        float calc = kp * ((float)err) + ki * ((float)err_sum) + kd * delta_err;
        output = calc;

        #if 0
            display(0,"dt=%ld in=%ld err=%ld esum=%ld derr=%ld output=%ld",
                delta_time,
                input,
                err,
                err_sum,
                ((int32_t)delta_err*100),
                output);
        #endif

        if (output > MAX_OUTPUT) output = MAX_OUTPUT;
        if (output > OUTPUT_THRESHOLD  && output < MIN_OUTPUT) output = MIN_OUTPUT;
        if (output < -MAX_OUTPUT) output = -MAX_OUTPUT;
        if (output < -OUTPUT_THRESHOLD  && output > -MIN_OUTPUT) output = -MIN_OUTPUT;

        #define MIN_MOVE_POSITION  6
        #define MAX_MOVE_POSITION  1017

        if ((setpoint <= MIN_MOVE_POSITION || setpoint >= MAX_MOVE_POSITION) && (
           (setpoint>move_start && input>=MAX_MOVE_POSITION) ||
           (setpoint<move_start && input<=MIN_MOVE_POSITION)))
        {
                reset();
                display(0,"stop finish value=%d user(%d)",cur_sample,map(cur_sample,0,1023,0,127));
                return;
        }

        if (output < OUTPUT_THRESHOLD &&
            abs(err) < SUCCESS_THRESHOLD)
        {
            finish_count++;
            if (finish_count > 5)
            {
                reset();
                display(0,"finish value=%d user(%d)",cur_sample,map(cur_sample,0,1023,0,127));
                return;
            }
        }
        else
            finish_count = 0;

        last_err = err;
        last_time = now;
    }



    void reset()
    {
        cur_sample = 0;
        working_sample = 0;
        num_pedal_reads = 0;
        last_send_value = -1;
        last_send_time = millis();

        running = false;

        input = 0;
        setpoint = 0;
        output = 0;
        err_sum = 0;
        last_err = 0;
        last_time = 0;
        finish_count = 0;
        move_time = 0;
        move_start = 0;
        analogWrite(PEDAL_OUT_PIN1,0);
        analogWrite(PEDAL_OUT_PIN2,0);
        display(0,"reset",0);
    }
#endif




//------------------------------
// setup()
//------------------------------

int byte_val = 0xAA;
volatile int data_dir = 1;
    // set to 1 while we are writing, so that any interrupts
    // received while it is high are ignored


void setup()
{
    Serial.begin(115200);
    uint32_t timeout = millis();
    while (!Serial && millis() < timeout + 1000) {}
    display(0,"PIDTest started",0);


    digitalWrite(DATA_IN_PIN,0);
    pinMode(DATA_IN_PIN,INPUT);     // crucial that this pin NEVER goes high
    attachInterrupt(digitalPinToInterrupt(DATA_IN_PIN), arduinoReceiveByte, RISING );
    data_dir = 0;   // our disable interrupts

    pinMode(DATA_OUT_PIN,OUTPUT);
    digitalWrite(DATA_OUT_PIN,0);


    #if !JUST_COMMS

        // Increase pin 9 & 10 clock PWM clock rate, resulting in
        // smoother movements and more predictable behavior.

        TCCR1B = TCCR1B & B11111000 | B00000010;
            // for PWM frequency of 3921.16 Hz


        pinMode(PEDAL_IN_PIN,INPUT);
        pinMode(PEDAL_OUT_PIN1,OUTPUT);
        pinMode(PEDAL_OUT_PIN2,OUTPUT);
        analogWrite(PEDAL_OUT_PIN1,0);
        analogWrite(PEDAL_OUT_PIN2,0);

        reset();
    #endif
}



//------------------------------
// loop()
//------------------------------

#if !JUST_COMMS
    void movePedalTo(int32_t value)
    {
        if (value > 1023)
            value = 1023;

        theAmpMeter.clearOverload();
        overload_noted = 0;
        setpoint = value;

        input = cur_sample;
        last_time = millis();

        display(0,"MOVE FROM %ld TO %ld  value=%d",input,setpoint,value);
        running = 1;
        move_start = input;
        move_time = millis();
    }
#endif

#define ARDUINO_DELAY            100
#define ARDUINO_START_IN_DELAY   (6 * ARDUINO_DELAY / 5)
#define ARDUINO_OUT_DELAY        (4 * ARDUINO_DELAY / 5)


void arduinoReceiveByte()
    // quick and dirty, timings derived empirically to
    // match the teensy code's arbitrary constants ..
    // No matter what I tried, this routine gets called twice for
    //    every byte sent by the teensy ... it gets another byte
    //    full of zeros with a zero stop bit.
{
    if (data_dir)
        return;
    delayMicroseconds(ARDUINO_START_IN_DELAY);
    int value = 0;
    for (int i=0; i<8; i++)
    {
        value = (value << 1) | digitalRead(DATA_IN_PIN);
        delayMicroseconds(ARDUINO_DELAY);
    }
    int stop_bit = digitalRead(DATA_IN_PIN);
    if (stop_bit)
    {
        display(0,"ARDUINO RECEIVED byte=0x%02x  dec(%d)  stop=%d",value,value,stop_bit);
        #if MOVE_PEDAL_TO_RECEIVED_BYTE
            if (value == 0)
                value = 0;
            else if (value == 127)
                value = 1023;
            else
                value = map(value,0,127,0,1023);
            movePedalTo(value);
        #endif
    }
}


void arduinoSendByte(int byte)
{
    data_dir = 1;
    display(0,"arduinoSendByte(0x%02x) dec(%d)",byte,byte);
    digitalWrite(DATA_OUT_PIN,1);        // start bit
    delayMicroseconds(ARDUINO_OUT_DELAY);
    for (int i=0; i<8; i++)
    {
        digitalWrite(DATA_OUT_PIN,(byte >> (7-i)) & 0x01);      // MSb first
        delayMicroseconds(ARDUINO_OUT_DELAY);
    }
    digitalWrite(DATA_OUT_PIN,1);        // stop bit
    delayMicroseconds(ARDUINO_DELAY);
    digitalWrite(DATA_OUT_PIN,0);        // finished
    delayMicroseconds(ARDUINO_DELAY);              // delay before enabling our input interrupt
    data_dir = 0;
}




void loop()
{
    #if 0   // !JUST_COMMS
        theAmpMeter.task();
        if (!overload_noted && theAmpMeter.overload())
        {
            overload_noted = 1;
            warning(0,"overload noted",0);
            reset();
        }
    #endif

    //----------------------------
    // Serial port
    //----------------------------

    if (Serial.available())
    {
        char c = Serial.read();

        if (c == 'b')
        {
            arduinoSendByte(byte_val++);
            if (byte_val > 255) byte_val = 0;
        }

    #if !JUST_COMMS

        else if (c == 27)
        {
            Serial.println();
            reset();
        }
        else if (c == 'v')
        {
            display(0,"current value=%d  user(%d)",cur_sample,map(cur_sample,0,1023,0,127));
        }
        else if (c == 13)
        {
            Serial.print(c);
            movePedalTo(in_point);
            in_point = 0;
        }
        else if (c >= '0' && c <= '9')
        {
            Serial.print(c);
            in_point = in_point * 10 + c - '0';
        }
        else if (c == 10)
        {
            Serial.print(c);
        }
    #endif
    }

#if !JUST_COMMS

    //-----------------------------------
    // pedal
    //-----------------------------------

    num_pedal_reads++;
    working_sample += analogRead(PEDAL_IN_PIN);
    if (num_pedal_reads >= PEDAL_READS_PER_SAMPLE)
    {
        cur_sample = (working_sample + PEDAL_READS_PER_SAMPLE/2) / PEDAL_READS_PER_SAMPLE;
        num_pedal_reads = 0;
        working_sample = 0;
    }

    //-----------------------------------
    // motor
    //-----------------------------------

    if (!num_pedal_reads)   // on each sample
    {
        if (running)
        {
            if (move_time && millis() > move_time + MOVE_TIMEOUT)
            {
                my_error("MOVE TIMED OUT",0);
                reset();
                return;
            }

            input = cur_sample;
            computePID();

            if (output > 0)
            {
                analogWrite(PEDAL_OUT_PIN1,output);
                analogWrite(PEDAL_OUT_PIN2,0);
            }
            else if (output < 0)
            {
                analogWrite(PEDAL_OUT_PIN1,0);
                analogWrite(PEDAL_OUT_PIN2,-output);
            }
            else
            {
                analogWrite(PEDAL_OUT_PIN1,0);
                analogWrite(PEDAL_OUT_PIN2,0);
            }
        }
    #if SEND_CONTINUOUS_CONTROL
        else if (millis() > last_send_time + CONTROL_SEND_TIME)
        {
            int user = map(cur_sample,0,1023,0,127);
            if (user > 127) user = 127;
            if (user != last_send_value)
            {
                display(0,"sending cur_sample(%d) user(%d)",cur_sample,user);
                last_send_value = user;
                arduinoSendByte(user);
            }
            last_send_time = millis();
        }
    #endif
    }



#endif

}   // loop()
