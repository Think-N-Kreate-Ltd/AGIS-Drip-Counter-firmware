#include <Arduino.h>
#include <Drip_Counter.h>
#include <esp_log.h>

static const char *TAG = "Drip_Counter_API";

Drip_Counter::Drip_Counter() {}

Drip_Counter::~Drip_Counter() {}

/// @brief Initialize Drip Counter object
/// NOTE: this is a blocking test
void Drip_Counter::init() {

  // Initialize data
  dripCounterDataPackage.data.dripRate = 0;
  dripCounterDataPackage.data.numDrops = 0;
  // add more...

//   Perform transmission test
  while (!transmissionTest()) {
    delay(500);
  }
}

/// @brief Request data from Drip Counter device
/// @return True if received exact number of bytes, False otherwise
bool Drip_Counter::requestData() {
  // send command
  Wire.beginTransmission((uint8_t)DC_I2C_ADDR);
  Wire.write(&COMMANDS_LOOKUP_TABLE[CMD_GET_DATA][0], 1);
  uint8_t error = Wire.endTransmission(true);
  Serial.printf("endTransmission: %u\n", error); // TODO: error handling

  // receive data
  uint8_t bytesReceived = Wire.requestFrom((uint8_t)DC_I2C_ADDR, COMMANDS_LOOKUP_TABLE[CMD_GET_DATA][2]);
  if (bytesReceived == sizeof(dripCounterData_t)) {
    Wire.readBytes((uint8_t*)&dripCounterDataPackage.data, bytesReceived);
    ESP_LOGD(TAG, "Received [%d] bytes",bytesReceived);
  } else {
    // TODO: error handling

    ESP_LOGE(TAG, "Number of bytes received mismatch");
    return false;
  }
  return true;
}

/// @brief Return drip rate from requested data
uint16_t Drip_Counter::getDripRate() { return dripCounterDataPackage.data.dripRate; }

/// @brief Return number of drops from requested data
uint16_t Drip_Counter::getNumDrops() { return dripCounterDataPackage.data.numDrops; }

void Drip_Counter::getStatus() {
  // send command
  Wire.beginTransmission((uint8_t)DC_I2C_ADDR);
  Wire.write(&COMMANDS_LOOKUP_TABLE[CMD_GET_TIME][0], 1);
  uint8_t error = Wire.endTransmission(true);
  Serial.printf("endTransmission: %u\n", error); // TODO: error handling

  // receive data
  uint8_t len = Wire.requestFrom((uint8_t)DC_I2C_ADDR, COMMANDS_LOOKUP_TABLE[CMD_GET_TIME][2]);
  uint8_t recvBuf[len+1];
  recvBuf[len] = '\0';  // received buffer is not NULL terminated
  Wire.readBytes(recvBuf, len);
  ESP_LOGD(TAG, "Received [%d] bytes: %s",len, recvBuf);
}

/// @return TRUE if first drop is detected, FALSE otherwise
bool Drip_Counter::firstDropDetected() {
  return dripCounterDataPackage.data.firstDropDetected;
}

/// @brief Send a test buffer to device and see if we get back the same data
/// @return True if we get back the same data, False otherwise
bool Drip_Counter::transmissionTest() {
  // send command and associated data
  Wire.beginTransmission((uint8_t)DC_I2C_ADDR);
  Wire.write(&COMMANDS_LOOKUP_TABLE[CMD_TRANSMISSION_TEST][0], 1);
//   uint8_t error = Wire.endTransmission(true);
//   Serial.printf("endTransmission: %u\n", error); // TODO: error handling

//   // send data
//   Wire.beginTransmission(DC_I2C_ADDR);
  Wire.write(TRANSMISSION_TEST_BUF, sizeof(TRANSMISSION_TEST_BUF));
  uint8_t error = Wire.endTransmission(true);
  Serial.printf("endTransmission: %u\n", error); // TODO: error handling

  delay(50); // short delay to wait for echo message from device

  // receive data
  uint8_t len = Wire.requestFrom(DC_I2C_ADDR, sizeof(TRANSMISSION_TEST_BUF));
  uint8_t recvBuf[len+1];
  recvBuf[len] = '\0';  // received buffer is not NULL terminated
  Wire.readBytes(recvBuf, len);
  ESP_LOGD(TAG, "Received [%d] bytes: %#X",len, recvBuf);

  // Compare 2 buffers and complain if mismatch
  if (memcmp(recvBuf, TRANSMISSION_TEST_BUF, sizeof(TRANSMISSION_TEST_BUF)) == 0) {
    // match
    ESP_LOGD(TAG, "Device initialized successfully");
    return true;
  } else {
    // mismatch
    ESP_LOGE(TAG, "Initialization failed. Data mismatched");
    return false;
  }
}