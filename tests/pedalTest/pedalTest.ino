
#include <myDebug.h>
#include "ampMeter.h"
#include "autoPedal.h"

// default PINS used by this program (in cpp files)
//
//  AMP_METER_PIN       A6      Analog input from ACS712 current sensor
//  PEDAL_IN_PIN        A7      Analog input from Pedal poteniometer
//  PEDAL_OUT_PIN1      9       PWM output to Motor Controller (L293, L9110 or similar)
//  PEDAL_OUT_PIN2      10      PWM output to Motor Controller (L293, L9110 or similar




//------------------------------
// setup()
//------------------------------

void setup()
{
    Serial.begin(115200);
    delay(1000);
    display(0,"pedalTest started",0);

    thePedal.start();
    // thePedal.factoryCalibrate();
}



//------------------------------
// loop()
//------------------------------

void loop()
{

    thePedal.task();

    //----------------------------
    // Serial port
    //----------------------------

    static int the_value = 0;

    if (Serial.available())
    {
        char c = Serial.read();
        if (c == 'c')
        {
            Serial.println(c);
            the_value = 0;
            thePedal.clear();
            theAmpMeter.clearOverload();
            display(0,"calibrate",0);
            thePedal.factoryCalibrate();
        }
        else if (c == 27)
        {
            Serial.println();
            thePedal.clear();
            display(0,"reset",0);
            the_value = 0;
        }
        else if (c == 13)
        {
            Serial.print(c);
            if (the_value > 127)
                the_value = 127;
            theAmpMeter.clearOverload();
            thePedal.setValue(the_value);
            the_value = 0;
        }
        else if (c >= '0' && c <= '9')
        {
            Serial.print(c);
            the_value = the_value * 10 + c - '0';
        }
        else if (c == 10)
        {
            Serial.print(c);
        }
    }


}   // loop()
