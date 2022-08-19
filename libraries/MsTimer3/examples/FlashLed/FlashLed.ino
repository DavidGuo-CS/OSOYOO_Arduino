#include <MsTimer3.h>
// Switch on LED on and off each half second

#if ARDUINO >= 100
const int led_pin = LED_BUILTIN; // 1.0 built in LED pin var
#else
const int led_pin = 13; // default to pin 13
#endif

void flash()
{
  static boolean output = HIGH;
  // Serial.println("flash led");
  digitalWrite(led_pin, output);
  output = !output;
}


void setup()
{
    Serial.begin(9600);
    pinMode(led_pin, OUTPUT);
    MsTimer3::set(1000, flash); // 500ms period
    MsTimer3::start();
    Serial.println("setup");
}

void loop()
{

}
