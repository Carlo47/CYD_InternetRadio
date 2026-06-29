/**
 * Class        Implementation of the class methods of ESP32AutoConnect
 *
 * Author       2024-09-18 Charles Geiser (https://www.dodeka.ch)
 *              2026-01-09 Implementation of an effective WiFi restart in
 *                         case of connection failure
 *              2026-03-06 Improved handling of preferences when they are not initialized
 *
 * Purpose      Yet another ESP32 autoconnect WiFi library
 *              Starts up an access point AutoConnectAP when no WLAN connection
 *              could be established and asks the user for WLAN credentials.
 *
 * Usage        initESP32AutoConnect(AsyncWebServer *webServer, Preferences &prefs, const char hostname[])
 *              ac.clearCredentials;  // for test
 *              ac.autoconnect();
 *
 *              - Connect your cell phone to AutoConnectAP
 *              - Open 192.168.4.1 in your browser and enter the WLAN credentials
 *              - Connect to your WLAN and open http://hostname to control
 *                your application remotely
 *              - The next time you start the program, your login details are
 *                known and do not need to be entered again
 *              - Note that WiFi.setAutoReconnect(false) is set to “false” because
 *                we will reconnect ourselves after an interruption using a timer
 *
 * Board        ESP32 DoIt DevKit V1
 * Remarks
 *
 * References   https://github.com/alanswx/ESPAsyncWiFiManager
 *              https://randomnerdtutorials.com/esp32-wi-fi-manager-asyncwebserver
 *              https://github.com/aliffathoni/ESPAutoWifi
 */
#include "ESP32AutoConnect.h"
#include "query.h"

namespace Esp32AutoConnect
{
  const char PREFS_NAMESPACE[] = "CREDENTIALS";

  /**
   * Looks for stored credentials and returns true
   * if both ssid and password are found
   */
  bool ESP32AutoConnect::credentialsAreAvailable()
  {
    log_i("==> begin");
    Preferences prefs;
    prefs.begin(PREFS_NAMESPACE, true);
    if (prefs.isKey("ssid"))
      _ssid = prefs.getString("ssid");
    if (prefs.isKey("password"))
      _password = prefs.getString("password");
    prefs.end();
    log_i("ssid=%s, password=%s", _ssid.c_str(), _password.c_str());
    log_i("==> end");
    return (_ssid != "" && _password != "") ? true : false;
  }

  /**
   * Clear the stored network
   */
  void ESP32AutoConnect::clearCredentials()
  {
    Preferences prefs;
    prefs.begin(PREFS_NAMESPACE);
    prefs.clear();
    prefs.end();
  }

