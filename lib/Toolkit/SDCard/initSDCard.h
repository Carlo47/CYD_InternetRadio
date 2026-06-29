#pragma once
#include <Arduino.h>
#include <SD.h>

namespace SDCard
{
  void initSDCard(SPIClass& spi);
  void printSDCardInfo();
  void listFiles(File dir, int indent = 0);
} // namespace SDCard