#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "ESP32AutoConnect.h"

void initESP32AutoConnect(AsyncWebServer &webServer, Preferences &prefs, const char hostname[])
{
  ESP32AutoConnect ac(webServer, prefs, hostname);
  //ac.clearCredentials();  // activate this line to remove stored credentials
  ac.autoConnect();
}