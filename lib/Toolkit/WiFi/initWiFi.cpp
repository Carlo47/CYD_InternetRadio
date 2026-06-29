#include "initWiFi.h"

namespace InitWiFi
{
  void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
  {
    switch (event)
    {
      case ARDUINO_EVENT_WIFI_STA_CONNECTED:
        Serial.printf("WiFi connected\n");
        break;

      case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        Serial.printf("WiFi got IP: %s\n", WiFi.localIP().toString().c_str());
        break;

      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        Serial.printf("WiFi is disconnected, reason: %d\n", info.wifi_sta_disconnected.reason);
        /*         Serial.println("Trying to reconnect WiFi...\n");
                WiFi.disconnect(false, false);
                WiFi.begin(); */
        break;
    }
  }

  /**
   * Establish the WiFi connection with router
   * and set a hostname for the ESP32
   */
  uint8_t initWiFi(const char ssid[], const char password[], const char hostname[])
  {
    Serial.printf("Try to connect to WiFi with: %s / %s\n", ssid, password);

    if (WiFi.status() == WL_CONNECTED)
      WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    vTaskDelay(pdMS_TO_TICKS(500));
    WiFi.persistent(false);
    WiFi.setSleep(false); // crucial for AsyncWebServer
    WiFi.setAutoReconnect(false);
    WiFi.onEvent(onWiFiEvent);
    WiFi.mode(WIFI_STA);
    vTaskDelay(pdMS_TO_TICKS(500));
    WiFi.setHostname(hostname);
    WiFi.begin(ssid, password);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 8000)
    {
      Serial.print(".");
      vTaskDelay(pdMS_TO_TICKS(200));
    }
    log_i("==> done");
    return WiFi.status() == WL_CONNECTED;
    // return WiFi.waitForConnectResult();
  }

  void initESP32AutoConnect(const char hostname[])
  {
    Esp32AutoConnect::ESP32AutoConnect ac(hostname);
    // Activate next line to remove stored credentials, run the code
    // and then recompile again with the line commented.
    // Next time you start, you have to reconnect to AutoConnectAP

    // ac.clearCredentials();
    ac.autoConnect();
  }

  /**
   * Use a raw string literal to print a formatted
   * string of WiFi connection details
   */
  void printConnectionDetails()
  {
    Serial.printf(R"(
Connection Details
------------------
  SSID         %s
  Hostname     %s
  IP-Address   %s
  MAC-Address  %s
  RSSI         %d (received signal strength indicator)
  )",
                  WiFi.SSID().c_str(),
                  // WiFi.hostname().c_str(),  // ESP8266
                  WiFi.getHostname(), // ESP32
                  WiFi.localIP().toString().c_str(),
                  WiFi.macAddress().c_str(),
                  WiFi.RSSI());
  }

  /**
   * Print nearby WiFi networks with SSID und RSSI
   */
  void printNearbyNetworks()
  {
    int n = WiFi.scanNetworks();
    printf(R"(
Nearby WiFi networks:
--------------------
)");
    for (int i = 0; i < n; i++)
    {
      printf("%s\t%d\r\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
    }
  }
} // namespace InitWiFi