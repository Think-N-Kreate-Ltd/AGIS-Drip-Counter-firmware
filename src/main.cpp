#include <Arduino.h>
#include <ezButton.h>
#include <Wire.h>
#include <esp_log.h>

// uncomment next line to use class GFX of library GFX_Root instead of Adafruit_GFX
//#include <GFX.h>

#define SPI_EPD_CLK     14
#define SPI_EPD_MOSI    13
#define SPI_EPD_MISO    -1
#define SPI_EPD_CS      21
#define SPI_EPD_BUSY    45
#define SPI_EPD_RST     48
#define SPI_EPD_DC      47

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"

struct fonts {
  const GFXfont* font;
  int width;
  int height;
};

fonts font_xl = {&FreeSansBold18pt7b, 9, 19};

struct partial_box {
  int left, top, width, height;

  void clear(bool black){
    display.setPartialWindow(left, top, width, height);
    display.firstPage();
    do{
      if(black) display.fillRect(left, top, width, height, GxEPD_BLACK);
      else display.fillRect(left, top, width, height, GxEPD_WHITE);
    }
    while (display.nextPage());
  }

  void print_text(String str, fonts f) {
    display.setPartialWindow(left, top, width, height);
    display.setFont(f.font);
    display.setTextColor(GxEPD_BLACK);

    int16_t tbx, tby; uint16_t tbw, tbh;
    display.getTextBounds(str, left, top, &tbx, &tby, &tbw, &tbh);
    uint16_t x = (left + width/2 - tbw / 2 - 1);
    display.getTextBounds(str, left, top, &tbx, &tby, &tbw, &tbh);
    uint16_t y = top + height/2 + tbh/2 - 1;

    display.firstPage();
    do{
      display.fillRect(left, top, width, height, GxEPD_WHITE);
      display.setCursor(x, y);
      display.print(str);
    }
    while (display.nextPage());
  }
};

// Initializing the boxes
partial_box dripRateGttBox = {0, 0, display.width(), 40};
partial_box dripRateMLhBox = {0, 60, display.width(), 40};
const char rateGttString[] = "gtt/m";
const char rateMLhString[] = "mL/h";

/*GPIO definitions*/
#define DROP_SENSOR_PIN      18 // input pin for geting output from sensor
#define DROP_SENSOR_LED_PIN  8  // output pin to sensor for turning on LED

/*Constant definitions*/
#define DROP_DEBOUNCE_TIME   10 // if two pulses are generated within debounce time, it must be detected as 1 drop

/*Variables for monitoring dripping parameters*/
ezButton dropSensor(DROP_SENSOR_PIN);     // create ezButton object that attaches to drop sensor pin
volatile unsigned int numDrops = 0;       // for counting the number of drops
volatile unsigned int dripRate = 0;       // for calculating the drip rate
volatile unsigned int timeBtw2Drops = UINT_MAX; // i.e. no more drop recently
volatile bool firstDropDetected = false;  // to check when we receive the 1st drop

/*Variables related to infusion*/
unsigned int dropFactor = 20;             // default value
volatile unsigned int infusedVolume_x100 = 0;  // 100 times larger than actual value, unit: mL

/*Timer pointers*/
hw_timer_t *Timer0_cfg = NULL; // create a pointer for timer0

/*Other variables*/
volatile bool turnOnLed = false;          // used to turn on the LED for a short time

/*Function prototypes*/
void IRAM_ATTR dropSensorISR();
void IRAM_ATTR dripCountUpdateISR();
void dropDetectedLEDTask(void *);
void refreshDisplayTask(void *);
void displayRateString();

void setup() {
  Serial.begin(115200);

  /*GPIO setup*/
  pinMode(DROP_SENSOR_PIN, INPUT);
  pinMode(DROP_SENSOR_LED_PIN, OUTPUT);
  digitalWrite(DROP_SENSOR_LED_PIN, HIGH); // prevent it initially turn on

  /*Setup for sensor interrupt*/
  attachInterrupt(DROP_SENSOR_PIN, &dropSensorISR, CHANGE);  // call interrupt when state change

  /*Setup for timer0*/
  Timer0_cfg = timerBegin(0, 80, true); // prescaler = 80
  timerAttachInterrupt(Timer0_cfg, &dripCountUpdateISR,
                       false);              // call the function motorcontrol()
  timerAlarmWrite(Timer0_cfg, 1000, true); // time = 80*1000/80,000,000 = 1ms
  timerAlarmEnable(Timer0_cfg);            // start the interrupt

  /*Setup for Epaper display*/
  // TODO: refactor
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.begin(SPI_EPD_CLK, SPI_EPD_MISO, SPI_EPD_MOSI, SPI_EPD_CS); // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)
  display.init(0UL, true);
  display.setRotation(4);
  display.setFont(font_xl.font);
  display.setTextColor(GxEPD_BLACK);
  displayRateString();

  /*Create a task for toggling LED everytime a drop is detected*/
  xTaskCreate(dropDetectedLEDTask,   /* Task function. */
              "Drop Detected LED Task", /* String with name of task. */
              4096,              /* Stack size in bytes. */
              NULL,              /* Parameter passed as input of the task */
              0,                 /* Priority of the task. */
              NULL);             /* Task handle. */

  /*Create a task for refreshing Epaper display*/
  xTaskCreate(refreshDisplayTask,   /* Task function. */
              "Refresh Display Task", /* String with name of task. */
              4096,              /* Stack size in bytes. */
              NULL,              /* Parameter passed as input of the task */
              0,                 /* Priority of the task. */
              NULL);             /* Task handle. */
}


void loop() {
  // Serial.printf("numDrops: %d, \tdripRate: %d\n", numDrops, dripRate);
}

