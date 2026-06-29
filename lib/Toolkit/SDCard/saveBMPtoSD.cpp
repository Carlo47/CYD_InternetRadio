#include "saveBMPtoSD.h"

namespace SDCard
{
  bool saveBmpToSD_16bit(LGFX& lcd, const char* filename)
  {
    bool result = false;
    File file   = SD.open(filename, "w");
    if (file)
    {
      int width  = lcd.width();
      int height = lcd.height();

      int rowSize = (2 * width + 3) & ~3;

      lgfx::bitmap_header_t bmpheader;
      bmpheader.bfType    = 0x4D42;
      bmpheader.bfSize    = rowSize * height + sizeof(bmpheader);
      bmpheader.bfOffBits = sizeof(bmpheader);

      bmpheader.biSize        = 40;
      bmpheader.biWidth       = width;
      bmpheader.biHeight      = height;
      bmpheader.biPlanes      = 1;
      bmpheader.biBitCount    = 16;
      bmpheader.biCompression = 3;

      file.write((std::uint8_t*) &bmpheader, sizeof(bmpheader));
      std::uint8_t buffer[rowSize];
      memset(&buffer[rowSize - 4], 0, 4);
      for (int y = lcd.height() - 1; y >= 0; y--)
      {
        lcd.readRect(0, y, lcd.width(), 1, (lgfx::rgb565_t*) buffer);
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

  bool saveBmpToSD_24bit(LGFX& lcd, const char* filename)
  {
    bool result = false;
    File file   = SD.open(filename, "w");
    if (file)
    {
      int width  = lcd.width();
      int height = lcd.height();

      int rowSize = (3 * width + 3) & ~3;

      lgfx::bitmap_header_t bmpheader;
      bmpheader.bfType    = 0x4D42;
      bmpheader.bfSize    = rowSize * height + sizeof(bmpheader);
      bmpheader.bfOffBits = sizeof(bmpheader);

      bmpheader.biSize        = 40;
      bmpheader.biWidth       = width;
      bmpheader.biHeight      = height;
      bmpheader.biPlanes      = 1;
      bmpheader.biBitCount    = 24;
      bmpheader.biCompression = 0;

      file.write((std::uint8_t*) &bmpheader, sizeof(bmpheader));
      std::uint8_t buffer[rowSize];
      memset(&buffer[rowSize - 4], 0, 4);
      for (int y = lcd.height() - 1; y >= 0; y--)
      {
        lcd.readRect(0, y, lcd.width(), 1, (lgfx::rgb888_t*) buffer);
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
} // namespace SDCard