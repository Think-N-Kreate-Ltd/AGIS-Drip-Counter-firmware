#include <DC_Display.h>
#include <DC_Commons.h>
// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"
#include <bitmaps/Battery_Bitmaps_16x10.h>

#define STATUS_BAR_HEIGHT                   10
#define BATTERY_SYMBOL_WIDTH                16

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
void printRates(struct partial_box box, String rateGtt_str, String rateMLh_str, fonts f) {
  display.setPartialWindow(box.left, box.top, box.width, box.height);
  display.setTextColor(GxEPD_BLACK);

  int16_t tbx, tby;
  uint16_t tbw, tbh;

  display.getTextBounds(rateGtt_str, box.left, box.top, &tbx, &tby, &tbw, &tbh);
  uint16_t gtt_x = (box.left + box.width/2 - tbw - 1);
  uint16_t gtt_y = box.top + 30;

  display.getTextBounds(GTT_STRING, box.left, box.top, &tbx, &tby, &tbw, &tbh);
  uint16_t gttString_x = ((display.width() - tbw) / 2) - tbx;
  uint16_t gttString_y = gtt_y + 20;

  display.getTextBounds(rateMLh_str, box.left, box.top, &tbx, &tby, &tbw, &tbh);
  uint16_t mlh_x = (box.left + box.width/2 - tbw - 1);
  uint16_t mlh_y = gttString_y + 40;

  display.getTextBounds(MLH_STRING, box.left, box.top, &tbx, &tby, &tbw, &tbh);
  uint16_t mlhString_x = ((display.width() - tbw) / 2) - tbx;
  uint16_t mlhString_y = mlh_y + 20;

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
}

/**
 * Display welcome screen with some texts, also to refresh the display
 * @param none
 * @return none
 */
void startScreen() {
  display.setRotation(2);
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
  display.clearScreen();

  display.setRotation(2);
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
    if (voltage >= 4.0) bitmap = (uint8_t*)batteryBitmap_100percent_16x10;
    else if (voltage >= 3.9) bitmap = (uint8_t*)batteryBitmap_75percent_16x10;
    else if (voltage >= 3.8) bitmap = (uint8_t*)batteryBitmap_50percent_16x10;
    else if (voltage >= 3.7) bitmap = (uint8_t*)batteryBitmap_25percent_16x10;
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
}