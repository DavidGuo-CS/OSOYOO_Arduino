#include <PMU.h>

void setup()
{
  Serial.begin(9600);
  Serial.println("LVD Demo");
  PMU.set_lvd(PM_LVD_4V0);
}

void loop()
{
}
