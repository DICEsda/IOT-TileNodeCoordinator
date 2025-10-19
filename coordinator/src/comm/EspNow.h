#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <functional>
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
    // Convert a MAC string "AA:BB:CC:DD:EE:FF" to six bytes
    static bool macStringToBytes(const String& macStr, uint8_t out[6]);
    // send JSON blob directly to a MAC (raw bytes)
    bool sendToMac(const uint8_t mac[6], const String& json);
    
    // Pairing
    void enablePairingMode(uint32_t durationMs = 30000);
    void disablePairingMode();
    bool isPairingEnabled() const;
    
    // Callbacks
    void setMessageCallback(std::function<void(const String& nodeId, const uint8_t* data, size_t len)> callback);
    void setPairingCallback(std::function<void(const uint8_t* mac, const uint8_t* data, size_t len)> callback);

private:
    bool pairingEnabled;
    uint32_t pairingEndTime;
    std::function<void(const String& nodeId, const uint8_t* data, size_t len)> messageCallback;
    std::function<void(const uint8_t* mac, const uint8_t* data, size_t len)> pairingCallback;

    static void handleEspNowReceive(const uint8_t* mac, const uint8_t* data, int len);
    void processReceivedData(const uint8_t* mac, const uint8_t* data, int len);
};
