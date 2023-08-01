#include <DC_Utilities.h>
#include <Arduino.h>
#include <DC_Commons.h>

/**
 * Read remaining battery voltage
 * @param none
 * @return battery voltage (V)
 */
float getBatteryVoltage() {
  digitalWrite(ADC_ENABLE_PIN, LOW);
  delay(2);
  int sum = 0;
  for (int i = 0; i < 1000; i++) {
    sum = sum + analogRead(ADC_PIN);
  }
  digitalWrite(ADC_ENABLE_PIN, HIGH);  // disable to save power
  float average = float(sum) / 1000;
  return average;
//   return float(average) / 4095 * (3.3) * 2;  // voltage divider with equal resistor values
}