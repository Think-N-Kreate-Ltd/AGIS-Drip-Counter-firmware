#ifndef E993476A_2792_4D1A_AC33_62E9F17CC12A
#define E993476A_2792_4D1A_AC33_62E9F17CC12A

#include <version.h>

// Comment to remove version from start screen
#define SHOW_VERSION

// Comment to enable device even during charging
#define DEVICE_DISABLE_DURING_CHARGING

/*GPIO definitions*/
// Drop sensor
#define DROP_SENSOR_PIN         6  // input pin for geting output from sensor
#define DROP_SENSOR_LED_PIN     5  // output pin to sensor for turning on LED
#define DROP_SENSOR_VCC_EN_PIN  2  // to enable/disable power to sensor (HIGH: on, LOW: off)

// Display - SPI
#define SPI_EPD_CLK            48
#define SPI_EPD_MOSI           45
#define SPI_EPD_MISO           -1
#define SPI_EPD_CS             47
#define SPI_EPD_BUSY           13
#define SPI_EPD_RST            14
#define SPI_EPD_DC             21

// Battery monitoring - ADC and charge status indicator
#define BATT_ADC_ENABLE_PIN    46
#define BATT_ADC_PIN           3
#define BATT_CHGb_PIN          11
#define BATT_STDBYb_PIN        12

// Latch pin for power on/off
#define LATCH_IO_PIN           17

// I2C pin for data output
#define DC_I2C_SDA_PIN         8
#define DC_I2C_SCL_PIN         18
#define DC_I2C_FREQ            1000000 // 100khz

// User button pin
#define USER_BUTTON_PIN        7

// Buzzer
#define BUZZER_PIN             10
#define BUZZER_FREQ            2000
#define BUZZER_TIME_ON         20        // in ms
#define BUZZER_TIME_OFF        150       // in ms

/*Constant definitions*/
#define DROP_DEBOUNCE_TIME         10     // if two pulses are generated within debounce time, it must be detected as 1 drop
#define DISPLAY_REFRESH_TIME       100    // time between display refresh (Spec: 0.3s partial refresh)
#define BATTERY_MONITOR_TIME       60000  // time between battery monitor
#define BATTERY_CHARGE_STATUS_TIME 1000   // time between battery charge status check
#define POPUP_WINDOW_HOLD_TIME     2000   // time duration of each pop-up window, 2s
#define NO_DROP_ALARM_TIME         20000  // alarm will be triggered if excedeeding this time
#define AUTO_OFF_TIME              60000  // device will auto-off if exceeding this time
#define DOUBLE_PRESS_TIMEOUT       300    // 2 consecutive button press within this time is considered a double press

const char GTT_STRING[] = "gtt/m";
const char MLH_STRING[] = "mL/h";
const char DROP_FACTOR_UNIT_STRING[] = "gtt/mL";
const char START_SCREEN_STRING[] = " Drip\nCounter";
const char POWER_OFF_SCREEN_STRING[] = "Shut\ndown...";
const char BATTERY_LOW_STRING[] = "WARNING\n\nBattery \n  Low";
const char DROP_FACTOR_SELECTION_STRING[] = " Drop\nfactor:";
const uint8_t dropFactorArray[] = {10, 15, 20, 60};

enum button_state_t{
    IDLE,            // not pressed
    SINGLE_PRESS,
    DOUBLE_PRESS,
};

/*Externally declared variables*/
extern volatile unsigned int dripRate;
extern volatile unsigned int numDrops;
extern volatile bool firstDropDetected;

#endif /* E993476A_2792_4D1A_AC33_62E9F17CC12A */
