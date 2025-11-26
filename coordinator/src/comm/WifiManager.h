#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <vector>
#include "../../shared/src/ConfigManager.h"

class WifiManager {
public:
    struct Status {
        bool connected = false;
        bool offlineMode = false;
        String ssid;
        int32_t rssi = -127;
        IPAddress ip;
    };

    WifiManager();
    ~WifiManager() = default;

    bool begin();
    void loop();
    bool ensureConnected();
    bool reconfigureWifi(); // Interactive WiFi reconfiguration at runtime

    bool isConnected() const { return status.connected; }
    bool isOffline() const { return status.offlineMode; }
    Status getStatus() const { return status; }
    
    void setEspNow(class EspNow* espNowPtr) { espNow = espNowPtr; }

private:
    ConfigManager config;
    String storedSsid;
    String storedPassword;
    Status status;
    uint32_t lastReconnectAttempt;
    class EspNow* espNow = nullptr;

    bool attemptConnect(const String& ssid, const String& password, bool verbose = true);
    bool interactiveSetup();
    bool promptYesNo(const String& prompt);
    String promptLine(const String& prompt, bool allowEmpty = false, bool hide = false);
    bool selectNetwork(String& ssidOut);
    void updateStatusCache();
};
