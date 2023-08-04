#include <Arduino.h>
#include <ezButton.h>
#include <Wire.h>
#include <esp_log.h>
#include <DC_Display.h>
#include <DC_Commons.h>
#include <DC_Utilities.h>
#include <DC_Logging.h>
#include <DC_I2C.h>

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
hw_timer_t *Timer1_cfg = NULL; // create a pointer for timer1

/*Other variables*/
volatile bool turnOnLed = false;          // used to turn on the LED for a short time
volatile bool powerButtonPressed = false;
volatile bool powerButtonHold = false;
volatile bool displayPowerOffScreen = false;
volatile unsigned long powerButtonHoldCount = 0;
ezButton powerButton(LATCH_IO_PIN);

/*Function prototypes*/
void IRAM_ATTR dropSensorISR();
void IRAM_ATTR dripCountUpdateISR();
void dropDetectedLEDTask(void *);
void refreshDisplayTask(void *);
void powerOffDisplayTask(void *);
void monitorBatteryTask(void *);
void IRAM_ATTR powerOffISR();

void setup() {
  Serial.begin(115200);

  /*GPIO setup*/
  pinMode(DROP_SENSOR_PIN, INPUT);
  pinMode(DROP_SENSOR_LED_PIN, OUTPUT);
  digitalWrite(DROP_SENSOR_LED_PIN, HIGH); // prevent it initially turn on

  /*I2C initialization for sending out data, e.g. to AGIS*/
  DC_i2cInit();

  // pinMode(ADC_ENABLE_PIN, OUTPUT);
  // pinMode(ADC_PIN, INPUT);
  // analogReadResolution(12);  // 12bit ADC
  // digitalWrite(ADC_ENABLE_PIN, HIGH);     // initially, disable to save power

  /*Setup for sensor interrupt*/
  attachInterrupt(DROP_SENSOR_PIN, &dropSensorISR, CHANGE);  // call interrupt when state change

  /*Setup for timer0*/
  Timer0_cfg = timerBegin(0, 80, true);
  timerAttachInterrupt(Timer0_cfg, &dripCountUpdateISR,
                       false);
  timerAlarmWrite(Timer0_cfg, 1000, true); // time = 80*1000/80,000,000 = 1ms
  timerAlarmEnable(Timer0_cfg);

  /*Setup for timer1*/
  Timer1_cfg = timerBegin(1, 80, true);
  timerAttachInterrupt(Timer1_cfg, &powerOffISR,
                       false);
  timerAlarmWrite(Timer1_cfg, 1000, true); // time = 80*1000/80,000,000 = 1ms
  timerAlarmEnable(Timer1_cfg);

  /*Initialize Epaper display and show welcome screen*/
  displayInit();
  startScreen();
  delay(500);

  /*Create a task for toggling LED everytime a drop is detected*/
  xTaskCreate(dropDetectedLEDTask,
              "Drop Detected LED Task",
              4096,
              NULL,
              0,
              NULL);

  /*Create a task for refreshing Epaper display*/
  xTaskCreate(refreshDisplayTask,
              "Refresh Display Task",
              4096,
              NULL,
              1,
              NULL);

  /*Create a task for displaying shutdown screen*/
  xTaskCreate(powerOffDisplayTask,
              "Power Off Display Task",
              4096,
              NULL,
              2,      // higher priority than other display tasks
              NULL);

  /*Create a task for monitoring battery level*/
  // xTaskCreate(monitorBatteryTask,
  //             "Monitor Battery Task",
  //             4096,
  //             NULL,
  //             0,
  //             NULL);
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
    if(!powerButtonHold) {
      static char rateGtt_buf[10];
      static char rateMLh_buf[10];
      sprintf(rateGtt_buf, "%d", dripRate);
      sprintf(rateMLh_buf, "%d", dripRate * (60 / dropFactor));

      printRates(dripRateBox, rateGtt_buf, rateMLh_buf, font_xl);
    }

    // free the CPU
    vTaskDelay(DISPLAY_REFRESH_TIME);
  }
}

/**
 * Task to display the power off screen to notify user
 * @param none
 * @return none
 */
void powerOffDisplayTask(void * arg) {
  for(;;) {
    if(powerButtonHold) {
      ESP_LOGD(POWER_LOG_TAG, "Power off screen started");
      powerOffScreen();
      // vTaskDelay(2000);
      powerButtonHold = false;
      displayPowerOffScreen = true;  // to notify powerOffISR()
    }

    // free the CPU
    vTaskDelay(100);
  }
}

/**
 * Monitor battery level based on its remaining voltage
 * and alarm if the battery level is low
 * @param none
 * @return none
 */
void monitorBatteryTask(void * arg) {
  for(;;) {
    float batteryVoltage = getBatteryVoltage();
    // Serial.printf("Battery voltage: %f\n", batteryVoltage);
    Serial.printf("ADC value: %f\n", batteryVoltage);

    // free the CPU
    vTaskDelay(BATTERY_MONITOR_TIME);
  }
}

/**
 * Properly shut down the application when power button is pressed and hold
 * @param none
 * @return none
 */
void IRAM_ATTR powerOffISR() {

  powerButton.loop();
  if(!powerButton.getState()) {
    powerButtonHoldCount++;
  }

  if(powerButtonHoldCount >= 500 && !displayPowerOffScreen) {
    /*raise the flag to display power off screen but not shut down yet*/
    powerButtonHold = true;
  }
  
  if(powerButtonHoldCount >= 2000){
    /*clean up program before shutdown*/
    //TODO: what needs to be cleaned up?

    // ESP_LOGI(POWER_LOG_TAG, "Shut down now");

    /*shut down now*/
    pinMode(LATCH_IO_PIN, OUTPUT);
    digitalWrite(LATCH_IO_PIN, LOW);
  }

  if(powerButton.isReleased()) {
    powerButtonHoldCount = 0;
  }
}