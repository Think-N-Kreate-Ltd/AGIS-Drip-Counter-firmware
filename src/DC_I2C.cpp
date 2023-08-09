// Ref:
// https://inductive-kickback.com/2019/04/creating-an-i2c-slave-interface-for-a-sensor-or-peripheral/
#include <DC_I2C.h>
#include <Wire.h>
#include <DC_Commons.h>
#include <Arduino.h>
#include <esp_log.h>
#include <DC_Logging.h>

static const char* TAG = "I2C_LOG";

MyI2CPeripheral I2CDevice;

/* Two sets of buffers...
 *  receivedBytes[] will store incoming bytes as they are accumulated
 *  
 *  pendingCommand[] will hold commands as they come in from the master
 *  and be processed in a task
 *  
 * We're using a little double buffering type deal here to keep  
 * the interrupt routine simple and "safely" process the writes
 * 
 * This is still subject to a race condition if the master is 
 * writing bytes too fast, but we're operating under the assumption
 * that each write will be followed for a request for a response 
 * and enough of a delay to allow processing to take place.
 */
volatile uint8_t receivedBytes[RECV_COMMAND_MAX_BYTES];
volatile uint8_t receivedByteIdx = 0;


// pendingCommand buffer and len
// volatile because it's the way we're talking between
// our interrupt and our main 'thread'--either side may
// change the values at will
volatile uint8_t pendingCommand[RECV_COMMAND_MAX_BYTES];
volatile uint8_t pendingCommandLength = 0;

static dripCounterDataPackage_t dripCounterDataPackage; 

/// @brief Join Drip Counter as an I2C peripheral
void MyI2CPeripheral::i2cInit() {
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  Wire.begin((uint8_t)DC_I2C_ADDR, DC_I2C_SDA_PIN, DC_I2C_SCL_PIN, DC_I2C_FREQ);
  
  /*Inititialze default settings*/
  // pendingCommandLength = 0;

  //TODO: add log info

  //TODO: error handling

}

/// @brief Check if there is command to be processed
/// @return True if there is command, False otherwise
// bool MyI2CPeripheral::commandPending() {
//   if (pendingCommandLength)
//     return true;
//   else
//     return false;
// }

void MyI2CPeripheral::process(volatile uint8_t *buffer, uint8_t len) {
  // keep track of last command for any read requests later
  currentRegister = buffer[0];

  // doing things here...

  statusValue++;  // just increment to see the value is changed in each transaction
  someValue++;
}

/// @brief Returns an appropriate buffer (or more exactly, pointer to the
/// buffer), and its length, according to received command previously, or default.
/// The idea is that the response will be sent over to the master.
/// TLDR: This is where we decide what to send out to the  master.
SlaveResponse_t MyI2CPeripheral::getResponse() {
  SlaveResponse_t resp;

  switch (currentRegister) {
  case CMD_TRANSMISSION_TEST:
    // Make a copy of the received bytes
    for (uint8_t i = 0; i < COMMANDS_LOOKUP_TABLE[CMD_TRANSMISSION_TEST][1]; i++) {
      // copy manually, there could be a better way?
      dripCounterDataPackage.bytes[i] = receivedBytes[i];
    }
    resp.buffer = dripCounterDataPackage.bytes + 1;  // 1st byte is the command, ignore it
    resp.size = COMMANDS_LOOKUP_TABLE[CMD_TRANSMISSION_TEST][2];
    ESP_LOGD(TAG, "---------- Initialization requested ----------");
    break;

  case CMD_SET_TIME:
    resp.buffer = &statusValue;
    resp.size = COMMANDS_LOOKUP_TABLE[CMD_SET_TIME][2];
    break;

  case CMD_GET_TIME:
    resp.buffer = (uint8_t *)&someValue;
    resp.size = COMMANDS_LOOKUP_TABLE[CMD_GET_TIME][2];
    break;

  case CMD_GET_DATA:
    /*Package data before sending out*/
    dripCounterDataPackage.data.dripRate = dripRate;
    dripCounterDataPackage.data.numDrops = numDrops;

    resp.buffer = dripCounterDataPackage.bytes;
    resp.size = COMMANDS_LOOKUP_TABLE[CMD_GET_DATA][2];
    ESP_LOGD(TAG, "---------- Data requested ----------");
    break;

  default:
    resp.buffer = (uint8_t *)"booya";
    resp.size = 5;
    break;
  }

  return resp;
}

/// @brief This will be called when the master is aksing to get some data from
/// the device
void onRequest(){
  /*Get the response*/
  SlaveResponse_t resp = I2CDevice.getResponse();

  /*Send the response to the master*/
  Wire.write(resp.buffer, resp.size);
  ESP_LOGD(TAG, "Sent [%d] bytes\n", resp.size);
}

/// @brief This will be called when the device receives bytes of data from the
/// master. Note that we may not get all the bytes we need in a single call, and
/// the number of bytes we expect depends on what is actually being transmitted
/// @param bytesReceived: number of bytes received in this call
void onReceive(int bytesReceived){

  uint8_t msgLen = 0;

  /*Read incoming bytes*/
  for (uint8_t i = 0; i < bytesReceived; i++) {
    receivedBytes[receivedByteIdx] = Wire.read(); // read 1 byte

    // The 1st byte of the message corresponds to the expected message length
    // See the Commands Lookup Table
    if (!msgLen) {
      msgLen = I2CDevice.expectedReceiveLength(receivedBytes[0]);
      ESP_LOGD(TAG, "Received command: %#x", receivedBytes[0]);
    }

    receivedByteIdx++;

    /*When we receive the full message*/
    if (receivedByteIdx >= msgLen) {
      ESP_LOGD(TAG, "Received [%d] bytes", msgLen);

      /*copy the bytes into command buffer*/
      for (uint8_t i = 0; i < msgLen; i++) {
        pendingCommand[i] =
            receivedBytes[i]; // copy manually, there could be a better way?
      }

      // raise the flag to signal that we've got a new command ready to be
      // processed
      pendingCommandLength = msgLen;

      // reset for the next command/message
      receivedByteIdx = 0;
      msgLen = 0;
    }
  }
}

/// @brief Returns the number of bytes to receive for a given message
/// @param commandID: command ID, see the Commands Lookup Table (CLT)
/// @return Number of bytes
uint8_t MyI2CPeripheral::expectedReceiveLength(uint8_t commandID) {
  /*Check if command ID exists in the table*/
  int rows = sizeof(COMMANDS_LOOKUP_TABLE) / sizeof(COMMANDS_LOOKUP_TABLE[0]);
  if (commandID < rows) {
    return COMMANDS_LOOKUP_TABLE[commandID][1];
  } else {
    // Unknown command
    ESP_LOGD(TAG, "Unknown command: %#X", commandID);
    return 0;
  }
}