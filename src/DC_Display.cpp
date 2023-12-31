#include <DC_Display.h>
#include <DC_Commons.h>
// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"
#include <bitmaps/Battery_Bitmaps_16x10.h>
#include <bitmaps/Drop_Factor_Bitmaps_25x10.h>

#define STATUS_BAR_HEIGHT                   10
#define BATTERY_SYMBOL_WIDTH                16
#define DROP_FACTOR_SYMBOL_WIDTH            25
#define BATTERY_LARGE_SYMBOL_WIDTH          51
#define BATTERY_LARGE_SYMBOL_HEIGHT         30

static const char* DISPLAY_TAG = "DISPLAY";

// enum battery_symbol_t {
//   BATTERY_100_PERCENT,
//   BATTERY_75_PERCENT,
//   BATTERY_50_PERCENT,
//   BATTERY_25_PERCENT,
//   BATTERY_LOW,
//   BATTERY_CHARGING,
//   BATTERY_CHARGE_COMPLETED
// };

fonts font_xl = {&FreeSansBold18pt7b, 9, 19};

// Initializing the boxes
partial_box dripRateBox = {0, STATUS_BAR_HEIGHT + 1, display.width(), display.height() - (STATUS_BAR_HEIGHT + 1)};

/**
 * Attach Epaper display with SPI pins and set display parameters
 * @param none
 * @return none
 */
void displayInit() {
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.begin(SPI_EPD_CLK, SPI_EPD_MISO, SPI_EPD_MOSI, SPI_EPD_CS); // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)
  display.init(0, true); // the 1st argument is the baudrate for display debug via serial interface
}

/**
 * Refresh display with updated value of drip rate in 2 units: gtt/m and mL/h
 * @param none
 * @return none
 */
void printRates(struct partial_box box, const String &rateGtt_str, String rateMLh_str, fonts f) {
  display.setPartialWindow(box.left, box.top, box.width, box.height);
  display.setTextColor(GxEPD_BLACK);

  int16_t tbx, tby;
  uint16_t tbw, tbh;

  display.setFont(f.font);
  display.getTextBounds(rateGtt_str, box.left, box.top, &tbx, &tby, &tbw, &tbh);
  uint16_t gtt_x = ((display.width() - tbw) / 2) - tbx;
  uint16_t gtt_y = box.top + 30;

  display.setFont(&FreeSansBold9pt7b);
  display.getTextBounds(GTT_STRING, box.left, box.top, &tbx, &tby, &tbw, &tbh);
  uint16_t gttString_x = ((display.width() - tbw) / 2) - tbx;
  uint16_t gttString_y = gtt_y + 20;

  display.setFont(f.font);
  display.getTextBounds(rateMLh_str, box.left, box.top, &tbx, &tby, &tbw, &tbh);
  uint16_t mlh_x = ((display.width() - tbw) / 2) - tbx;
  uint16_t mlh_y = gttString_y + 40;

  display.setFont(&FreeSansBold9pt7b);
  display.getTextBounds(MLH_STRING, box.left, box.top, &tbx, &tby, &tbw, &tbh);
  uint16_t mlhString_x = ((display.width() - tbw) / 2) - tbx;
  uint16_t mlhString_y = mlh_y + 20;

  // TODO: why partial update below takes so long (> 1000 ms)?
  // int previous = millis();

  display.firstPage();
  do{
    display.fillRect(box.left, box.top, box.width, box.height, GxEPD_WHITE);

    // display rate in gtt/m
    display.setFont(f.font);
    display.setCursor(gtt_x, gtt_y);
    display.print(rateGtt_str);

    // display "gtt/m" text
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(gttString_x, gttString_y);
    display.print(GTT_STRING);

    // display rate in mL/h
    display.setFont(f.font);
    display.setCursor(mlh_x, mlh_y);
    display.print(rateMLh_str);

    // display "mL/h" text
    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(mlhString_x, mlhString_y);
    display.print(MLH_STRING);
  }
  while (display.nextPage());

  // ESP_LOGD("TEST", "took %d ms\n", millis() - previous);
}

/**
 * Display welcome screen with some texts, also to refresh the display
 * @param none
 * @return none
 */
