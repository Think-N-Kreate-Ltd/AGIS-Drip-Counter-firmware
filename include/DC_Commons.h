#ifndef E993476A_2792_4D1A_AC33_62E9F17CC12A
#define E993476A_2792_4D1A_AC33_62E9F17CC12A

#include <version.h>

// Comment to remove version from start screen
#define SHOW_VERSION

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

// Battery monitoring - ADC and charge status indicator
#define BATT_ADC_ENABLE_PIN  46
#define BATT_ADC_PIN         3
#define BATT_CHGb_PIN        11
#define BATT_STDBYb_PIN      12

// Latch pin for power on/off
#define LATCH_IO_PIN         9

// I2C pin for data output
#define DC_I2C_SDA_PIN      35
#define DC_I2C_SCL_PIN      36
#define DC_I2C_FREQ         1000000 // 100khz

/*Constant definitions*/
#define DROP_DEBOUNCE_TIME     10     // if two pulses are generated within debounce time, it must be detected as 1 drop
#define DISPLAY_REFRESH_TIME   1000   // time between display refresh
#define BATTERY_MONITOR_TIME   5000   // time between battery monitor, 5s
#define POPUP_WINDOW_HOLD_TIME 2000   // time duration of each pop-up window, 2s
#define NO_DROP_ALARM_TIME     20000  // alarm will be triggered if excedeeding this time
#define NO_DROP_AUTO_OFF_TIME  60000  // device will auto-off if excedeeding this time

const char GTT_STRING[] = "gtt/m";
const char MLH_STRING[] = "mL/h";
const char START_SCREEN_STRING[] = " Drip\nCounter";
const char POWER_OFF_SCREEN_STRING[] = "Shut\ndown...";
const char BATTERY_LOW_STRING[] = "WARNING\n\nBattery \n  Low";

/*Externally declared variables*/
extern volatile unsigned int dripRate;
extern volatile unsigned int numDrops;
extern volatile bool firstDropDetected;

#endif /* E993476A_2792_4D1A_AC33_62E9F17CC12A */
