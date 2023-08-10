#ifndef D9883568_A0F9_4F92_BBB2_2C8DFDE8A374
#define D9883568_A0F9_4F92_BBB2_2C8DFDE8A374

#include <inttypes.h>

/*Device I2C adress*/
#define DC_I2C_ADDR                    0x23

/*Maximum number of bytes of each I2C transaction*/
#define RECV_COMMAND_MAX_BYTES          20
#define SEND_COMMAND_MAX_BYTES          20

/// @brief Buffer used for transmission test
/// ASCII equivalent: "Drip Counter"
static const uint8_t TRANSMISSION_TEST_BUF[] = {
    0x44, 0x72, 0x69, 0x70, 0x20, 0x43,
    0x6F, 0x75, 0x6E, 0x74, 0x65, 0x72};

/// @brief Content of data package to be sent out to the master
/// Note that the number of bytes of the package is not necessarily the sum
/// of the size of each member (due to data structure alignment)
typedef struct dripCounterDataStruct {
    uint16_t dripRate;          // 2 bytes
    uint16_t numDrops;          // 2 bytes
    // uint16_t timeBtw2Drops;     // 2 bytes
    bool firstDropDetected;     // 1 byte
    // add more...
} dripCounterData_t;

/// @brief Using union so that later we can send data out as array of bytes
typedef union dripCounterDataPackageUnion {
  dripCounterData_t data;                  // data in struct format
  uint8_t bytes[SEND_COMMAND_MAX_BYTES];   // bytes to be sent out
} dripCounterDataPackage_t;

// TODO: define the commands
/// @brief Assuming each command is 1 byte
#define CMD_TRANSMISSION_TEST          0x00   // receive and echo back test
#define CMD_SET_TIME                   0x01
#define CMD_GET_TIME                   0x02
#define CMD_GET_DATA                   0x03

/// @brief Commands Lookup Table (CLT)
/// message length = payload + 1 (assuming each command is 1 byte)
static const uint8_t COMMANDS_LOOKUP_TABLE[4][3] = {
// ID,message length,response 
  {(uint8_t)CMD_TRANSMISSION_TEST, sizeof(TRANSMISSION_TEST_BUF)+1, sizeof(TRANSMISSION_TEST_BUF)},
  {(uint8_t)CMD_SET_TIME,          0x05,                            0x01},
  {(uint8_t)CMD_GET_TIME,          0x01,                            0x04},
  {(uint8_t)CMD_GET_DATA,          0x01,                            sizeof(dripCounterData_t)},
};

/// @brief Container to hold a buffer and its size (C++ style)
typedef struct SlaveResponseStruct {
  uint8_t * buffer;
  uint8_t size;
  SlaveResponseStruct() : buffer(NULL), size(0) {}
  SlaveResponseStruct(uint8_t * buf, uint8_t len) : buffer(buf), size(len) {}
} SlaveResponse_t;


#endif /* D9883568_A0F9_4F92_BBB2_2C8DFDE8A374 */
