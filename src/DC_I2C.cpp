#include <DC_I2C.h>
#include <Wire.h>
#include <DC_Commons.h>
#include <Arduino.h>
#include <esp_log.h>
#include <DC_Logging.h>

drip_counter_data_t drip_counter_data; 

void DC_i2cInit() {
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  Wire.begin((uint8_t)DC_I2C_ADDR, DC_I2C_SDA_PIN, DC_I2C_SCL_PIN, DC_I2C_FREQ);

  //TODO: add log info

  //TODO: error handling

#if CONFIG_IDF_TARGET_ESP32
  char message[64];
  snprintf(message, 64, "%u Packets.", i++);
  Wire.slaveWrite((uint8_t *)message, strlen(message));
#endif
}

void onRequest(){
  // Wire.print(i++);
  // Wire.print(" Packets.");
  // Wire.print("Drip Rate: ");


  /*Package data to struct before sending out*/
  drip_counter_data.dripRate = dripRate;
  drip_counter_data.numDrops = numDrops;
  // add more...

  Wire.write((byte *)&drip_counter_data, sizeof(drip_counter_data_t));
  ESP_LOGD(I2C_LOG_TAG, "Data sent");
}

void onReceive(int len){
  ESP_LOGD(I2C_LOG_TAG, "Received[%d]:", len);
  while(Wire.available()){
    //TODO: how to log bytes?
    // ESP_LOGD(I2C_LOG_TAG, "%b", Wire.read());
    Serial.write(Wire.read());
  }
  Serial.println();
}