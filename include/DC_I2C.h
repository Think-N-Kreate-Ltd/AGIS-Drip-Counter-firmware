#ifndef A97DACBB_1FE5_45F3_8425_E8ED8D1C89BD
#define A97DACBB_1FE5_45F3_8425_E8ED8D1C89BD

#include <Arduino.h>
#include <Drip_Counter_API_Config.h>

class MyI2CPeripheral {
public:
  MyI2CPeripheral()
      : currentRegister(0), statusValue(0x42), someValue(0x123456) {}

  void i2cInit();
  //   bool commandPending();
  void process(volatile uint8_t *buffer, uint8_t len);
  SlaveResponse getResponse();
  uint8_t expectedReceiveLength(uint8_t commandID);

private:
  uint8_t currentRegister;
  uint8_t statusValue;
  uint32_t someValue;
  uint16_t customData[2];
  //   volatile uint8_t pendingCommandLength;
};

extern MyI2CPeripheral I2CDevice;

extern volatile uint8_t pendingCommand[];
extern volatile uint8_t pendingCommandLength;

void onRequest();
void onReceive(int len);

#endif /* A97DACBB_1FE5_45F3_8425_E8ED8D1C89BD */
