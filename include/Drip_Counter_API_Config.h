#ifndef D9883568_A0F9_4F92_BBB2_2C8DFDE8A374
#define D9883568_A0F9_4F92_BBB2_2C8DFDE8A374

#include <inttypes.h>

/*Device I2C adress*/
#define DC_I2C_ADDR                    0x23

/*Maximum number of bytes of each I2C transaction*/
#define RCV_COMMAND_MAX_BYTES          20

typedef struct dripCounterDataStruct {
    uint16_t dripRate;          // 2 bytes
    uint16_t numDrops;          // 2 bytes
    // uint16_t timeBtw2Drops;     // 2 bytes
    // bool firstDropDetected;     // 1 byte
    // add more...
} dripCounterData_t;

/// @brief Using union so that later we can send data out as array of bytes
typedef union dripCounterDataPackageUnion {
  dripCounterData_t data;
  uint8_t bytes[sizeof(dripCounterData_t)];
} dripCounterDataPackage_t;

/*Commands Lookup Table (CLT)*/
// TODO: define the commands
#define CMD_GET_STATUS                 0x00
#define CMD_SET_TIME                   0x01
#define CMD_GET_TIME                   0x02
#define CMD_GET_DATA                   0x03
/// @brief Assuming each command is 1 byte
static const uint8_t commandsLookupTable[4][3] = {
// ID,message length,response 
  {(uint8_t)CMD_GET_STATUS,     0x01, 0x01},
  {(uint8_t)CMD_SET_TIME,       0x05, 0x01},
  {(uint8_t)CMD_GET_TIME,       0x01, 0x04},
  {(uint8_t)CMD_GET_DATA,       0x01, sizeof(dripCounterData_t)},
};

/// @brief Container to hold a buffer and its size (C++ style)
typedef struct SlaveResponseStruct {
  uint8_t * buffer;
  uint8_t size;
  SlaveResponseStruct() : buffer(NULL), size(0) {}
  SlaveResponseStruct(uint8_t * buf, uint8_t len) : buffer(buf), size(len) {}
} SlaveResponse;

#endif /* D9883568_A0F9_4F92_BBB2_2C8DFDE8A374 */
