#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "../Models.h"

class EspNow {
public:
    EspNow();
    ~EspNow();

    bool begin();
    void loop();

    // Communication
    bool sendLightCommand(const String& nodeId, uint8_t brightness, uint16_t fadeMs = 0);
    bool broadcastPairingMessage();
    
    // Pairing
    void enablePairingMode(uint32_t durationMs = 30000);
    bool isPairingEnabled() const;
    
    // Callbacks
    void setMessageCallback(void (*callback)(const String& nodeId, const uint8_t* data, size_t len));
    void setPairingCallback(void (*callback)(const uint8_t* mac, const uint8_t* data, size_t len));

private:
    bool pairingEnabled;
    uint32_t pairingEndTime;
    void (*messageCallback)(const String& nodeId, const uint8_t* data, size_t len);
    void (*pairingCallback)(const uint8_t* mac, const uint8_t* data, size_t len);

    static void handleEspNowReceive(const uint8_t* mac, const uint8_t* data, int len);
    void processReceivedData(const uint8_t* mac, const uint8_t* data, int len);
};