/**
 * Process the interrupt signals generated by drops
 * @param none
 * @return none
 */
void IRAM_ATTR dropSensorISR() {
  static int lastState;    // var to record the last value of the sensor
  static int lastTime;     // var to record the last value of the calling time
  static int lastDropTime; // var to record the time of last drop

  // in fact, the interrupt will only be called when state change
  // just one more protection to prevent calling twice when state doesn't change
  int dropSensorState = dropSensor.getStateRaw();
  if (lastState != dropSensorState) {
    lastState = dropSensorState;
    // call when drop detected
    // disable for `DROP_DEBOUNCE_TIME` ms after called
    if ((dropSensorState == 1) && 
        ((millis()-lastTime)>=DROP_DEBOUNCE_TIME)) {
      turnOnLed = true; // turn on LED for a short time

      lastTime = millis();

      // // FIRST DROP DETECTION
      // if (!firstDropDetected){
      //   firstDropDetected = true;
      //   lastDropTime = -9999; // prevent timeBtw2Drops become inf

      //   if (!lockInfusionStartTime) {
      //     // mark this as starting time of infusion
      //     infusionStartTime = millis();
      //   }
      // }
      // if (infusionState != infusionState_t::IN_PROGRESS) {
      //   // TODO: when click "Set and Run" button on the website again to
      //   // start another infusion, infusionState should be IN_PROGRESS but
      //   // somehow it is STARTED
      //   infusionState = infusionState_t::STARTED; // droping has started
      // }

      // record the value
      timeBtw2Drops = millis() - lastDropTime;
      lastDropTime = millis();
      numDrops++;

      // NOTE: Since we cannot do floating point calculation in interrupt,
      // we multiply the actual infused volume by 100 times to perform the integer calculation
      // Later when we need to display, divide it by 100 to get actual value.
      if (dropFactor != UINT_MAX) {
        // BUG: with some dropFactor, the division will return less accurate result
        infusedVolume_x100 += (100 / dropFactor);
      }

      // if infusion has completed but we still detect drop,
      // something must be wrong. Need to sound the alarm.
      // if (infusionState == infusionState_t::ALARM_COMPLETED) {
      //   infusionState = infusionState_t::ALARM_VOLUME_EXCEEDED;
      // }

    } else if (dropSensorState == 0) {/*nothing*/}
  } 
}

/**
 * Update dripping parameters
 * @param none
 * @return none
 */
void IRAM_ATTR dripCountUpdateISR() {
  // Checking for no drop for 20s
  static int timeWithNoDrop;
  int dropSensorState = dropSensor.getStateRaw();
  if (dropSensorState == 0) {
    timeWithNoDrop++;
    if (timeWithNoDrop >= 20000) {
      // reset these values
      firstDropDetected = false;
      timeBtw2Drops = UINT_MAX;

      // if ((infusionState == infusionState_t::ALARM_COMPLETED) || 
      //     (infusionState == infusionState_t::ALARM_VOLUME_EXCEEDED)) {
      //       infusionState = infusionState_t::NOT_STARTED;
      //     }

      // // infusion is still in progress but we cannot detect drops for 20s,
      // // something must be wrong, sound the alarm
      // if (infusionState == infusionState_t::IN_PROGRESS) {
      //   infusionState = infusionState_t::ALARM_STOPPED;

      // // prevent infusion time goto 0
      // lockInfusionStartTime = true;
      // }
    }
  } else {
    timeWithNoDrop = 0;
  }

  // get latest value of dripRate
  // explain: dripRate = 60 seconds / time between 2 consecutive drops
  // NOTE: this needs to be done in timer interrupt
  // TODO: use double division for precise calculation
  dripRate = 60000 / timeBtw2Drops;
}

/**
 * Toggle LED everytime a drop is detected
 * @param none
 * @return none
 */
void dropDetectedLEDTask(void * arg) {
  for(;;) {
    // toggle LED
    if (turnOnLed) {
      digitalWrite(DROP_SENSOR_LED_PIN, LOW);   // reversed because the LED is pull up
      vTaskDelay(50);
      digitalWrite(DROP_SENSOR_LED_PIN, HIGH);  // reversed because the LED is pull up
      turnOnLed = false;
    }

    while (!turnOnLed) {
      // free the CPU
      vTaskDelay(50);
    }
  }
}

/**
 * Refresh Epaper display every 1 second
 * @param none
 * @return none
 */
void refreshDisplayTask(void * arg) {
  for(;;) {
    static char rateGtt_buf[10];
    sprintf(rateGtt_buf, "%d", dripRate);
    dripRateGttBox.print_text(rateGtt_buf, font_xl);

    // static char rateMLh_buf[10];
    // // TODO: drip rate in mL/h
    // sprintf(rateMLh_buf, "%d", dripRate);
    // dripRateMLhBox.print_text(rateMLh_buf, font_xl);

    // free the CPU
    vTaskDelay(1000);
  }
}

void displayRateString() {
  display.setRotation(4);
  display.setFont(&FreeSansBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;

  display.getTextBounds(rateGttString, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = 50;
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(rateGttString);
  } while (display.nextPage());

// TODO: refresh 2 strings at the same time
  // display.getTextBounds(rateMLhString, 0, 0, &tbx, &tby, &tbw, &tbh);
  // // center bounding box by transposition of origin:
  // x = ((display.width() - tbw) / 2) - tbx;
  // y = 120;
  // display.setFullWindow();
  // display.firstPage();
  // do {
  //   // display.fillScreen(GxEPD_WHITE);
  //   display.setCursor(x, y);
  //   display.print(rateMLhString);
  // } while (display.nextPage());
}