#include <Arduino.h>
#include <LovyanGFX.hpp>
#include "lgfx_ESP32_2432S028.h"
#include <SPI.h>

using Action = void(&)(LGFX &lcd);

void nop(LGFX &lcd){};

void calibrateTouchPad(LGFX &lcd)
  {
    lcd.fillScreen(TFT_BLACK);
    lcd.setTextSize((std::max(lcd.width(), lcd.height()) + 255) >> 8);

    //if (lcd.width() < lcd.height()) lcd.setRotation(lcd.getRotation() ^ 1);

    // Draws guidance text on the screen
    lcd.setTextDatum(textdatum_t::middle_center);
    lcd.setTextSize(1.0);
    lcd.drawString("touch the arrow marker", lcd.width()>>1, lcd.height() >> 1);
    lcd.setTextDatum(textdatum_t::top_left);

    // When using touch, perform calibration. Touch the tips of the arrows 
    // that appear in the four corners of the screen in sequence.
    uint16_t fg = TFT_WHITE;
    uint16_t bg = TFT_BLACK;
    if (lcd.isEPD()) std::swap(fg, bg);
    uint16_t caldata[8];
    lcd.calibrateTouch(caldata, fg, bg, std::max(lcd.width(), lcd.height()) >> 3);
    //lcd.calibrateTouch(nullptr, fg, bg, 20);
    Serial.printf(R"(
Raw Touch Values
----------------
x0 = %4d y0 =%4d
x1 = %4d y1 =%4d
x2 = %4d y2 =%4d
x3 = %4d y3 =%4d 
)", caldata[0], caldata[1], caldata[2], caldata[3], 
    caldata[4], caldata[5], caldata[6], caldata[7]);
    
    log_e("==> done");
  }


/**
 * Draw a framed crosshair in portrait orientation
*/
void framedCrosshair(LGFX &lcd)
{
  char str[24]; 
  lcd.fillScreen(TFT_BLACK);
  lcd.drawRect(0,0, lcd.width(), lcd.height(), TFT_RED);
  lcd.drawLine(0,0, lcd.width(), lcd.height(), TFT_GREEN);
  lcd.drawLine(lcd.width(),0, 0, lcd.height(), TFT_BLUE);
  lcd.fillRect(0,0, 20, 20, TFT_GREEN);
  lcd.fillRect(lcd.width()-10,lcd.height()-10, 10, 10, TFT_RED);;
  lcd.setTextSize(1.0);
  sprintf(str, "(0,0) origin, rot=%d", lcd.getRotation());
  lcd.drawString(str, 25,0);
}

/**
 * Draw a grid 20 x 20 
*/
void grid(LGFX &lcd)
{
  int x = 0, y = 0, d = 20;
  lcd.fillScreen(TFT_BLACK);
  while (y < lcd.height())
  {
    lcd.drawLine(0, y, lcd.width(), y, TFT_WHITE);
    y += d;
  }

  while (x < lcd.width())
  {
    lcd.drawLine(x, 0, x, lcd.height(), TFT_WHITE);
    x += d;
  }
  
}

/**
 * Show some facts about the display
*/
void lcdInfo(LGFX &lcd)
{
  Serial.printf(R"(
LCD Info
--------
width x height = %d x %d 
rotation       = %d 
color depth    = %d 
text size X    = %4.2f
text size Y    = %4.2f
)", lcd.width(), lcd.height(), lcd.getRotation(), lcd.getColorDepth(),
    lcd.getTextSizeX(), lcd.getTextSizeY());
  Serial.printf("\n");
}


GFXfont defaultFont = fonts::DejaVu18;

/**
 * Initialize display and call the greeting function.
 * The default for greeting is nop(). To calibrate the 
 * touchscreen call it as initDisplay(lcd, calibrateTouchScreen).
 * The greeting function takes as argument the passed lcd
*/
void initDisplay(LGFX &lcd, uint8_t rotation=0, GFXfont *theFont=&defaultFont, Action greet=nop)
  {
    lcd.begin() ? log_i("--> done") : log_i("--> failed");
    lcd.clear();
    lcd.setTextSize(1.0);
    lcd.setTextDatum(lgfx::textdatum::TL_DATUM);
    lcd.setFont(theFont);
    lcd.setRotation(rotation);
    lcd.setBrightness(255);
    greet(lcd);
    log_i("==> done");
  }
