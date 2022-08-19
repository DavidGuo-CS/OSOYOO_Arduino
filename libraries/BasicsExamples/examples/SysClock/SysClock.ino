// demo for system clock switch
// using function "sysClock(mode)" to switch
// system clock between internal or external oscillator
// e.g:
// INT_OSC_32K	0
// INT_OSC_32M	1
// EXT_OSC_32M	2
// EXT_OSC_24M	3
// EXT_OSC_16M	4
// EXT_OSC_12M	5
// EXT_OSC_8M	6
// EXT_OSC_4M	7
// EXT_OSC_2M	8
// EXT_OSC_1M	9
// EXT_OSC_400K 10
// EXT_OSC_32K 11
//  sysClock(INT_OSC_32M) for internal 32M oscillator
//  sysClock(EXT_OSC_16M) for external 16M crystal

void setup() {
  // put your setup code here, to run once:
  sysClock(INT_OSC_32M);          // Set internal 32M oscillator
  sysClockPrescale(SYSCLK_DIV_2); // set clock prescale 0 2 4 8 16 32 64 128
  sysClockOutput(1);              // sysclock out put to D8
  pinMode(13, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalToggle(13);
}