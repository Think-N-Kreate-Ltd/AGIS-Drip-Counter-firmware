#include <DC_Display.h>
#include <DC_Commons.h>
// select the display class and display driver class in the following file (new style):
#include "GxEPD2_display_selection_new_style.h"

fonts font_xl = {&FreeSansBold18pt7b, 9, 19};

// Initializing the boxes
partial_box dripRateBox = {0, 0, display.width(), display.height()};

/**
 * Attach Epaper display with SPI pins and set display parameters
 * @param none
 * @return none
 */
void displayInit() {
  SPI.end(); // release standard SPI pins, e.g. SCK(18), MISO(19), MOSI(23), SS(5)
  SPI.begin(SPI_EPD_CLK, SPI_EPD_MISO, SPI_EPD_MOSI, SPI_EPD_CS); // map and init SPI pins SCK(13), MISO(12), MOSI(14), SS(15)
  display.init(0UL, true);
  display.setRotation(2);
  display.setFont(font_xl.font);
  display.setTextColor(GxEPD_BLACK);
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
  uint16_t mlh_y = gttString_y + 50;

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
}