  void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info)
  {
    Preferences prefs;
    String      ssid, password;
    prefs.begin(PREFS_NAMESPACE, true);
    if (prefs.isKey("ssid"))
      ssid = prefs.getString("ssid", "");
    if (prefs.isKey("password"))
      password = prefs.getString("password", "");
    prefs.end();
    log_i("event=%d", event);
    switch (event)
    {
      case ARDUINO_EVENT_WIFI_OFF: // 100
        Serial.printf("WiFi off\n");
        vTaskDelay(pdMS_TO_TICKS(300));
        break;
      case ARDUINO_EVENT_WIFI_READY: // 101
        Serial.printf("WiFi ready\n");
        vTaskDelay(pdMS_TO_TICKS(300));
        break;
      case ARDUINO_EVENT_WIFI_STA_START: // 110
        Serial.printf("WiFi started\n");
        vTaskDelay(pdMS_TO_TICKS(300));
        break;
      case ARDUINO_EVENT_WIFI_STA_STOP: // 111
        Serial.printf("WiFi stoped\n");
        vTaskDelay(pdMS_TO_TICKS(300));
        break;
      case ARDUINO_EVENT_WIFI_STA_CONNECTED: // 112
        Serial.printf("WiFi auto connected\n");
        break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: // 113
        Serial.printf("WiFi disconnected, reason: %d\n", info.wifi_sta_disconnected.reason);
        Serial.println("Trying to auto reconnect WiFi...\n");
        WiFi.disconnect(true, true);
        vTaskDelay(pdMS_TO_TICKS(300));
        WiFi.mode(WIFI_OFF);
        vTaskDelay(pdMS_TO_TICKS(500));
        WiFi.mode(WIFI_STA);
        vTaskDelay(pdMS_TO_TICKS(300));
        WiFi.begin(ssid, password);
        break;
      case ARDUINO_EVENT_WIFI_STA_GOT_IP: // 115
        Serial.printf("WiFi auto got IP: %s\n", WiFi.localIP().toString().c_str());
        break;
    }
  }

  /**
   * Returns true if th ESP is connected to the network stored in preferences
   */
  bool ESP32AutoConnect::weAreConnectedToWLAN(String ssid, String password)
  {
    if (WiFi.status() == WL_CONNECTED)
      WiFi.disconnect(true, true);
    WiFi.mode(WIFI_OFF);
    vTaskDelay(pdMS_TO_TICKS(500)); // RF Stabilisierung
    WiFi.mode(WIFI_STA);
    vTaskDelay(pdMS_TO_TICKS(300)); // RF Stabilisierung
    WiFi.setSleep(false);           // crucial for AsyncWebServer
    WiFi.setAutoReconnect(false);
    WiFi.persistent(false);
    WiFi.onEvent(onWiFiEvent);
    WiFi.setHostname(_hostname.c_str());
    WiFi.begin(ssid, password);
    vTaskDelay(pdMS_TO_TICKS(500));
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 8000)
    {
      Serial.print(".");
      vTaskDelay(pdMS_TO_TICKS(200));
    }
    return WiFi.status() == WL_CONNECTED;
  }

  /**
   * Look for available networks and compose the
   * option list for the query web page
   */
  String ESP32AutoConnect::composeNetworkList()
  {
    String networks = "";
    int    n        = WiFi.scanNetworks();
    for (int i = 0; i < n; i++)
    {
      // <option value="0">Dodeka2G4</option>
      char buf[64];
      snprintf(buf, sizeof(buf), "<option value=\"%d\">%s</option>", i, WiFi.SSID(i).c_str());
      networks += String(buf);
    }
    // log_i("%s", networks.c_str());
    return networks;
  }

  /**
   * Starts an access point, searches for available networks and prompts the user
   * on the query web page to select a network and enter the corresponding password
   */
  void ESP32AutoConnect::requestCredentialsAndRestart()
  {
    // Preferences prefs;
    String& hn = _hostname;
    if (WiFi.isConnected())
      WiFi.disconnect(true, true);
    WiFi.softAP(_apSSID.c_str(), NULL);
    log_i(R"raw(
---    
Connect your mobile to the access point %s and enter
your WLAN credentials on the webpage http://%s
---)raw",
          _apSSID.c_str(),
          WiFi.softAPIP().toString().c_str());

    String networks = composeNetworkList();
    String query    = query0;       // raw literal from query.h
    query.replace("{n}", networks); // insert available networks into query web page

    _server->on(
        "/", HTTP_GET, [query](AsyncWebServerRequest* request) { request->send(200, "text/html", query.c_str()); });

    _server->on("/get",
                HTTP_GET,
                [hn](AsyncWebServerRequest* request)
                {
                  Preferences prefs;
                  String      ssid, password;
                  if (request->hasParam("ssid"))
                  {
                    int index = (request->getParam("ssid")->value()).toInt();
                    ssid      = WiFi.SSID(index);
                  }
                  if (request->hasParam("password"))
                  {
                    password = request->getParam("password")->value();
                  }

                  if (prefs.begin(PREFS_NAMESPACE))
                  {
                    prefs.putString("ssid", ssid);
                    prefs.putString("password", password);
                    prefs.end();
                  }
                  else
                    log_i("Could not open prefs namespace: %s", PREFS_NAMESPACE);

                  request->send(200,
                                "text/html",
                                "<html>"
                                "<head>"
                                "<style>"
                                "body { font-size: 24px; font-family: Arial; padding: 2rem; }"
                                ".box {"
                                "background:#fffae6;"
                                "border:1rem solid #e0c200;"
                                "padding:1rem;"
                                "border-radius:1rem;"
                                "max-width:500px;"
                                "}"
                                "</style>"
                                "</head>"
                                "<body>"
                                "<div class='box'>"
                                "<b>Credentials saved!</b><br><br>"
                                "ESP will restart and connect to your<br>"
                                "WLAN <b>" +
                                    ssid +
                                    "</b>.<br><br>"
                                    "Proceed to <b>http://" +
                                    hn +
                                    "[:port]</b>"
                                    "</div>"
                                    "</body>"
                                    "</html>");

                  // asynchronous restart after 5 seconds
                  esp_timer_handle_t            restartTimer;
                  const esp_timer_create_args_t timerArgs = {.callback =
                                                                 [](void*)
                                                             {
                                                               WiFi.disconnect(true, true);
                                                               WiFi.mode(WIFI_OFF);
                                                               vTaskDelay(pdMS_TO_TICKS(500));
                                                               ESP.restart();
                                                             },
                                                             .arg             = nullptr,
                                                             .dispatch_method = ESP_TIMER_TASK,
                                                             .name            = "restart_timer"};
                  esp_timer_create(&timerArgs, &restartTimer);
                  esp_timer_start_once(restartTimer, 5000000);
                });
    _server->begin();
  }

  /*   void notFound1(AsyncWebServerRequest* request)
    {
      request->send(404, "text/plain", "Not found");
    } */

  /**
   * Start the autoconnect process
   */
  void ESP32AutoConnect::autoConnect()
  {
    bool notConnected = true;

    if (credentialsAreAvailable())
    {
      if (weAreConnectedToWLAN(_ssid, _password))
      {
        log_i(R"(
---
Connected to your WLAN %s. 
Proceed to http://%s or http://%s
---)",
              _ssid,
              WiFi.getHostname(),
              WiFi.localIP().toString().c_str());
        notConnected = false;
      }
      else
      {
        log_i(R"raw(
---
Could not connect to your WLAN %s / %s,
try again or select another network.
---)raw",
              _ssid.c_str(),
              _password.c_str());
        requestCredentialsAndRestart();
      }
    }
    else
    {
      log_e("==> No WiFi credentials available");
      requestCredentialsAndRestart();
    }
    // The next lines prevent the main loop from being entered
    // before a connection has been established.
    while (notConnected)
    {
      vTaskDelay(pdMS_TO_TICKS(100));
    }
  }
} // namespace Esp32AutoConnect