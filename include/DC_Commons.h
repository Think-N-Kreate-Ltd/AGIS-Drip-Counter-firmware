#ifndef E993476A_2792_4D1A_AC33_62E9F17CC12A
#define E993476A_2792_4D1A_AC33_62E9F17CC12A

/*GPIO definitions*/
#define DROP_SENSOR_PIN      18 // input pin for geting output from sensor
#define DROP_SENSOR_LED_PIN  8  // output pin to sensor for turning on LED
#define SPI_EPD_CLK          14
#define SPI_EPD_MOSI         13
#define SPI_EPD_MISO         -1
#define SPI_EPD_CS           21
#define SPI_EPD_BUSY         45
#define SPI_EPD_RST          48
#define SPI_EPD_DC           47

/*Constant definitions*/
#define DROP_DEBOUNCE_TIME   10    // if two pulses are generated within debounce time, it must be detected as 1 drop
#define DISPLAY_REFRESH_TIME 1000  // time between display refresh

const char GTT_STRING[] = "gtt/m";
const char MLH_STRING[] = "mL/h";
const char START_SCREEN_STRING[] = "Drip \nCounter";

#endif /* E993476A_2792_4D1A_AC33_62E9F17CC12A */
