#ifndef B6F3EDF6_26E6_475A_8BCD_2F6D61486F3D
#define B6F3EDF6_26E6_475A_8BCD_2F6D61486F3D

// uncomment next line to use class GFX of library GFX_Root instead of Adafruit_GFX
//#include <GFX.h>

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <DC_Commons.h>

struct fonts {
  const GFXfont* font;
  int width;
  int height;
};

struct partial_box {
  int left, top, width, height;

  // void clear(bool black){
  //   display.setPartialWindow(left, top, width, height);
  //   display.firstPage();
  //   do{
  //     if(black) display.fillRect(left, top, width, height, GxEPD_BLACK);
  //     else display.fillRect(left, top, width, height, GxEPD_WHITE);
  //   }
  //   while (display.nextPage());
  // }
};

extern fonts font_xl;

// Initializing the boxes
extern partial_box dripRateBox;

void displayInit();
void startScreen();
void printRates(struct partial_box box, String rateGtt_str, String rateMLh_str, fonts f);
void powerOffScreen();

#endif /* B6F3EDF6_26E6_475A_8BCD_2F6D61486F3D */
