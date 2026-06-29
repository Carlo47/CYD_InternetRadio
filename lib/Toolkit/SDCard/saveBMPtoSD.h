#pragma once
#include "lgfx_esp32-2432S028R.h"
#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <SD.h>

namespace SDCard
{
  bool saveBmpToSD_16bit(LGFX& lcd, const char* filename);
  bool saveBmpToSD_24bit(LGFX& lcd, const char* filename);
} // namespace SDCard
