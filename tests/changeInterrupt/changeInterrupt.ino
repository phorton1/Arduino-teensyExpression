#include "myDebug.h"

const byte ledPin = 13;
const byte interruptPin = 3;
volatile byte state = LOW;

void setup() 
{
    Serial.begin(115200);
    uint32_t timeout = millis();
    while (!Serial && millis() < timeout + 1000) {}
    display(0,"changeInterrupt started",0);
        
  pinMode(ledPin, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), blink, CHANGE);
}

void loop() {
  digitalWrite(ledPin, state);
}

void blink() 
{
    display(0,"ISR triggered",0); 
  state = !state;
}
