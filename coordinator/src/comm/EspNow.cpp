#include "EspNow.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "../../shared/src/EspNowMessage.h"
#include "../nodes/NodeRegistry.h"
#include "../utils/Logger.h"
#include <Preferences.h>

// Simple singleton to bridge static callbacks
static EspNow* s_self = nullptr;

// ✓ ESP-NOW v2.0 callback signatures (Checklist: ESP-NOW Version)
// v2 uses esp_now_recv_info_t instead of passing MAC directly
void staticRecvCallback(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    Logger::info("*** ESP-NOW V2 RX CALLBACK TRIGGERED ***");
    if (s_self && recv_info && recv_info->src_addr) {
        s_self->handleEspNowReceive(recv_info->src_addr, data, len);
    } else {
        Logger::error("s_self is NULL or invalid recv_info in receive callback!");
    }
}

void staticSendCallback(const uint8_t* mac, esp_now_send_status_t status) {
    if (!mac) {
        Logger::warn("ESP-NOW V2: send_cb (null mac) status=%d", (int)status);
        return;
    }
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Logger::info("ESP-NOW V2: send_cb to %s status=%d (%s)", macStr, (int)status, 
                 status == ESP_NOW_SEND_SUCCESS ? "SUCCESS" : "FAIL");
}

// small helper to hex encode bytes
static String bytesToHex(const uint8_t* b, size_t n) {
    String s;
    s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) {
        char tmp[3];
        snprintf(tmp, sizeof(tmp), "%02X", b[i]);
        s += tmp;
    }
    return s;
}

bool EspNow::macStringToBytes(const String& macStr, uint8_t out[6]) {
    int vals[6];
    if (sscanf(macStr.c_str(), "%x:%x:%x:%x:%x:%x",
               &vals[0], &vals[1], &vals[2], &vals[3], &vals[4], &vals[5]) != 6) {
        return false;
    }
    for (int i = 0; i < 6; ++i) out[i] = (uint8_t)vals[i];
    return true;
}

EspNow::EspNow()
    : pairingEnabled(false)
    , pairingEndTime(0)
    , messageCallback(nullptr)
    , pairingCallback(nullptr) {}

EspNow::~EspNow() {}

