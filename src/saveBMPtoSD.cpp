#include <Arduino.h>
#include <SD.h>
#include <LovyanGFX.hpp>
#include "lgfx_esp32-2432S028.h"

/**
 * Screenshot routines for RGB565 and RGB888 color model
 * adapted from LovyanGFX example 
 * https://github.com/lovyan03/LovyanGFX/tree/master/examples/Standard/SaveBMP
 * The order of the colors must be rotated to obtain 
 * the same colors in the file as on the screen. 
 */

/**
 * Helper function to explore the content 
 * of the color buffer
 */
void printBuf565(lgfx::rgb565_t* buf, int bufSize)
{
  int nTuples = bufSize / 2;
  for (int i = 0; i < nTuples; i++)
  {
    Serial.printf("%3d,%3d,%3d\n", buf->r5, buf->g6, buf->b5);
    buf++;
  }
}


/**
 * Rotate the colors to get the right sequence  
 * in the picture saved to SD card.
 * Because the color green, originally represented with 
 * 6 bits, is now stored in the bit field of RED with 
 * 5 bits, color information is lost. 
 */
void rotate_rgb565(lgfx::rgb565_t* buf, int bufSize)
{
  auto  rotateTuple = [](lgfx::rgb565_t* pRGB) 
        {  
          lgfx::rgb565_t tmp = *pRGB;
          pRGB->r5 = tmp.g6;
          pRGB->g6 = tmp.b5<<1;  // 1 bit of color information gets lost
          pRGB->b5 = tmp.r5;
        };
  int nTuples = bufSize / 2;
  for (int i = 0; i < nTuples; i++)
  {
    rotateTuple(buf);
    buf++;
  }  
}

/**
 * Saves the LCD screen to SD card in RGB565 format. 
 * The order of the colors must be rotated to obtain 
 * the same colors in the file as on the screen.
 * Because the color green, originally represented with 
 * 6 bits, is now stored in the bit field of RED with 
 * 5 bits, color information is lost. 
 */
bool saveBmpToSD_16bit(LGFX &lcd, const char *filename)
{
  bool result = false;
  File file = SD.open(filename, "w");
  if (file)
  {
    int width  = lcd.width();
    int height = lcd.height();

    int rowSize = (2 * width + 3) & ~ 3;

    lgfx::bitmap_header_t bmpheader;
    bmpheader.bfType = 0x4D42;
    bmpheader.bfSize = rowSize * height + sizeof(bmpheader);
    bmpheader.bfOffBits = sizeof(bmpheader);

    bmpheader.biSize = 40;
    bmpheader.biWidth = width;
    bmpheader.biHeight = height;
    bmpheader.biPlanes = 1;
    bmpheader.biBitCount = 16;
    bmpheader.biCompression = 3;

    file.write((std::uint8_t*)&bmpheader, sizeof(bmpheader));
    std::uint8_t buffer[rowSize];
    memset(&buffer[rowSize - 4], 0, 4);
    for (int y = lcd.height() - 1; y >= 0; y--)
    {
      lcd.readRect(0, y, lcd.width(), 1, (lgfx::rgb565_t*)buffer);
      //printBuf565((lgfx::rgb565_t*)buffer, sizeof(buffer));
      //rotate_rgb565((lgfx::rgb565_t*)buffer, sizeof(buffer));
      file.write(buffer, rowSize);
    }
    file.close();
    result = true;
  }
  else
  {
    Serial.print("error:file open failure\n");
  }
  return result;
}


/**
 * Helper function to explore the content 
 * of the color buffer
 */
void printBuf888(lgfx::rgb888_t* buf, int bufSize)
{
  int nTriples = bufSize / 3;
  for (int i = 0; i < nTriples; i++)
  {
    Serial.printf("%3d,%3d,%3d\n", buf->r, buf->g, buf->b);
    buf++;
  }
}


/**
 * Rotate the colors to get the right sequence  
 * in the picture saved to SD card
 */
void rotate_rgb888(lgfx::rgb888_t* buf, int bufSize)
{
  auto  rotateTriple = [](lgfx::rgb888_t* pRGB) 
        {  
          lgfx::rgb888_t tmp = *pRGB;
          pRGB->r = tmp.g;
          pRGB->g = tmp.b;
          pRGB->b = tmp.r;
        };
  int nTriples = bufSize / 3;
  for (int i = 0; i < nTriples; i++)
  {
    rotateTriple(buf);
    buf++;
  }  
}

/**
 * Saves the LCD screen to SD card in RGB888 format. 
 * The order of the colors must be rotated to obtain 
 * the same colors in the file as on the screen.
 */
bool saveBmpToSD_24bit(LGFX &lcd, const char *filename)
{
  log_i("save to bmp24 started");
  bool result = false;
  File file = SD.open(filename, "w");
  if (file)
  {
    int width  = lcd.width();
    int height = lcd.height();

    int rowSize = (3 * width + 3) & ~ 3;

    lgfx::bitmap_header_t bmpheader;
    bmpheader.bfType = 0x4D42;
    bmpheader.bfSize = rowSize * height + sizeof(bmpheader);
    bmpheader.bfOffBits = sizeof(bmpheader);

    bmpheader.biSize = 40;
    bmpheader.biWidth = width;
    bmpheader.biHeight = height;
    bmpheader.biPlanes = 1;
    bmpheader.biBitCount = 24;
    bmpheader.biCompression = 0;

    file.write((std::uint8_t*)&bmpheader, sizeof(bmpheader));
    std::uint8_t buffer[rowSize];
    memset(&buffer[rowSize - 4], 0, 4);
    //lcd.fillScreen(TFT_BLUE);
    for (int y = lcd.height() - 1; y >= 0; y--)
    {
      lcd.readRect(0, y, lcd.width(), 1, (lgfx::rgb888_t*)buffer);
      //printBuf888((lgfx::rgb888_t*)buffer, sizeof(buffer));
      rotate_rgb888((lgfx::rgb888_t*)buffer, sizeof(buffer));
      file.write(buffer, rowSize);
    }
    file.close();
    result = true;
  }
  else
  {
    Serial.print("error:file open failure\n");
  }
  return result;
}