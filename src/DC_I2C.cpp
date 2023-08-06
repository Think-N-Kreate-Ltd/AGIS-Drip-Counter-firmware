#include <DC_I2C.h>
#include <Wire.h>
#include <DC_Commons.h>
#include <Arduino.h>
#include <esp_log.h>
#include <DC_Logging.h>
#include <Drip_Counter_API_config.h>

static drip_counter_data_t drip_counter_data; 
char buf[32];
static uint8_t i2cRecvMode;  // 0: Command; 1: Data

void DC_i2cInit() {
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  Wire.begin((uint8_t)DC_I2C_ADDR, DC_I2C_SDA_PIN, DC_I2C_SCL_PIN, DC_I2C_FREQ);

  // Set the default receive mode
  i2cRecvMode = 0;

  //TODO: add log info

  //TODO: error handling

}

void onRequest(){
  if (i2cRecvMode == 1) { // transmission test
    Wire.write((byte *)&buf, sizeof(buf));
    i2cRecvMode = 0;
  } else if (i2cRecvMode == 0){ // data
    /*Package data to struct before sending out*/
    drip_counter_data.dripRate = dripRate;
    drip_counter_data.numDrops = numDrops;
    // add more...

    Wire.write((byte *)&drip_counter_data, sizeof(drip_counter_data_t));
  }

  ESP_LOGD(I2C_LOG_TAG, "Data sent");
}

void onReceive(int len){
  // char buf[len+1];
  buf[len] = '\0';   // received buffer is not NULL terminated 

  Wire.readBytes(buf, len);
  ESP_LOGD(I2C_LOG_TAG, "Received: %s", buf);

  i2cRecvMode = 1; // TODO: refactor
}