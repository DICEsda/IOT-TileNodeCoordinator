#pragma once

#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include <functional>
#include <vector>
#include <algorithm>
#include "../Models.h"

// Forward declarations for ESP-NOW v2.0 friend functions
class EspNow;
void staticRecvCallback(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len);
void staticSendCallback(const uint8_t* mac, esp_now_send_status_t status);

struct PeerStats {
    int8_t lastRssi;
    uint32_t lastSeenMs;
    uint32_t messageCount;
    uint32_t failedCount;
};

class EspNow {
    // Declare static callback functions as friends (ESP-NOW v2.0 signatures)
    friend void staticRecvCallback(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len);
    friend void staticSendCallback(const uint8_t* mac, esp_now_send_status_t status);
    
public:
    EspNow();
    ~EspNow();

    bool begin();
    void loop();

    // Communication
    bool sendLightCommand(const String& nodeId, uint8_t brightness, uint16_t fadeMs = 0, bool overrideStatus = false, uint16_t ttlMs = 1500);
    bool broadcastPairingMessage();
    // Convert a MAC string "AA:BB:CC:DD:EE:FF" to six bytes
    static bool macStringToBytes(const String& macStr, uint8_t out[6]);
    // send JSON blob directly to a MAC (raw bytes)
    bool sendToMac(const uint8_t mac[6], const String& json);
    
    // Pairing
    void enablePairingMode(uint32_t durationMs = 30000);
    void disablePairingMode();
    bool isPairingEnabled() const;
    // Persistence
    bool addPeer(const uint8_t mac[6]);
    bool removePeer(const uint8_t mac[6]);
    void clearAllPeers();
    void loadPeersFromStorage();
    void savePeersToStorage();
    
    // Callbacks
    void setMessageCallback(std::function<void(const String& nodeId, const uint8_t* data, size_t len)> callback);
    void setPairingCallback(std::function<void(const uint8_t* mac, const uint8_t* data, size_t len)> callback);
    void setSendErrorCallback(std::function<void(const String& nodeId)> callback);
    
    // Connection quality
    int8_t getPeerRssi(const String& macStr) const;
    PeerStats getPeerStats(const String& macStr) const;

private:
    bool pairingEnabled;
    uint32_t pairingEndTime;
    std::function<void(const String& nodeId, const uint8_t* data, size_t len)> messageCallback;
    std::function<void(const uint8_t* mac, const uint8_t* data, size_t len)> pairingCallback;
    std::function<void(const String& nodeId)> sendErrorCallback;

    void handleEspNowReceive(const uint8_t* mac, const uint8_t* data, int len);
    void processReceivedData(const uint8_t* mac, const uint8_t* data, int len);

    // Peer persistence cache
    static constexpr const char* PREFS_NS = "peers";
    std::vector<String> peers; // stored as MAC strings
    
    // Connection quality tracking
    std::map<String, PeerStats> peerStats;
};
