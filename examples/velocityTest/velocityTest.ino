#include <myDebug.h>
#include "ampMeter.h"

#define MOVE_PEDAL_TO_RECEIVED_BYTE   1


#define PEDAL_IN_PIN                A7      // pedal poteniometer
#define PEDAL_OUT_PIN1              9       // PWM output to Motor Controller (L293, L9110 or similar)
#define PEDAL_OUT_PIN2              10
#define DATA_OUT_PIN                5       // digital output to my kludge level shifter
#define DATA_IN_PIN                 3       // digital input frommy kludge level shifter
    // The DATA_IN_PIN is set for a rising interrupt.
    // The nano only supports pin interrupts on pins 2&3.
    // We need the other one (pin 2) for the software serial
    // in ampMeter.cpp


//------------------------------
// setup()
//------------------------------


void setup()
{
    pinMode(13,OUTPUT);

    Serial.begin(115200);
    uint32_t timeout = millis();
    while (!Serial && millis() < timeout + 1000) {}
    display(0,"PIDTest started",0);

    digitalWrite(DATA_IN_PIN,0);
    pinMode(DATA_IN_PIN,INPUT);     // crucial that this pin NEVER goes high
    attachInterrupt(digitalPinToInterrupt(DATA_IN_PIN), arduinoReceiveByte, RISING );

    pinMode(DATA_OUT_PIN,OUTPUT);
    digitalWrite(DATA_OUT_PIN,0);

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
}


//---------------------------------------------------
// kludge 1-wire comms
//---------------------------------------------------

volatile int data_dir = 1;
    // set to 1 while we are writing, so that any interrupts
    // received while it is high are ignored


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



//-----------------------------------------------
// control
//-----------------------------------------------

bool overload_noted = 0;

int cur_pedal_sample = 0;
int pedal_read_count = 0;
int last_pedal_sample = 0;

int move_range = 0;
int move_to_value = 0;
int move_start_value = 0;
uint32_t move_start_time = 0;
int move_last_value = 0;
uint32_t move_last_time = 0;

int force = 0;
int direction = 0;


void reset()
{
    cur_pedal_sample = 0;
    pedal_read_count = 0;
    last_pedal_sample = -1;

    move_range = 0;
    move_to_value = 0;
    move_start_value = 0;
    move_start_time = 0;
    move_last_value = 0;
    move_last_time = 0;

    force = 0;
    direction = 0;

    display(0,"reset",0);
}





//------------------------------
// loop()
//------------------------------
// there's a bump in the middle that requires more force to get over
// it appears to require about 10ms at 255 (5V) to generally, but not always, register a move

#define MOVE_TIMEOUT  3000000
#define PEDAL_READS_PER_SAMPLE   10

#define INITIAL_MOVE_FORCE  255
#define INITIAL_MOVE_TIME   10000

#define CONTINUE_MOVE_FORCE  220

#define STATIC_MOVE_FORCE    100


void calculateMove()
{
    uint32_t now = micros();
    uint32_t elapsed = now - move_start_time;


    int dx = last_pedal_sample - move_last_value;
    uint32_t dt = now - move_last_time;
    uint32_t remain = abs(move_to_value - last_pedal_sample);
    uint32_t moved = abs(last_pedal_sample - move_start_value);

    if (elapsed > INITIAL_MOVE_TIME)
    {
        if (remain < move_range / 5)
            force = map(remain,0,move_range/5,STATIC_MOVE_FORCE,CONTINUE_MOVE_FORCE);
        else
            force = CONTINUE_MOVE_FORCE;
    }

    // if (remain < 20)
    //    force = 0;

#if 1


    display(0,"t(%lu) dt(%lu) x(%d) dx(%d) remain(%d) d(%lu) f(%d)  user(%d)",
        elapsed,
        dt,
        last_pedal_sample,
        dx,
        remain,
        direction,
        force,
        map(last_pedal_sample,0,1023,0,127));

#endif


    if (elapsed > MOVE_TIMEOUT)
    {
        warning(0,"move timed out ms(%lu) value=%d",elapsed/1000,last_pedal_sample);
        reset();
        return;
    }

    if ((direction < 0 && last_pedal_sample <= move_to_value) ||
        (direction > 0 && last_pedal_sample >= move_to_value))
    {
        warning(0,"arrived ms(%lu) value=%d",elapsed/1000,last_pedal_sample);
        reset();
        return;
    }

    move_last_time = now;
    move_last_value = last_pedal_sample;

}


