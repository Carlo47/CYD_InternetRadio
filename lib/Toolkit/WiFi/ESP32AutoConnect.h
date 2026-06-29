/**
 * Header       ESP32AutoConnect.h
 *
 * Author       2024-09-18 Charles Geiser (https://www.dodeka.ch)
 *
 * Purpose      Declaration of the class ESP32AutoConnect
 */
#pragma once
#include "esp_wifi.h"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include <WiFi.h>

namespace Esp32AutoConnect
{
  class ESP32AutoConnect
  {
    public:
      ESP32AutoConnect(const char hostname[] = "esp-websrv") : _hostname(hostname)
      {
        _server = new AsyncWebServer(80);
      }

      ~ESP32AutoConnect()
      {
        delete (_server);
      }

      void autoConnect();
      void clearCredentials();

    private:
      bool            credentialsAreAvailable();
      bool            weAreConnectedToWLAN(String ssid, String password);
      void            requestCredentialsAndRestart();
      String          composeNetworkList();
      String          _apSSID = "AutoConnectAP";
      String          _apPassword;
      String          _ssid;
      String          _password;
      String          _hostname;
      AsyncWebServer* _server;
  };
} // namespace Esp32AutoConnect