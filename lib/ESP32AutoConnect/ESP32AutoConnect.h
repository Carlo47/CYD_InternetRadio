/**
 * Header       ESP32AutoConnect.h
 * 
 * Author       2024-09-18 Charles Geiser (https://www.dodeka.ch)
 * 
 * Purpose      Declaration of the class ESP32AutoConnect           
 */

#include "Arduino.h"
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#pragma once

class ESP32AutoConnect 
{
    public:
        ESP32AutoConnect(AsyncWebServer& server, Preferences& prefs, String hostname="esp-websrv") : 
            _server(server), _prefs(prefs), _hostname(hostname) 
        {
        }

        void autoConnect();
        void clearCredentials();
        
    private:
        bool credentialsAreAvailable();
        bool weAreConnectedToWLAN(String ssid, String password);
        void requestCredentialsAndRestart();
        String composeNetworkList();
        String _apSSID = "AutoConnectAP";
        String _apPassword;
        String _ssid;
        String _password;
        String _hostname;
        Preferences& _prefs;
        AsyncWebServer& _server;      
};