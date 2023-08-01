#include <DC_I2C.h>
#include <Wire.h>
#include <DC_Commons.h>
#include <Arduino.h>

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
  Wire.print(dripRate);
  // Serial.println("onRequest");
}

void onReceive(int len){
  Serial.printf("onReceive[%d]: ", len);
  while(Wire.available()){
    Serial.write(Wire.read());
  }
  Serial.println();
}