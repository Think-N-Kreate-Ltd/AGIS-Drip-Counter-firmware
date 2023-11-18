#include <Arduino.h>
#include <ezButton.h>
#include <Wire.h>
#include <esp_log.h>
#include <DC_Display.h>
#include <DC_Commons.h>
#include <DC_Utilities.h>
#include <DC_Logging.h>
#include <DC_I2C.h>
#include <semphr.h>

/*Variables for monitoring dripping parameters*/
ezButton dropSensor(DROP_SENSOR_PIN);     // create ezButton object that attaches to drop sensor pin
volatile unsigned int numDrops = 0;       // for counting the number of drops
volatile unsigned int dripRate = 0;       // for calculating the drip rate
volatile unsigned int timeBtw2Drops = UINT_MAX; // i.e. no more drop recently
volatile bool firstDropDetected = false;  // to check when we receive the 1st drop

/*Variables related to infusion*/
unsigned int dropFactor = 0;             // unset value, required user selection
volatile unsigned int infusedVolume_x100 = 0;  // 100 times larger than actual value, unit: mL
bool dropFactorConfirmed = false;         // TRUE when drop factor is selected

/*Timer pointers*/
hw_timer_t *Timer0_cfg = NULL; // create a pointer for timer0
hw_timer_t *Timer1_cfg = NULL; // create a pointer for timer1

/*Other variables*/
volatile bool turnOnLed = false;          // used to turn on the LED for a short time
volatile bool enablePowerOff = false;
volatile unsigned long powerButtonHoldCount = 0;
ezButton powerButton(LATCH_IO_PIN);
ezButton userButton(USER_BUTTON_PIN);     // to select Drop Factor and enable other functions in the future
button_state_t userButtonState = button_state_t::IDLE;
unsigned long deviceLastActiveTime;       // used for auto-off function

/*Variables related to I2C*/

/*Mutex to keep different tasks from controlling the display at the same time*/
SemaphoreHandle_t  displayMutex;

/*Task handles*/
TaskHandle_t refreshDisplayTaskHandle;
TaskHandle_t processI2CCommandsTaskHandle;
TaskHandle_t dropDetectedLEDTaskHandle;
TaskHandle_t monitorBatteryTaskHandle;

/*Function prototypes*/
void IRAM_ATTR dropSensorISR();
void IRAM_ATTR dripCountUpdateISR();
void dropDetectedLEDTask(void *);
void refreshDisplayTask(void *);
void powerOffTask(void *);
void monitorBatteryTask(void *);
void monitorBatteryChargeStatusTask(void * arg);
void IRAM_ATTR buttonsPressedISR();
void powerOffTask(void *);
void processI2CCommandsTask(void * arg);
void dropFactorSelectionTask(void * arg);

