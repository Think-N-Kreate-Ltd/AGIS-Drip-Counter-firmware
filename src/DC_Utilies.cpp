#include <DC_Utilities.h>
#include <Arduino.h>
#include <DC_Commons.h>

static const char* BATTERY_TAG = "BATTERY";

/**
 * Read remaining battery voltage
 * @param none
 * @return battery voltage (V)
 */
// TODO: battery voltage is offset from the measured one. How to compensate?
float getBatteryVoltage() {

  /*NOTE: this whole process takes less than 150ms to complete (checked with scope)*/
  // To reduce this time, we can use smaller number of samples at the expense of
  // more noisy ADC
  // Float calculation may also slow down the whole process.

  /*Read battery ADC*/
  digitalWrite(BATT_ADC_ENABLE_PIN, LOW);  // enable to start sampling
  delay(25);  // took 25ms for ADC to fully stable (due to capacitor), check the scope to see
  int sum = 0;
  int num_samples = 1000;
  for(int i = 0; i < num_samples; i++){
    sum += analogRead(BATT_ADC_PIN);
  }
  digitalWrite(BATT_ADC_ENABLE_PIN, HIGH);  // disable to save power
  float average = float(sum) / num_samples;

  /*Convert ADC to battery voltage*/
  // 12 bit ADC => ADC = 0 ---> 4095
  // Vref = 3.3V
  // => V_ADC = ADC * 3.3 / 4095
  // Voltage divider with equal resistors => V_BATT = V_ADC * 2
  float batteryVoltage = (average * 3.3 / 4095) * 2;
  ESP_LOGD(BATTERY_TAG, "Battery voltage: %.2f", batteryVoltage);
  return batteryVoltage;
}

/// @brief Get charging status based on the states of the charge status indicators (check WS4508S datasheet for more details)
/// @return Charging status, one of the three: NOT_CHARGING, CHARGING, CHARGE_COMPLETED.
charge_status_t getChargeStatus() {
  // CHGb_state: Open-drain Charge Status Ouput. LOW when battery is charging, Hi-Z when charge cycle completed or VCC is removed.
  bool CHGb_state = digitalRead(BATT_CHGb_PIN);

  // STDBYb_state: Charge Complete Status Output. LOW when charge cycle completed, Hi-Z otherwise.
  bool STDBYb_state = digitalRead(BATT_STDBYb_PIN);

  if ((CHGb_state == LOW) && (STDBYb_state == LOW)) {
    ESP_LOGD(BATTERY_TAG, "Charger is not connected");
    return charge_status_t::NOT_CHARGING;
  }
  else if ((CHGb_state == LOW) && (STDBYb_state == HIGH)) {
    ESP_LOGD(BATTERY_TAG, "Battery is charging");
    return charge_status_t::CHARGING;
  }
  else if ((CHGb_state == HIGH) && (STDBYb_state == LOW)) {
    ESP_LOGD(BATTERY_TAG, "Battery charge completed");
    return charge_status_t::CHARGE_COMPLETED;
  }
  else {
    ESP_LOGD(BATTERY_TAG, "Unknown status");
    return charge_status_t::UNKNOWN;
  }
}