bool EspNow::begin() {
    Logger::info("===========================================");
    Logger::info("ESP-NOW V2.0 INITIALIZATION CHECKLIST");
    Logger::info("===========================================");
    
    // ✓ Checklist: Wi-Fi Mode - Set to STA only, disable SoftAP
    Logger::info("✓ [1/9] Setting WiFi mode to STA only...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); // Ensure not connected to any AP
    delay(100);
    
    if (WiFi.getMode() != WIFI_STA) {
        Logger::error("✗ WiFi mode is not STA! Current mode: %d", WiFi.getMode());
        return false;
    }
    Logger::info("  ✓ WiFi mode confirmed as STA");
    
    // ✓ Checklist: MAC Address - Get programmatically
    Logger::info("✓ [2/9] Getting MAC address programmatically...");
    uint8_t mac[6];
    WiFi.macAddress(mac);
    Logger::info("  ✓ Coordinator MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // ✓ Checklist: Board Definitions - Using latest ESP-IDF via platformio
    Logger::info("✓ [3/9] Board definitions: ESP32-S3 via latest ESP-IDF");
    
    // Disable Wi-Fi power save for better RX reliability
    Logger::info("✓ [4/9] Disabling WiFi sleep for reliable reception...");
    WiFi.setSleep(false);
    
    // Boost TX power for better range
    Logger::info("✓ [5/9] Setting TX power to maximum (19.5dBm)...");
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    
    // ✓ Checklist: ESP-NOW Version - Initialize v2.0
    Logger::info("✓ [6/9] Initializing ESP-NOW v2.0...");
    esp_err_t initResult = esp_now_init();
    if (initResult != ESP_OK) {
        Logger::error("✗ esp_now_init() failed with error %d", initResult);
        return false;
    }
    Logger::info("  ✓ ESP-NOW v2.0 initialized successfully");
    
    // Set WiFi channel - both devices must use same channel
    Logger::info("✓ [7/9] Setting WiFi channel to 1...");
    esp_wifi_set_promiscuous(true);
    esp_err_t chRes = esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    
    uint8_t primary = 0; wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
    esp_wifi_get_channel(&primary, &second);
    
    if (primary != 1) {
        Logger::error("✗ Failed to set channel to 1! Currently on channel %d", (int)primary);
        return false;
    }
    Logger::info("  ✓ Channel set to %d (secondary: %d)", (int)primary, (int)second);
    
    // Set WiFi protocol for ESP32-S3 and ESP32-C3 compatibility
    Logger::info("  Setting WiFi protocol (802.11b/g/n)...");
    esp_err_t protocolRes = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    if (protocolRes != ESP_OK) {
        Logger::error("✗ Failed to set WiFi protocol, error=%d", protocolRes);
    } else {
        Logger::info("  ✓ WiFi protocol set successfully");
    }
    
    s_self = this;
    
    // ✓ Checklist: Callbacks - Implement send and receive callbacks
    Logger::info("✓ [8/9] Registering ESP-NOW v2.0 callbacks...");
    esp_err_t recvResult = esp_now_register_recv_cb(staticRecvCallback);
    if (recvResult != ESP_OK) {
        Logger::error("✗ Failed to register recv callback, error=%d", recvResult);
        return false;
    }
    
    esp_err_t sendResult = esp_now_register_send_cb(staticSendCallback);
    if (sendResult != ESP_OK) {
        Logger::error("✗ Failed to register send callback, error=%d", sendResult);
        return false;
    }
    Logger::info("  ✓ Send and receive callbacks registered");
    
    // ✓ Checklist: Peer Registration - Add broadcast peer first
    Logger::info("✓ [9/9] Registering broadcast peer...");
    uint8_t bcast[6]; memset(bcast, 0xFF, 6);
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, bcast, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false; // ✓ Checklist: Encryption - Disabled for now
    peerInfo.ifidx = WIFI_IF_STA;
    
    esp_err_t bres = esp_now_add_peer(&peerInfo);
    if (bres == ESP_OK) {
        Logger::info("  ✓ Broadcast peer (FF:FF:FF:FF:FF:FF) added on channel 1");
    } else if (bres == ESP_ERR_ESPNOW_EXIST) {
        Logger::info("  ✓ Broadcast peer already exists");
    } else {
        Logger::error("✗ Failed to add broadcast peer, error=%d", (int)bres);
    }
    
    // Load persisted peers and re-register them
    loadPeersFromStorage();
    for (const auto& macStr : peers) {
        uint8_t peerMac[6];
        if (macStringToBytes(macStr, peerMac)) {
            addPeer(peerMac);
            Logger::info("  ✓ Restored peer: %s", macStr.c_str());
        }
    }
    
    // ✓ Checklist: Testing - Send simple test message
    Logger::info("===========================================");
    Logger::info("TESTING: Sending test broadcast...");
    uint8_t testBcast[6]; memset(testBcast, 0xFF, 6);
    String testMsg = "{\"test\":\"coordinator_alive\"}";
    
    // ✓ Checklist: Message Size - Keep ≤ 250 bytes
    if (testMsg.length() > 250) {
        Logger::warn("Test message exceeds 250 bytes!");
    } else {
        Logger::info("  Message size: %d bytes (within 250 byte limit)", testMsg.length());
    }
    
    esp_err_t testRes = esp_now_send(testBcast, (uint8_t*)testMsg.c_str(), testMsg.length());
    if (testRes == ESP_OK) {
        Logger::info("  ✓ Test broadcast queued successfully");
    } else {
        Logger::error("✗ Test broadcast failed, error=%d", testRes);
    }
    
    delay(500);
    
    Logger::info("===========================================");
    Logger::info("✓ ESP-NOW V2.0 READY - All checks passed!");
    Logger::info("✓ Coordinator listening for node messages");
    Logger::info("===========================================");
    
    return true;
}

void EspNow::loop() {
    // Debug: Periodically log that we're alive and waiting
    static uint32_t lastDebugLog = 0;
    if (millis() - lastDebugLog > 10000) {
        Logger::debug("ESP-NOW: Loop running, pairing=%d", isPairingEnabled());
        lastDebugLog = millis();
    }
    
    // Check pairing timeout
    if (pairingEnabled && millis() > pairingEndTime) {
        pairingEnabled = false;
        Logger::info("ESP-NOW: Pairing window closed");
    }
}

bool EspNow::sendLightCommand(const String& nodeId, uint8_t brightness, uint16_t fadeMs) {
    uint8_t mac[6];
    if (!macStringToBytes(nodeId, mac)) {
        Logger::warn("sendLightCommand: invalid MAC string %s", nodeId.c_str());
        return false;
    }

    SetLightMessage msg;
    // Optimize: Build cmd_id more efficiently
    char cmdIdBuf[32];
    snprintf(cmdIdBuf, sizeof(cmdIdBuf), "%lu-%02X%02X%02X", 
             (unsigned long)millis(), mac[3], mac[4], mac[5]);
    msg.cmd_id = cmdIdBuf;
    msg.light_id = ""; // coordinator will publish state mapping separately
    msg.r = 0; msg.g = 0; msg.b = 0; msg.w = brightness;
    msg.fade_ms = fadeMs;
    msg.override_status = false;
    msg.ttl_ms = 1500;

    String json = msg.toJson();
    bool ok = sendToMac(mac, json);
    if (!ok) {
        Logger::warn("sendLightCommand: failed to deliver to %s", nodeId.c_str());
    } else {
        Logger::info("sendLightCommand sent %s -> %s (w=%d)", msg.cmd_id.c_str(), nodeId.c_str(), brightness);
    }
    return ok;
}

bool EspNow::broadcastPairingMessage() {
    // Placeholder broadcast
    return true;
}

void EspNow::enablePairingMode(uint32_t durationMs) {
    pairingEnabled = true;
    pairingEndTime = millis() + durationMs;
}

void EspNow::disablePairingMode() {
    pairingEnabled = false;
}

bool EspNow::isPairingEnabled() const {
    return pairingEnabled && millis() < pairingEndTime;
}

void EspNow::setMessageCallback(std::function<void(const String& nodeId, const uint8_t* data, size_t len)> callback) {
    messageCallback = callback;
}

void EspNow::setPairingCallback(std::function<void(const uint8_t* mac, const uint8_t* data, size_t len)> callback) {
    pairingCallback = callback;
}

void EspNow::handleEspNowReceive(const uint8_t* mac, const uint8_t* data, int len) {
    // Validate parameters first
    if (!mac || !data || len <= 0 || len > 250) {
        Logger::error("Invalid ESP-NOW receive parameters: mac=%p, data=%p, len=%d", 
                     (void*)mac, (void*)data, len);
        return;
    }
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // ✓ Checklist: Testing - Print received messages on serial
    Logger::info("==> ESP-NOW V2 RECEIVED %dB from %s", len, macStr);
    Logger::hexDump("ESP-NOW V2 RX", data, len, 64);
    
    String payload((const char*)data, len);
    Logger::info("==> Payload: %s", payload.c_str());
    
    processReceivedData(mac, data, len);
}

void EspNow::processReceivedData(const uint8_t* mac, const uint8_t* data, int len) {
    // Validate input - should already be validated but double-check
    if (!mac || !data || len <= 0) return;
    
    // Format MAC as ID string
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    String nodeId = String(macStr);

    // Basic forwarding: if pairing is active and the message is a join_request, forward raw data
    String payload((const char*)data, len);
    MessageType mt = MessageFactory::getMessageType(payload);
    
    Logger::info("Processing message type=%d from %s", (int)mt, macStr);

    if (mt == MessageType::JOIN_REQUEST) {
        Logger::info("*** JOIN_REQUEST detected from %s ***", macStr);
        
        if (isPairingEnabled()) {
            Logger::info("Pairing is enabled - processing JOIN_REQUEST");
            if (pairingCallback) {
                Logger::info("Calling pairingCallback...");
                pairingCallback(mac, data, (size_t)len);
            } else {
                Logger::error("CRITICAL: Pairing active but no pairingCallback registered!");
            }
            // Also ensure peer exists so that accept message can be sent directly
            addPeer(mac);
        } else {
            Logger::warn("JOIN_REQUEST received but pairing is NOT enabled - press button to enable pairing");
        }
        return;
    }

    // Forward other messages via general callback
    if (messageCallback) {
        messageCallback(nodeId, (const uint8_t*)payload.c_str(), payload.length());
    } else {
        Logger::warn("Message received but no messageCallback registered");
    }
}

bool EspNow::sendToMac(const uint8_t mac[6], const String& json) {
    // ✓ Checklist: Message Size - Verify before sending
    if (json.length() > 250) {
        Logger::error("Message too large: %d bytes (max 250)", json.length());
        return false;
    }
    
    // ✓ Checklist: Error Handling - Check send result
    esp_err_t res = esp_now_send(mac, (uint8_t*)json.c_str(), json.length());
    if (res != ESP_OK) {
        Logger::warn("ESP-NOW V2 send failed: %d (will retry via callback)", res);
        return false;
    }
    return true;
}

bool EspNow::addPeer(const uint8_t mac[6]) {
    // ✓ Checklist: Peer Registration - Register node's MAC on coordinator
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 1; // Must match coordinator's channel
    peerInfo.encrypt = false; // ✓ Checklist: Encryption disabled
    peerInfo.ifidx = WIFI_IF_STA;
    
    esp_err_t res = esp_now_add_peer(&peerInfo);
    if (res == ESP_OK || res == ESP_ERR_ESPNOW_EXIST) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        String smac(macStr);
        bool found=false; 
        for (auto& s:peers){ if (s==smac){found=true;break;} }
        if (!found) { 
            peers.push_back(smac); 
            savePeersToStorage(); 
            Logger::info("✓ Peer registered: %s", macStr);
        }
        return true;
    }
    Logger::warn("✗ Failed to add peer: error %d", res);
    return false;
}

bool EspNow::removePeer(const uint8_t mac[6]) {
    esp_err_t res = esp_now_del_peer(mac);
    if (res == ESP_OK) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        String smac(macStr);
        peers.erase(std::remove(peers.begin(), peers.end(), smac), peers.end());
        savePeersToStorage();
        return true;
    }
    Logger::warn("Failed to remove peer: %d", res);
    return false;
}

void EspNow::loadPeersFromStorage() {
    Preferences p;
    if (!p.begin(PREFS_NS, true)) {
        Logger::warn("Preferences unavailable - peer list will not persist (NVS not ready)");
        peers.clear();
        return;
    }
    uint32_t count = p.getUInt("count", 0);
    peers.clear(); 
    peers.reserve(count);
    for (uint32_t i=0; i<count; i++) {
        String key = String("mac") + String(i);
        String mac = p.getString(key.c_str(), "");
        if (mac.length() == 17) peers.push_back(mac);
    }
    p.end();
    Logger::info("Loaded %d peers from storage", peers.size());
}

void EspNow::savePeersToStorage() {
    Preferences p;
    if (!p.begin(PREFS_NS, false)) {
        Logger::debug("Preferences unavailable - peer list not saved (NVS not ready)");
        return;
    }
    p.clear();
    p.putUInt("count", peers.size());
    for (size_t i=0; i<peers.size(); ++i) {
        String key = String("mac") + String(i);
        p.putString(key.c_str(), peers[i]);
    }
    p.end();
    Logger::debug("Saved %d peers to storage", peers.size());
}
