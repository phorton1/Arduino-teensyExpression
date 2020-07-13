// comTest
//
// Runs on a teensy3.6 to communicate with the pedal Arduino,
// which in turn uses 2 pins, a voltage divider, a diode,
// and an explicit pull-down resister.
//
// The teensy receives a bogus message each time the arduino (Pedal/PIDTest)
// is started.  This *may* require a careful powerup sequence of the
// teensyExpression box with these "pedals" attached, in real life.
//
// See PIDTest.ino for the Arduino side at this time.

#include <myDebug.h>

#define DATA_PIN              23
    // bi-directional


//------------------------------
// setup()
//------------------------------

void setup()
{
    Serial.begin(115200);
    uint32_t timeout = millis();
    while (!Serial && millis() < timeout + 1000) {}
    display(0,"comTest started",0);
    pinMode(DATA_PIN,INPUT);
    attachInterrupt(digitalPinToInterrupt(DATA_PIN), teensyReceiveByte, RISING );

    pinMode(13,OUTPUT);
}


#define TEENSY_DELAY            100
#define TEENSY_START_IN_DELAY   (6 * TEENSY_DELAY / 5)
#define TEENSY_END_OUT_DELAY    (4 * TEENSY_DELAY / 5)


void teensyReceiveByte()
    // quick and dirty, timings derived empirically to
    // match the arduino code's arbitrary constants.
{
    delayMicroseconds(TEENSY_START_IN_DELAY);
    int value = 0;
    for (int i=0; i<8; i++)
    {
        value = (value << 1) | digitalRead(DATA_PIN);
        delayMicroseconds(TEENSY_DELAY);
    }
    int stop_bit = digitalRead(DATA_PIN);
    digitalWrite(DATA_PIN,0);
        // this appeared to be needed to drive the signal low
        // or else a 2nd interrupt was always triggered
    display(0,"TEENSY RECEIVED byte=0x%02x  stop=%d",value,stop_bit);
}


void teensySendByte(int byte)
{
    display(0,"teensySendByte(0x%02x)",byte);
    pinMode(DATA_PIN,OUTPUT);
    digitalWrite(DATA_PIN,0);        // start bit
    delayMicroseconds(TEENSY_DELAY);
    digitalWrite(DATA_PIN,1);        // start bit
    delayMicroseconds(TEENSY_DELAY);

    for (int i=0; i<8; i++)
    {
        digitalWrite(DATA_PIN,(byte >> (7-i)) & 0x01);      // MSb first
        delayMicroseconds(TEENSY_DELAY);
    }
    digitalWrite(DATA_PIN,1);        // stop bit
    delayMicroseconds(TEENSY_END_OUT_DELAY);
    digitalWrite(DATA_PIN,0);        // finished

    pinMode(DATA_PIN,INPUT);
    attachInterrupt(digitalPinToInterrupt(DATA_PIN), teensyReceiveByte, RISING );
}




//------------------------------
// loop()
//------------------------------

int move_value = 0;


void loop()
{
    #define BLINK_ON_TIME 1
    #define BLINK_OFF_TIME 999

    static bool blink_value = 0;
    static elapsedMillis blink_time = 0;
    if ((blink_value && blink_time > BLINK_ON_TIME) ||
        (!blink_value && blink_time > BLINK_OFF_TIME))
    {
        blink_value = !blink_value;
        digitalWrite(13,blink_value);
        blink_time = 0;
    }

    if (Serial.available())
    {
        char c = Serial.read();

        if (c == 'b')
        {
            static int byte_val = 0xAA;
            teensySendByte(byte_val++);
            if (byte_val > 255) byte_val = 0;
        }
        else if (c == 13)
        {
            Serial.print(c);
            if (move_value > 127)
                move_value = 127;
            display(0,"MOVING PEDAL TO user(%d)",move_value);
            teensySendByte(move_value);
            move_value = 0;
        }
        else if (c >= '0' && c <= '9')
        {
            Serial.print(c);
            move_value = move_value * 10 + c - '0';

        }
        else if (c == 10)
        {
            Serial.print(c);
        }
    }

}   // loop()