void startScreen() {
  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(START_SCREEN_STRING, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(START_SCREEN_STRING);

#ifdef SHOW_VERSION
    display.setCursor(0, 100);
    display.print(VERSION_STRING);
#endif

  } while (display.nextPage());

  // Clear the screen to avoid overlap
  display.clearScreen();
  // May need to wait a bit to fully clear the display
  // delay(500);
}

/**
 * Display screen to notify that device is about to be turned off
 * @param none
 * @return none
 */
void powerOffScreen() {
  display.clearScreen(); // without this, the display will not be clear in very rare cases

  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(POWER_OFF_SCREEN_STRING, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(POWER_OFF_SCREEN_STRING);
  } while (display.nextPage());

  // Clear the screen to avoid overlap
  display.clearScreen(0x00);  // all black
  // May need to wait a bit to fully clear the display
  // delay(500);
}

void drawBatteryBitmap(float voltage, charge_status_t status) {
  /*Select corresponding bitmap*/
  uint8_t *bitmap;
  if (status == charge_status_t::NOT_CHARGING) {
    // Display battery symbol with closest remaining level: 100%, 75%, 50%, 25%
    if (voltage >= BATTERY_100_PERCENT_THRESHOLD_VOLTAGE) {
      bitmap = (uint8_t*)batteryBitmap_100percent_16x10;
    }
    else if (voltage >= BATTERY_75_PERCENT_THRESHOLD_VOLTAGE) {
      bitmap = (uint8_t*)batteryBitmap_75percent_16x10;
    }
    else if (voltage >= BATTERY_50_PERCENT_THRESHOLD_VOLTAGE) {
      bitmap = (uint8_t*)batteryBitmap_50percent_16x10;
    }
    else if (voltage >= BATTERY_25_PERCENT_THRESHOLD_VOLTAGE) {
      bitmap = (uint8_t*)batteryBitmap_25percent_16x10;
    }
    else {
      // Low battery
      bitmap = (uint8_t*)batteryBitmap_low_16x10;
    }
  }
  else if (status == charge_status_t::CHARGING) {
    bitmap = (uint8_t*)batteryBitmap_charging_16x10;
  }
  else if (status == charge_status_t::CHARGE_COMPLETED) {
    bitmap = (uint8_t*)batteryBitmap_charge_completed_16x10;
  }
  else {
    // Unknown charge status, this will never happen
    // Do nothing
  }

  /*Display the selected bitmap*/
  uint8_t x = display.width() - BATTERY_SYMBOL_WIDTH;
  uint8_t y = 1;
  display.setPartialWindow(x, y, BATTERY_SYMBOL_WIDTH, STATUS_BAR_HEIGHT);

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(x, y, bitmap, BATTERY_SYMBOL_WIDTH,
                               STATUS_BAR_HEIGHT, GxEPD_BLACK);
  } while (display.nextPage());

  ESP_LOGD(DISPLAY_TAG, "Battery symbol drawn");
}

void drawDropFactorBitmap(uint8_t dropFactor) {
  /*Select corresponding bitmap*/
  uint8_t *bitmap;

  if (dropFactor == 10) {
      bitmap = (uint8_t*)dropFactorBitmap_10_25x10;
  }
  else if (dropFactor == 15) {
      bitmap = (uint8_t*)dropFactorBitmap_15_25x10;
  }
  else if (dropFactor == 20) {
      bitmap = (uint8_t*)dropFactorBitmap_20_25x10;
  }
  else if (dropFactor == 60) {
      bitmap = (uint8_t*)dropFactorBitmap_60_25x10;
  }
  else {
    // should never reach here
    ESP_LOGD(DISPLAY_TAG, "Drop Factor symbol doesn't exist");
  }

  /*Display the selected bitmap*/
  uint8_t x = 1;
  uint8_t y = 1;
  display.setPartialWindow(x, y, DROP_FACTOR_SYMBOL_WIDTH, STATUS_BAR_HEIGHT);

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(x, y, bitmap, DROP_FACTOR_SYMBOL_WIDTH,
                               STATUS_BAR_HEIGHT, GxEPD_BLACK);
  } while (display.nextPage());

  ESP_LOGD(DISPLAY_TAG, "Drop Factor symbol drawn");
}