void setup() {
  Serial.begin(115200);

  /*GPIO setup*/
  // for drop sensor
  pinMode(DROP_SENSOR_PIN, INPUT);
  pinMode(DROP_SENSOR_LED_PIN, OUTPUT);
  digitalWrite(DROP_SENSOR_LED_PIN, HIGH);    // initially OFF
  pinMode(DROP_SENSOR_VCC_EN_PIN, OUTPUT);
  digitalWrite(DROP_SENSOR_VCC_EN_PIN, LOW);  // initially OFF

  // for battery monitoring
  pinMode(BATT_ADC_ENABLE_PIN, OUTPUT);
  pinMode(BATT_ADC_PIN, INPUT);
  analogReadResolution(12);  // 12bit ADC
  digitalWrite(BATT_ADC_ENABLE_PIN, HIGH);     // turn off PMOS to disconnect voltage divider
  pinMode(BATT_CHGb_PIN, INPUT);
  pinMode(BATT_STDBYb_PIN, INPUT);

  /*Initialize Epaper display and show welcome screen*/
  displayInit();
  startScreen();

  /*I2C initialization for sending out data, e.g. to AGIS*/
  I2CDevice.i2cInit();

  /*Setup for timer0*/
  Timer0_cfg = timerBegin(0, 80, true);
  timerAttachInterrupt(Timer0_cfg, &dripCountUpdateISR,
                       false);
  timerAlarmWrite(Timer0_cfg, 1000, true); // time = 80*1000/80,000,000 = 1ms
  timerAlarmDisable(Timer0_cfg);  // initially disable interrupt

  /*Setup for timer1*/
  Timer1_cfg = timerBegin(1, 80, true);
  timerAttachInterrupt(Timer1_cfg, &buttonsPressedISR,
                       false);
  timerAlarmWrite(Timer1_cfg, 1000, true); // time = 80*1000/80,000,000 = 1ms
  timerAlarmEnable(Timer1_cfg);

  /*Initialize mutex*/
  displayMutex = xSemaphoreCreateMutex();
  assert(displayMutex);

  /*Create a task for software power off*/
  xTaskCreate(powerOffTask,
              "Power Off Task",
              4096,
              NULL,
              configMAX_PRIORITIES-1,      // HIGHEST PRIORITY, to make sure this task can preempt other tasks to shut down
              NULL);

  /*Create a task for monitoring battery charge status*/
  xTaskCreate(monitorBatteryChargeStatusTask,
              "Monitor Battery Charge Status Task",
              4096,
              NULL,
              configMAX_PRIORITIES-2,      // make sure this will run first to show the battery level
              NULL);

  /*Create a task for drop factor selection*/
  xTaskCreate(dropFactorSelectionTask,
              "Drop Factor Selection Task",
              4096,
              NULL,
              configMAX_PRIORITIES-3,      // lower priority than powerOffTask, this needs to complete once before other tasks can run
              NULL);

  /*Create a task for processing I2C commands*/
  xTaskCreate(processI2CCommandsTask,
              "Process I2C Commands Task",
              4096,
              NULL,
              configMAX_PRIORITIES-4,      // this should be a very high priority task
              &processI2CCommandsTaskHandle);
  vTaskSuspend(processI2CCommandsTaskHandle);

  /*Create a task for refreshing Epaper display*/
  xTaskCreate(refreshDisplayTask,
              "Refresh Display Task",
              4096,
              NULL,
              1,
              &refreshDisplayTaskHandle);
  vTaskSuspend(refreshDisplayTaskHandle);

  /*Create a task for toggling LED everytime a drop is detected*/
  xTaskCreate(dropDetectedLEDTask,
              "Drop Detected LED Task",
              4096,
              NULL,
              0,
              &dropDetectedLEDTaskHandle);
  vTaskSuspend(dropDetectedLEDTaskHandle);

  /*Create a task for monitoring battery level*/
  xTaskCreate(monitorBatteryTask,
              "Monitor Battery Task",
              4096,
              NULL,
              0,
              &monitorBatteryTaskHandle);
  vTaskSuspend(monitorBatteryTaskHandle);

  // Record this as device last active time
  deviceLastActiveTime = millis();
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
      if (!firstDropDetected){
        firstDropDetected = true;
        lastDropTime = -9999; // prevent timeBtw2Drops become inf

        // if (!lockInfusionStartTime) {
        //   // mark this as starting time of infusion
        //   infusionStartTime = millis();
        // }
      }
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
    if (timeWithNoDrop >= NO_DROP_ALARM_TIME) {
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

    // TODO: User button can be used to start monitoring infusion.
    // When user button is pressed, auto-off function will be disabled. 
    // E.g. when there is an alarm, it should remain alarm and not auto-off.
  } else {
    timeWithNoDrop = 0;
    deviceLastActiveTime = millis();
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
 * Refresh Epaper display every `DISPLAY_REFRESH_TIME` second
 * @param none
 * @return none
 */
void refreshDisplayTask(void * arg) {
  for(;;) {
    xSemaphoreTake(displayMutex, portMAX_DELAY);
    // TODO: only refresh display if new drop detected and rate is different
    static char rateGtt_buf[10];
    static char rateMLh_buf[10];
    sprintf(rateGtt_buf, "%d", dripRate);
    sprintf(rateMLh_buf, "%d", dripRate * (60 / dropFactor));

    printRates(dripRateBox, rateGtt_buf, rateMLh_buf, font_xl);
    xSemaphoreGive(displayMutex);

    // free the CPU
    vTaskDelay(DISPLAY_REFRESH_TIME);
  }
}

/**
 * Task to display the power off screen, clean up, and then power off
 * Also check device last active time to auto-off
 * @param none
 * @return none
 */
void powerOffTask(void * arg) {
  for(;;) {
    if(enablePowerOff || (millis() - deviceLastActiveTime > AUTO_OFF_TIME)) {
      /*Cleaning up before power off*/
      // Disable timers
      timerAlarmDisable(Timer0_cfg);
      timerAlarmDisable(Timer1_cfg);
      // Cut-off power to sensor
      digitalWrite(DROP_SENSOR_VCC_EN_PIN, LOW);
      // etc...

      /*Notify that device is about to be powered off*/
      xSemaphoreTake(displayMutex, portMAX_DELAY);
      ESP_LOGD(POWER_TAG, "Power off signal received. Cleaning up...");
      powerOffScreen();

      /*power off now*/
      ESP_LOGD(POWER_TAG, "Power off now");
      pinMode(LATCH_IO_PIN, OUTPUT);
      digitalWrite(LATCH_IO_PIN, LOW);

      // It will take 2s to fully discharge the capacitor to power off.
      // Block other tasks from controlling the display during this time.
      while (true) {
        vTaskDelay(100);
      }

      // The program should never reach here, since it is already powered off.
      xSemaphoreGive(displayMutex);
    }

    // free the CPU
    vTaskDelay(10);
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
    /*Get battery voltage to estimate remaining percentage*/
    float batteryVoltage = getBatteryVoltage();

    /*Battery low check*/
    xSemaphoreTake(displayMutex, portMAX_DELAY);
    if (batteryVoltage < BATTERY_LOW_THRESHOLD_VOLTAGE) {
      displayPopup(BATTERY_LOW_STRING);
      vTaskDelay(POPUP_WINDOW_HOLD_TIME);
    }
    // TODO: mutex will not be released until vTaskDelay() finishes.
    // If user hits the power off button during this delay time, the
    // display will not turn off immediately. How to improve this?
    xSemaphoreGive(displayMutex);

    // free the CPU
    vTaskDelay(BATTERY_MONITOR_TIME);
  }
}

/**
 * Monitor battery charge status and update battery symbol accordingly
 * @param none
 * @return none
 */
// TODO: maybe it's better to trigger this based on interrupt?
void monitorBatteryChargeStatusTask(void * arg) {
  for(;;) {
    /*Get battery charging status*/
    static charge_status_t previousChargeStatus = charge_status_t::UNKNOWN;  // initial value to have smth to compare
    charge_status_t chargeStatus = getChargeStatus();

    /*Refresh battery symbol based on battery voltage and charge status*/
    // Only redraw if charge status has changed
    if (chargeStatus != previousChargeStatus) {
      /*Get battery voltage to estimate remaining percentage*/
      float batteryVoltage = getBatteryVoltage();

      xSemaphoreTake(displayMutex, portMAX_DELAY);
      drawBatteryBitmap(batteryVoltage, chargeStatus);
      xSemaphoreGive(displayMutex);

      previousChargeStatus = chargeStatus;
    }

    // free the CPU
    vTaskDelay(BATTERY_CHARGE_STATUS_TIME);
  }
}

/**
 * Check whether power button or user button is pressed.
 * Properly shut down the application when power button is pressed and hold
 * @param none
 * @return none
 */
void IRAM_ATTR buttonsPressedISR() {
  /*Check powerButton state*/
  powerButton.loop();
  if(!powerButton.getState() && !enablePowerOff) {
    powerButtonHoldCount++;
  }

  if(powerButtonHoldCount >= 50) {
    // signal task to power off
    enablePowerOff = true;
    powerButtonHoldCount = 0;
  }

  /*Check userButton state*/
  // 0: wating for 1st press
  // 1: waiting for 1st release
  // 2: 1st released, checking single or double press
  // 3: waiting for 2nd release
  static uint8_t buttonState = 0;
  static unsigned int lastPressedTime;
  userButton.loop();

  if (buttonState == 0) {
    if (userButton.isPressed()) {
      // 1st press
      buttonState = 1;
      lastPressedTime = millis();
    }
  }
  else if (buttonState == 1) {
    // wait for stable release
    if (userButton.isReleased()) {
      buttonState = 2;
      lastPressedTime = millis();
    }
  }
  else if (buttonState == 2) {
    // differentiate between single and double press
    if (millis() - lastPressedTime > DOUBLE_PRESS_TIMEOUT) {
      // single press
      userButtonState = button_state_t::SINGLE_PRESS;
      buttonState = 0;
      lastPressedTime = millis();
    }
    else if (userButton.isPressed()) {
      // double press
      userButtonState = button_state_t::DOUBLE_PRESS;
      buttonState = 3;
      lastPressedTime = millis();
    }
  }
  else if (buttonState == 3) {
    // wait for stable release 2
    if (userButton.isReleased()) {
      buttonState = 0;
      lastPressedTime = millis();
    }
  }
}

/**
 * Task to process incoming I2C commands from external devices
 * @param none
 * @return none
 */
void processI2CCommandsTask(void * arg) {
  for(;;) {

    if (pendingCommandLength) {
      I2CDevice.process(pendingCommand, pendingCommandLength);
      pendingCommandLength = 0;    // process once
    }

    // free the CPU
    vTaskDelay(10);
  }
}

/**
 * Process User Button press to change Drop Factor and confirm
 * @param none
 * @return none
 */
void dropFactorSelectionTask(void * arg) {
  for(;;) {
    xSemaphoreTake(displayMutex, portMAX_DELAY);
    static uint8_t index = 2;        // correspond to initial drop factor to be displayed: 20 gtt/mL
    static uint8_t activeDropFactor = dropFactorArray[index];   // used to display, not yet confirmed

    // Draw the screen once so that we don't have to redraw every task call
    static bool drawOnce = false;
    if (!drawOnce) {
      dropFactorSelectionScreen(activeDropFactor);
      drawOnce = true;
    }

    // Check user button press to determine the confirmed drop factor:
    if (userButtonState == button_state_t::SINGLE_PRESS) {
      userButtonState = button_state_t::IDLE;  // do this immediately after enter block, otherwise there could be missed button press

      // change to the next drop factor
      index++;
      if (index == sizeof(dropFactorArray) / sizeof(dropFactorArray[0])) {
        index = 0;
      }
      activeDropFactor = dropFactorArray[index];
      dropFactorSelectionScreen(activeDropFactor);
      deviceLastActiveTime = millis();
    }
    else if (userButtonState == button_state_t::DOUBLE_PRESS) {
      userButtonState = button_state_t::IDLE;

      // drop factor is confirmed
      dropFactor = activeDropFactor;

      // draw the corresponding drop factor symbol on status bar
      drawDropFactorBitmap(dropFactor);

      /*Now we can enable some peripherals and initialization*/
      // Enable power for sensor
      digitalWrite(DROP_SENSOR_VCC_EN_PIN, HIGH);
      // Setup for sensor interrupt
      attachInterrupt(DROP_SENSOR_PIN, &dropSensorISR, CHANGE);  // call interrupt when state change
      // Enable timer interrupt
      timerAlarmEnable(Timer0_cfg);

      // Enable other tasks that were initially suspended
      vTaskResume(refreshDisplayTaskHandle);
      vTaskResume(processI2CCommandsTaskHandle);
      vTaskResume(dropDetectedLEDTaskHandle);
      vTaskResume(monitorBatteryTaskHandle);

      dropFactorConfirmed = true;
      deviceLastActiveTime = millis();
    }
    else if (userButtonState == button_state_t::IDLE) {
      // waiting for user to confirm drop factor
      // do nothing in here
    }

    xSemaphoreGive(displayMutex);

    // Once drop factor is confirmed, we no longer need this task running
    if (dropFactorConfirmed) {
      vTaskSuspend(NULL);
    }

    // free the CPU
    vTaskDelay(10);
  }
}