//============================================
// ADC demo for lgt328p
// Nulllab org
// Using new added internal 2.56V reference
// analogReadResolution:
// ADC10BIT ADC12BIT

// analogReference :
// DEFAULT EXTERNAL INTERNAL1V024 INTERNAL2V048 INTERNAL4V096
//============================================

uint16_t value;

void setup() {
  // put your setup code here, to run once:
  analogReadResolution(ADC12BIT);  // ADC10BIT default is 10bit 
  analogReference(INTERNAL2V048);   // DEFAULT EXTERNAL INTERNAL1V024 INTERNAL2V048 INTERNAL4V096
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  value = analogRead(A0);
  Serial.println(value);
  delay(1);
}
