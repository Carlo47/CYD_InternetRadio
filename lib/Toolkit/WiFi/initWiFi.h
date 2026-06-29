#pragma once
#include "ESP32AutoConnect.h"
#include <Arduino.h>
#include <WIFI.h>

namespace InitWiFi
{
  void    initESP32AutoConnect(const char hostname[]);
  uint8_t initWiFi(const char ssid[], const char password[], const char hostname[]);
  void    printConnectionDetails();
  void    printNearbyNetworks();
} // namespace InitWiFi