/// @brief Display a pop-up window (regtanle shape with boundary) to show some important message
/// @param message Message to be shown within the pop-up window. The message length should be very short!
void displayPopup(const char * message) {
  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);

  // Explain:
  // Pop-up window is basically (entire window - status bar)

  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(message, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;

  uint16_t popupWidth = display.width();
  uint16_t popupHeight = display.height() - (STATUS_BAR_HEIGHT + 1);
  uint16_t popup_y = STATUS_BAR_HEIGHT + 1;

  display.setPartialWindow(0, popup_y, popupWidth, popupHeight);

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.fillRect(0, popup_y, popupWidth, popupHeight, GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(message);
  } while (display.nextPage());
}

/// @brief Drop Factor selection screen: 4 selectable drop factors
void dropFactorSelectionScreen(uint8_t activeDropFactor) {
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);

  int16_t tbx, tby;
  uint16_t tbw, tbh;

  display.setFont(&FreeMonoBold9pt7b);
  display.getTextBounds(DROP_FACTOR_SELECTION_STRING, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = STATUS_BAR_HEIGHT + 1 + tbh / 2;

  display.setFont(&FreeSansBold18pt7b);
  static char activeDropFactor_buf[3];
  sprintf(activeDropFactor_buf, "%d", activeDropFactor);
  display.getTextBounds(activeDropFactor_buf, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t dropFactor_x = ((display.width() - tbw) / 2) - tbx;
  uint16_t dropFactor_y = y + 60;
  uint16_t dropFactor_tbh = tbh;

  display.getTextBounds(DROP_FACTOR_UNIT_STRING, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t dropFactorUnitString_x = ((display.width() - tbw) / 2) - tbx - 5;  // "- 5" to better align, don't know why
  uint16_t dropFactorUnitString_y = dropFactor_y + 30;

  display.setPartialWindow(0, STATUS_BAR_HEIGHT + 1, display.width(), display.height() - (STATUS_BAR_HEIGHT + 1));
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    // information texts
    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(x, y);
    display.print(DROP_FACTOR_SELECTION_STRING);

    display.setCursor(dropFactorUnitString_x, dropFactorUnitString_y);
    display.print(DROP_FACTOR_UNIT_STRING);

    // draw triangles to illustrate that the number can be changed
    uint8_t triangle_size = 8;
    uint8_t triangle_margin = 3;
    uint16_t triangle_left_x0 = triangle_margin;
    uint16_t triangle_left_y0 = dropFactor_y - (dropFactor_tbh / 2);
    display.fillTriangle(triangle_left_x0, triangle_left_y0,
                         triangle_left_x0 + triangle_size, triangle_left_y0 - triangle_size,
                         triangle_left_x0 + triangle_size, triangle_left_y0 + triangle_size,
                         GxEPD_BLACK);
    uint16_t triangle_right_x0 = display.width() - triangle_margin;
    uint16_t triangle_right_y0 = triangle_left_y0;
    display.fillTriangle(triangle_right_x0, triangle_right_y0,
                         triangle_right_x0 - triangle_size, triangle_right_y0 - triangle_size,
                         triangle_right_x0 - triangle_size, triangle_right_y0 + triangle_size,
                         GxEPD_BLACK);

    // drop factor number
    display.setFont(&FreeSansBold18pt7b);
    display.setCursor(dropFactor_x, dropFactor_y);
    display.print(activeDropFactor_buf);
  } while (display.nextPage());
}

void drawBatteryBitmapCharging(charge_status_t status) {
  uint8_t *bitmap;

  /*Select corresponding bitmap*/
  if (status == charge_status_t::CHARGING) {
    bitmap = (uint8_t*)batteryBitmap_charging_large_51x30;
  }
  else if (status == charge_status_t::CHARGE_COMPLETED) {
    bitmap = (uint8_t*)batteryBitmap_charge_completed_large_51x30;
  }
  else {
    // Unknown charge status, this will never happen
    // Do nothing
  }

  /*Display the selected bitmap*/
  uint8_t x = (display.width() - BATTERY_LARGE_SYMBOL_WIDTH) / 2;
  uint8_t y = (display.height() - BATTERY_LARGE_SYMBOL_HEIGHT) / 2;
  display.setFullWindow();

  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.drawInvertedBitmap(x, y, bitmap, 51, 30, GxEPD_BLACK);
  } while (display.nextPage());

  ESP_LOGD(DISPLAY_TAG, "Battery charging symbol drawn");
}