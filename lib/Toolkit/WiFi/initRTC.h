#pragma once
#include <Arduino.h>
#include <WiFi.h>

namespace InitRTC
{
  void initRTC(const char timeZone[], const char ntpServerPool[], bool disconnect = false);
  void printDateTime(int format);
} // namespace InitRTC