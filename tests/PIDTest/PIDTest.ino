#include <myDebug.h>

#define DATA_OUT_PIN     5
#define DATA_IN_PIN      3
    // The nano only supports pin interrupts on pins 2&3
    // we need the other one (pin 2) for the software serial
    // in ampMeter.cpp

#define JUST_COMMS   0
#define MOVE_PEDAL_TO_RECEIVED_BYTE   1


#if !JUST_COMMS
    #include "ampMeter.h"

    #define PEDAL_IN_PIN                A7
    #define PEDAL_OUT_PIN1              9     // PWM output to Motor Controller (L293, L9110 or similar)
    #define PEDAL_OUT_PIN2              10

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

        if (output < OUTPUT_THRESHOLD &&
            abs(err) < SUCCESS_THRESHOLD)
        {
            finish_count++;
            if (finish_count > 5)
            {
                reset();
                int v = analogRead(PEDAL_IN_PIN);
                display(0,"finish value=%d user(%d)",v,map(v,0,1023,0,127));
            }
        }
        else
            finish_count = 0;

        last_err = err;
        last_time = now;
    }



    void reset()
    {
        running = false;

        input = 0;
        setpoint = 0;
        output = 0;
        err_sum = 0;
        last_err = 0;
        last_time = 0;
        finish_count = 0;
        move_time = 0;
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

void movePedalTo(int32_t value)
{
    if (value > 1023)
        value = 1023;

    theAmpMeter.clearOverload();
    overload_noted = 0;
    setpoint = value;

    input = analogRead(PEDAL_IN_PIN);
    last_time = millis();

    display(0,"MOVE FROM %ld TO %ld  value=%d",input,setpoint,value);
    running = 1;
    move_time = millis();
}



void arduinoReceiveByte()
    // quick and dirty, timings derived empirically to
    // match the teensy code's arbitrary constants ..
    // No matter what I tried, this routine gets called twice for
    //    every byte sent by the teensy ... it gets another byte
    //    full of zeros with a zero stop bit.
{
    if (data_dir)
        return;
    delayMicroseconds(120);
    int value = 0;
    for (int i=0; i<8; i++)
    {
        value = (value << 1) | digitalRead(DATA_IN_PIN);
        delayMicroseconds(100);
    }
    int stop_bit = digitalRead(DATA_IN_PIN);
    if (stop_bit)
    {
        display(0,"ARDUINO RECEIVED byte=0x%02x  stop=%d",value,stop_bit);
        #if MOVE_PEDAL_TO_RECEIVED_BYTE
            value = map(value,0,127,0,1023);
            movePedalTo(value);
        #endif
    }
}


void arduinoSendByte(int byte)
{
    data_dir = 1;
    display(0,"arduinoSendByte(0x%02x)",byte);
    digitalWrite(DATA_OUT_PIN,1);        // start bit
    delayMicroseconds(80);
    for (int i=0; i<8; i++)
    {
        digitalWrite(DATA_OUT_PIN,(byte >> (7-i)) & 0x01);      // MSb first
        delayMicroseconds(80);
    }
    digitalWrite(DATA_OUT_PIN,1);        // stop bit
    delayMicroseconds(100);
    digitalWrite(DATA_OUT_PIN,0);        // finished
    delayMicroseconds(100);              // delay before enabling our input interrupt
    data_dir = 0;
}



int last_in_value = 0;



void loop()
{
    #if !JUST_COMMS
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
            int value = analogRead(PEDAL_IN_PIN);
            display(0,"current value=%d  user(%d)",value,map(value,0,1023,0,127));
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
    // motor
    //-----------------------------------

    if (running)
    {
        if (move_time && millis() > move_time + MOVE_TIMEOUT)
        {
            my_error("MOVE TIMED OUT",0);
            reset();
            return;
        }

        input = analogRead(PEDAL_IN_PIN);
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
#endif

}   // loop()
