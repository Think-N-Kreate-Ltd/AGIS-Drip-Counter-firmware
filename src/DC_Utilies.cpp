#include <DC_Utilities.h>
#include <Arduino.h>
#include <DC_Commons.h>

/**
 * Read remaining battery voltage
 * @param none
 * @return battery voltage (V)
 */
float getBatteryVoltage() {

  /*NOTE: this whole process takes less than 150ms to complete (checked with scope)*/
  // To reduce this time, we can use smaller number of samples at the expense of
  // more noisy ADC
  // Float calculation may also slow down the whole process.

  /*Read battery ADC*/
  digitalWrite(ADC_ENABLE_PIN, LOW);  // enable to start sampling
  delay(25);  // took 25ms for ADC to fully stable (due to capacitor), check the scope to see
  int sum = 0;
  int num_samples = 1000;
  for(int i = 0; i < num_samples; i++){
    sum += analogRead(ADC_PIN);
  }
  digitalWrite(ADC_ENABLE_PIN, HIGH);  // disable to save power
  float average = float(sum) / num_samples;

  /*Convert ADC to battery voltage*/
  // 12 bit ADC => ADC = 0 ---> 4095
  // Vref = 3.3V
  // => V_ADC = ADC * 3.3 / 4095
  // Voltage divider with equal resistors => V_BATT = V_ADC * 2
  return (average * 3.3 / 4095) * 2;
}