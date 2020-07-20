#include <myDebug.h>

#define DATA_OUT_PIN     5
#define DATA_IN_PIN      3

#define PEDAL_IN_PIN                A7
#define PEDAL_OUT_PIN1              10     // PWM output to Motor Controller (L293, L9110 or similar)
#define PEDAL_OUT_PIN2              9



void setup()
{
    Serial.begin(115200);
    uint32_t timeout = millis();
    while (!Serial && millis() < timeout + 1000) {}
    display(0,"detentTest started",0);

    digitalWrite(DATA_IN_PIN,0);
    pinMode(DATA_IN_PIN,INPUT);     // crucial that this pin NEVER goes high

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

}



//------------------------------
// loop()
//------------------------------


#define PEDAL_READS_PER_SAMPLE      10

#define DETENT_WINDOW       10
#define DETENT_MAGNET       20
#define DETENT_FORCE        255


int cur_sample = 0;
int working_sample = 0;
int num_pedal_reads = 0;

int detent_pos = 500;



void loop()
{
    //----------------------------
    // Serial port
    //----------------------------

    if (Serial.available())
    {
        static int set_detent = 0;

        char c = Serial.read();
        if (c == 13)
        {
            Serial.print(c);
            display(0,"set_detent=%d",set_detent);
            detent_pos = set_detent;
            set_detent = 0;
        }
        else if (c >= '0' && c <= '9')
        {
            Serial.print(c);
            set_detent = set_detent * 10 + c - '0';
        }
        else if (c == 10)
        {
            Serial.print(c);
        }
    }


    //-----------------------------------
    // sample pedal
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
    // move pedal on each full sample
    //-----------------------------------

    if (!num_pedal_reads)
    {
        int force = DETENT_FORCE;
        int direction = 0;
        static int count = 0;

        if (cur_sample >= detent_pos+DETENT_WINDOW &&
            cur_sample <= detent_pos+DETENT_WINDOW+DETENT_MAGNET)
        {
            direction = -1;
            display(0,"backwards in(%d) %d",count++,cur_sample);
        }
        else if
            (cur_sample <= detent_pos-DETENT_WINDOW &&
             cur_sample >= detent_pos-DETENT_WINDOW-DETENT_MAGNET)
        {
            direction = 1;
            display(0,"forwards in(%d) %d",count++,cur_sample);
        }
        else    // just displays moves
        {
            static int last_pos = 0;
            if (cur_sample > last_pos + 4 ||
                cur_sample < last_pos - 4)
            {
                last_pos = cur_sample;
                display(0,"no_move(%d) %d",count++,cur_sample);
            }
        }

        if (direction > 0)
        {
            analogWrite(PEDAL_OUT_PIN1,force);
            analogWrite(PEDAL_OUT_PIN2,0);
        }
        else if (direction < 0)
        {
            analogWrite(PEDAL_OUT_PIN1,0);
            analogWrite(PEDAL_OUT_PIN2,force);
        }
        else
        {
            analogWrite(PEDAL_OUT_PIN1,0);
            analogWrite(PEDAL_OUT_PIN2,0);
        }
    }

}   // loop()
