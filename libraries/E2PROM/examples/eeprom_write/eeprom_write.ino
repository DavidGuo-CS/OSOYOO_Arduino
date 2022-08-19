/*
 * EEPROM Write
 *
 * Stores values read from analog input 0 into the EEPROM.
 * These values will stay in the EEPROM when the board is
 * turned off and may be retrieved later by another sketch.
 */

#include <EEPROM.h>

// the current address in the EEPROM (i.e. which byte
// we're going to write to next)
int addr = 10;

void setup()
{
    Serial.begin(9600);
    while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
    }
    Serial.print("EEPROM length:");
    Serial.println(EEPROM.length());
    EEPROM.write(0, 20);
    EEPROM.write(1, 31);
    EEPROM.write(2, 42);
    EEPROM.write(3, 53);
    EEPROM.write(4, 108);
    EEPROM[5] = 109;
    EEPROM[6] = 45;

}

void loop()
{
  // need to divide by 4 because analog inputs range from
  // 0 to 1023 and each byte of the EEPROM can only hold a
  // value from 0 to 255.
  int val = analogRead(0) / 4;
  
  // write the value to the appropriate byte of the EEPROM.
  // these values will remain there when the board is
  // turned off.
  EEPROM.write(addr, val);
  Serial.print("EEPROM Write:");
  Serial.print(addr);
  Serial.print(" val:");
  Serial.println(val);
  // advance to the next address.  there are 512 bytes in 
  // the EEPROM, so go back to 0 when we hit 512.
  addr = addr + 1;
  if (addr == EEPROM.length())
    addr = 0;
  
  delay(100);
}
