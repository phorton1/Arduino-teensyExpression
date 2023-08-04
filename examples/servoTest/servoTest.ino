#include <myDebug.h>
#include <Servo.h>


// new setup main gear has 50 teeth to move about 25 degrees, or
// 25/360=0.069444 revolutions.  The servo will see 3.47222 teeth
// move past, which, with a 12 tooth gear is about 0.2893 revolutions,
// or about 104 degrees of movement on the servo.

// The pot reads from about 60 to 650 over 180 degrees, or 590*2 per rev,
// so it will see about 0.2893*590 = 170 units of change across the movement
// range.

// Assuming the "center" of 90 degrees at 1.502 ms, and the 1856 us range given
// in Servo.h (544..2400) AND reversing the signs means that, over 180 degrees,
// that one degree is approximately 10.311 us.   Given that we the servo is moving over
// over +/- 52 degrees, this comes out to approximately (1502) +/- 536 us.  Since
// the minimum movement is in 4 us chunks, that's +/- 134 chunks.


#define PEDAL_IN_PIN                A7
#define SERVO_PWM_PIN               9

#define PEDAL_READS_PER_SAMPLE      10

int cur_sample = 0;
int working_sample = 0;
int num_pedal_reads = 0;


Servo servo;

// absolutes 0..180, seem to work well
// range from zero seems to work best (not centered)
// so I introduce constants for 104/180 of max ranges
// and they seem to work ok.
// Assemble the pedal with the pot at zero !!!

#define POT_MIN                 60
#define POT_180_MAX             650
#define POT_104_MAX             401

#define MICROS_MIN              544
#define MICROS_180_MAX          2400
#define MICROS_104_MAX          1616

// use first 104 degrees

int pedalToUser(int pedal)
{
    int user = map(pedal,POT_MIN,POT_104_MAX,0,127);
    if (user > 127) user=127;
    if (user < 0) user = 0;
    user = 127 - user;
    return user;
}


int userToServo(int user)
{
    user = 127 - user;
    int servo = map(user,0,127,MICROS_MIN,MICROS_104_MAX);
    if (servo > MICROS_104_MAX) servo = MICROS_104_MAX;
    if (servo < MICROS_MIN) servo = MICROS_MIN;
    display(0,"userToServo(%d)=%d",user,servo);
    return servo;
}



void setup()
{

    // Increase pin 9 & 10 clock PWM clock rate, resulting in
    // smoother movements and more predictable behavior.

    // TCCR1B = TCCR1B & B11111000 | B00000010;
        // for PWM frequency of 3921.16 Hz

    Serial.begin(115200);
    uint32_t timeout = millis();
    while (!Serial && millis() < timeout + 1000) {}
    display(0,"servoTest started",0);

    pinMode(PEDAL_IN_PIN,INPUT);
}



//------------------------------
// loop()
//------------------------------


void loop()
{
    static bool oscilate = 0;
    static bool osc_dir = 0;
    static uint32_t osc_time = 0;

    uint32_t now = millis();
    if (oscilate && now > osc_time + 1000)
    {
        osc_dir = !osc_dir;
        display(0,"osc_dir=%d",osc_dir);
        servo.write(userToServo(osc_dir ? 127 : 0));
        osc_time = millis();
    }

    //----------------------------
    // Serial port
    //----------------------------

    if (Serial.available())
    {
        static int position = 66;
        static bool enable = 0;

        char c = Serial.read();
        if (c == 'o')
        {
            enable = 0;
            oscilate = !oscilate;
            display(0,"oscilate=%d",oscilate);
            if (oscilate)
            {
                osc_dir = 1;
                servo.write(userToServo(127));
                servo.attach(SERVO_PWM_PIN);
                osc_time = millis();
            }
            else
            {
                servo.detach();
            }
        }
        else if (c == 'e')
        {
            enable = !enable;
            display(0,"set enable=%d",enable);
            if (enable)
            {
                servo.write(userToServo(position));
                servo.attach(SERVO_PWM_PIN);
            }
            else
            {
                servo.detach();
            }
        }
        else if (c == 13)
        {
            Serial.print(c);
            display(0,"set position=%d",position);
            servo.write(userToServo(position));
            // servo.write(position);
            display(0,"readback us=%d",servo.readMicroseconds());
            position = 0;
        }
        else if (c >= '0' && c <= '9')
        {
            Serial.print(c);
            position = position * 10 + c - '0';
        }
        else if (c == 10)
        {
            Serial.print(c);
        }
    }


    // pedal

    num_pedal_reads++;
    working_sample += analogRead(PEDAL_IN_PIN);
    if (num_pedal_reads >= PEDAL_READS_PER_SAMPLE)
    {
        cur_sample = (working_sample + PEDAL_READS_PER_SAMPLE/2) / PEDAL_READS_PER_SAMPLE;
        num_pedal_reads = 0;
        working_sample = 0;

        static int last_value = 0;
        int value = pedalToUser(cur_sample);
        if (last_value != value)
        {
            last_value = value;
            display(0,"us(%d) pot(%d) user(%d)",servo.readMicroseconds(),cur_sample,value);
        }
    }

}   // loop()
