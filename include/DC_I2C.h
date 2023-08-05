#ifndef A97DACBB_1FE5_45F3_8425_E8ED8D1C89BD
#define A97DACBB_1FE5_45F3_8425_E8ED8D1C89BD

#include <Arduino.h>

struct drip_counter_data_t {
    uint16_t dripRate;          // 2 bytes
    uint16_t numDrops;          // 2 bytes
    // uint16_t timeBtw2Drops;     // 2 bytes
    // bool firstDropDetected;     // 1 byte
    // add more...
};

void DC_i2cInit();
void onRequest();
void onReceive(int len);

#endif /* A97DACBB_1FE5_45F3_8425_E8ED8D1C89BD */