void movePedalTo(int32_t value)
{
    if (value > 1023)
        value = 1023;
    if (value < 0)
        value = 0;
    if (value == last_pedal_sample)
    {
        warning(0,"NO MOVE.  Pedal already at %d",value);
        return;
    }

    theAmpMeter.clearOverload();
    overload_noted = 0;

    move_to_value = value;
    move_start_value = last_pedal_sample;
    move_last_value = move_start_value;
    move_range = abs(move_to_value - move_start_value);

    warning(0,"MOVE FROM %d TO %d",move_start_value,move_to_value);

    force = INITIAL_MOVE_FORCE;
    direction = move_to_value > move_start_value ? 1 : -1;
    move_start_time = micros();
    move_last_time = move_start_time;
}




void loop()
{
    #if 1
        theAmpMeter.task();
        if (!overload_noted && theAmpMeter.overload())
        {
            overload_noted = 1;
            warning(0,"overload noted",0);
            reset();
        }
    #endif

    #if 1
        static bool blink_state = 0;
        static uint32_t blink_millis = 0;
        uint32_t now_millis = millis();
        if (now_millis > blink_millis + 500)
        {
            blink_state = !blink_state;
            digitalWrite(13,blink_state);
            blink_millis = now_millis;
        }
    #endif

    //----------------------------
    // Serial port
    //----------------------------

    if (Serial.available())
    {
        char c = Serial.read();
        static int in_point = 0;

        if (c == 'b')
        {
            static int byte_val = 0xAA;
            arduinoSendByte(byte_val++);
            if (byte_val > 255) byte_val = 0;
        }
        else if (c == 27)
        {
            Serial.println();
            reset();
            in_point = 0;
        }
        else if (c == 'v')
        {
            display(0,"value(%d) user(%d)",last_pedal_sample,map(last_pedal_sample,0,1023,0,127));
            in_point = 0;
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
        else if (c == '+')
        {
            in_point = 0;
            movePedalTo(last_pedal_sample + 1);
        }
        else if (c == '-')
        {
            in_point = 0;
            movePedalTo(last_pedal_sample - 1);
        }
    }


    //-----------------------------------
    // sample the pedal
    //-----------------------------------

    pedal_read_count++;
    cur_pedal_sample += analogRead(PEDAL_IN_PIN);
    if (pedal_read_count >= PEDAL_READS_PER_SAMPLE)
    {
        pedal_read_count = 0;
        last_pedal_sample = (cur_pedal_sample + PEDAL_READS_PER_SAMPLE/2) / PEDAL_READS_PER_SAMPLE;
        cur_pedal_sample = 0;
        if (move_start_time)
            calculateMove();
    }

    //---------------------------------
    // motor
    //---------------------------------

    static int last_force = 0;
    static int last_direction = 0;
    if (force != last_force || direction != last_direction)
    {
        last_force = force;
        last_direction = direction;
        int force_dir = force * direction;

        if (force_dir > 0)
        {
            analogWrite(PEDAL_OUT_PIN1,abs(force));
            analogWrite(PEDAL_OUT_PIN2,0);
        }
        else if (force_dir < 0)
        {
            analogWrite(PEDAL_OUT_PIN1,0);
            analogWrite(PEDAL_OUT_PIN2,force);
        }
        else
        {
            analogWrite(PEDAL_OUT_PIN1,0);
            analogWrite(PEDAL_OUT_PIN2,0);
        }

    }   // force or direction changed

}   // loop()
