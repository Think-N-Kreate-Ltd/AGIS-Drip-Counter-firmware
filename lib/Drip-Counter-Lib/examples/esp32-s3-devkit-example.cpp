#include <Arduino.h>
#include <Wire.h>
#include <Drip_Counter.h>

#define AGIS_I2C_SCL_PIN  47
#define AGIS_I2C_SDA_PIN  48
#define I2C_FREQ          100000

Drip_Counter drip_counter;
uint16_t dripRate;
uint16_t numDrops;
bool firstDropDetected = false;

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  Wire.begin(AGIS_I2C_SDA_PIN, AGIS_I2C_SCL_PIN, 100000);

  /*Initialize Drip Counter device*/
  drip_counter.init();
}

void loop() {
  if(drip_counter.requestData()) {
    dripRate = drip_counter.getDripRate();
    numDrops = drip_counter.getNumDrops();
    firstDropDetected = drip_counter.firstDropDetected();
    Serial.printf("dripRate: %d \tnumDrops: %d \tfirstDropDetected: %d\n",
                  dripRate, numDrops, firstDropDetected);
  }
  else {
    Serial.printf("Data requested failed\n");
  }

  // drip_counter.getStatus();

  delay(1000);
}