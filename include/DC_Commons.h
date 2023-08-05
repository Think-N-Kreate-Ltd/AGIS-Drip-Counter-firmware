#ifndef E993476A_2792_4D1A_AC33_62E9F17CC12A
#define E993476A_2792_4D1A_AC33_62E9F17CC12A

/*GPIO definitions*/
// Drop sensor
#define DROP_SENSOR_PIN      18 // input pin for geting output from sensor
#define DROP_SENSOR_LED_PIN  8  // output pin to sensor for turning on LED

// Display - SPI
#define SPI_EPD_CLK          14
#define SPI_EPD_MOSI         13
#define SPI_EPD_MISO         -1
#define SPI_EPD_CS           21
#define SPI_EPD_BUSY         45
#define SPI_EPD_RST          48
#define SPI_EPD_DC           47

// Battery monitoring - ADC
#define ADC_ENABLE_PIN       46
#define ADC_PIN              3

// Latch pin for power on/off
#define LATCH_IO_PIN         9

// I2C pin for data output
#define DC_I2C_ADDR         0x23
#define DC_I2C_SDA_PIN      35
#define DC_I2C_SCL_PIN      36
#define DC_I2C_FREQ         1000000 // 100khz

/*Constant definitions*/
#define DROP_DEBOUNCE_TIME   10    // if two pulses are generated within debounce time, it must be detected as 1 drop
#define DISPLAY_REFRESH_TIME 1000  // time between display refresh
#define BATTERY_MONITOR_TIME 5000  // time between battery monitor

const char GTT_STRING[] = "gtt/m";
const char MLH_STRING[] = "mL/h";
const char START_SCREEN_STRING[] = "Drip \nCounter";
const char POWER_OFF_SCREEN_STRING[] = "Shut \ndown...";

/*Externally declared variables*/
extern volatile unsigned int dripRate;
extern volatile unsigned int numDrops;

#endif /* E993476A_2792_4D1A_AC33_62E9F17CC12A */
