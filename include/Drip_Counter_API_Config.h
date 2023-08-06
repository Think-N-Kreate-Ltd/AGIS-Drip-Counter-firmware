#ifndef D9883568_A0F9_4F92_BBB2_2C8DFDE8A374
#define D9883568_A0F9_4F92_BBB2_2C8DFDE8A374

#include <inttypes.h>

/*Device I2C adress*/
#define DC_I2C_ADDR                    0x23

/*Device specific commands*/
#define DC_CMD_TRANSMISSION_TEST       0x01
#define DC_CMD_REQUEST_DATA            0x02


struct drip_counter_data_t {
    uint16_t dripRate;          // 2 bytes
    uint16_t numDrops;          // 2 bytes
    // uint16_t timeBtw2Drops;     // 2 bytes
    // bool firstDropDetected;     // 1 byte
    // add more...
};

#endif /* D9883568_A0F9_4F92_BBB2_2C8DFDE8A374 */
