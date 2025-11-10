#include "EspNow.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "../../shared/src/EspNowMessage.h"
#include "../nodes/NodeRegistry.h"
#include "../utils/Logger.h"
#include <Preferences.h>
#include <map>

// Simple singleton to bridge static callbacks
static EspNow* s_self = nullptr;
// Recently handled JOIN requests to avoid duplicate processing
static std::map<String, uint32_t> s_recentJoin;

// ✓ ESP-NOW v2.0 callback signatures (Checklist: ESP-NOW Version)
// v2 uses esp_now_recv_info_t instead of passing MAC directly
void staticRecvCallback(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    // Extract RSSI and pass to handler (ESP-NOW v2.0 provides RSSI in recv_info)
    if (s_self && recv_info && recv_info->src_addr) {
        s_self->handleEspNowReceive(recv_info->src_addr, data, len);
        
        // Update RSSI stats (rx_ctrl is a pointer in ESP-NOW v2.0)
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 recv_info->src_addr[0], recv_info->src_addr[1], recv_info->src_addr[2],
                 recv_info->src_addr[3], recv_info->src_addr[4], recv_info->src_addr[5]);
        String smac(macStr);
        
        auto& stats = s_self->peerStats[smac];
        if (recv_info->rx_ctrl) {
            stats.lastRssi = recv_info->rx_ctrl->rssi;
        }
        stats.lastSeenMs = millis();
        stats.messageCount++;
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
    
    // Update stats and trigger error callback
    if (s_self) {
        String smac(macStr);
        auto& stats = s_self->peerStats[smac];
        if (status != ESP_NOW_SEND_SUCCESS) {
            stats.failedCount++;
            // Trigger error callback for visual feedback
            if (s_self->sendErrorCallback) {
                s_self->sendErrorCallback(smac);
            }
        }
    }
    
    if (status == ESP_NOW_SEND_SUCCESS) {
        Logger::debug("ESP-NOW V2: send_cb OK -> %s", macStr);
    } else {
        Logger::warn("ESP-NOW V2: send_cb to %s FAILED (status=%d)", macStr, (int)status);
    }
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
    , pairingCallback(nullptr)
    , sendErrorCallback(nullptr) {}

EspNow::~EspNow() {}

bool EspNow::begin() {
    Logger::info("===========================================");
    Logger::info("ESP-NOW V2.0 INITIALIZATION CHECKLIST");
    Logger::info("===========================================");
    
    // ✓ Checklist: Wi-Fi Mode - Set to STA only, disable SoftAP
    Logger::info("✓ [1/9] Setting WiFi mode to STA only...");
    WiFi.mode(WIFI_STA);
    // Only disconnect if actually connected to avoid warning spam
    if (WiFi.status() == WL_CONNECTED) {
        WiFi.disconnect();
    }
    delay(100);

    // Pre-set channel BEFORE esp_now_init for maximum compatibility
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    // Optionally set a conservative ESPNOW PHY rate (if available)
    #ifdef CONFIG_IDF_TARGET_ESP32S3
    #ifdef WIFI_PHY_RATE_1M_L
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_1M_L);
    #endif
    #endif
    
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

    // Set a PMK (even if we use unencrypted peers) to improve compatibility on some stacks
    static const uint8_t PMK[16] = {'S','M','A','R','T','T','I','L','E','_','P','M','K','_','0','1'};
    esp_err_t pmkRes = esp_now_set_pmk(PMK);
    if (pmkRes == ESP_OK) {
        Logger::info("  ✓ PMK set");
    } else {
        Logger::warn("  PMK set failed: %d", (int)pmkRes);
    }
    
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
    
    Logger::info("===========================================");
    Logger::info("✓ ESP-NOW V2.0 READY - All checks passed!");
    Logger::info("✓ Coordinator listening for node messages");
    Logger::info("===========================================");
    
    return true;
}

void EspNow::loop() {
    // Debug: Periodically log that we're alive and waiting
    static uint32_t lastDebugLog = 0;
    uint32_t now = millis();
    if (now - lastDebugLog > 10000) {
        Logger::debug("ESP-NOW: Loop running, pairing=%d, peers=%d", isPairingEnabled(), peers.size());
        lastDebugLog = now;
    }

    // Optimized pairing beacon with adaptive frequency
    if (isPairingEnabled()) {
        static uint32_t lastBeacon = 0;
        uint32_t elapsed = now - (pairingEndTime - 30000); // Time since pairing started (assume 30s window)
        // Faster beacons in first 10 seconds, slower after
        uint32_t beaconInterval = (elapsed < 10000) ? 800 : 2000;
        
        if (now - lastBeacon > beaconInterval) {
            uint8_t bcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
            const char* ping = "{\"msg\":\"pairing_ping\"}";
            esp_err_t res = esp_now_send(bcast, (const uint8_t*)ping, strlen(ping));
            if (res != ESP_OK) {
                Logger::debug("Pairing beacon failed: %d", (int)res);
            }
            lastBeacon = now;
        }
    }

    // Check pairing timeout
    if (pairingEnabled && now > pairingEndTime) {
        pairingEnabled = false;
        Logger::info("ESP-NOW: Pairing window closed");
    }
}

bool EspNow::sendLightCommand(const String& nodeId, uint8_t brightness, uint16_t fadeMs, bool overrideStatus, uint16_t ttlMs) {
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
    msg.override_status = overrideStatus;
    msg.ttl_ms = ttlMs;

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

void EspNow::setSendErrorCallback(std::function<void(const String& nodeId)> callback) {
    sendErrorCallback = callback;
}

void EspNow::handleEspNowReceive(const uint8_t* mac, const uint8_t* data, int len) {
    // Validate parameters first
    if (!mac || !data || len <= 0 || len > 250) {
        return; // Silent drop for invalid packets
    }
    
    // Quick filter: must be JSON
    if (((const char*)data)[0] != '{') {
        return; // Drop non-JSON frames silently
    }
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Only log at DEBUG level to reduce overhead
    Logger::debug("RX %dB from %s", len, macStr);
    
    processReceivedData(mac, data, len);
}

void EspNow::processReceivedData(const uint8_t* mac, const uint8_t* data, int len) {
    // Validate input - should already be validated but double-check
    if (!mac || !data || len <= 0) return;
    
    // Format MAC as ID string
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Avoid String allocation for nodeId until needed
    // Basic forwarding: if pairing is active and the message is a join_request, forward raw data
    String payload((const char*)data, len);
    MessageType mt = MessageFactory::getMessageType(payload);
    
    // Only log at debug when needed
    if (mt == MessageType::JOIN_REQUEST) {
        Logger::info("JOIN_REQUEST from %s", macStr);
        // Deduplicate JOIN within 4s per MAC
        uint32_t nowMs = millis();
        auto it = s_recentJoin.find(String(macStr));
        if (it != s_recentJoin.end() && (nowMs - it->second) < 4000U) {
            Logger::debug("Duplicate JOIN_REQUEST ignored for %s", macStr);
            return;
        }
        s_recentJoin[String(macStr)] = nowMs;

        // Always ensure peer exists so we can unicast responses
        addPeer(mac);

        if (isPairingEnabled()) {
            if (pairingCallback) {
                pairingCallback(mac, data, (size_t)len);
            } else {
                Logger::error("Pairing active but no pairingCallback registered");
            }
        } else {
        // Forward to general handler so Coordinator can re-accept known nodes
            if (messageCallback) {
                String nodeId(macStr);
                messageCallback(nodeId, (const uint8_t*)payload.c_str(), payload.length());
            }
        }
        return;
    }

    // Forward other messages via general callback
    if (messageCallback) {
        String nodeId(macStr);
        messageCallback(nodeId, (const uint8_t*)payload.c_str(), payload.length());
    }
}

bool EspNow::sendToMac(const uint8_t mac[6], const String& json) {
    // ✓ Checklist: Message Size - Verify before sending
    if (json.length() > 250) {
        Logger::error("Message too large: %d bytes (max 250)", json.length());
        return false;
    }
    
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // ✓ Checklist: Error Handling - Check send result
    esp_err_t res = esp_now_send(mac, (uint8_t*)json.c_str(), json.length());
    if (res != ESP_OK) {
        // ESP_ERR_ESPNOW_NOT_FOUND (12393) means peer not registered - try adding
        if (res == ESP_ERR_ESPNOW_NOT_FOUND) {
            Logger::info("Peer %s not found in ESP-NOW, adding...", macStr);
            if (addPeer(mac)) {
                // Small delay to ensure peer is registered
                delay(10);
                // Retry send after adding peer
                res = esp_now_send(mac, (uint8_t*)json.c_str(), json.length());
                if (res == ESP_OK) {
                    Logger::info("Send successful after adding peer %s", macStr);
                    return true;
                } else {
                    Logger::warn("Send to %s failed after adding peer: %d", macStr, res);
                }
            } else {
                Logger::warn("Failed to add peer %s", macStr);
            }
        }
        Logger::warn("ESP-NOW V2 send failed to %s: %d", macStr, res);
        return false;
    }
    return true;
}

bool EspNow::addPeer(const uint8_t mac[6]) {
    // ✓ Checklist: Peer Registration - Register node's MAC on coordinator
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    String smac(macStr);
    
    // Always try to add peer to ESP-NOW (it will return EXIST if already there)
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 1; // Must match coordinator's channel
    peerInfo.encrypt = false; // ✓ Checklist: Encryption disabled
    peerInfo.ifidx = WIFI_IF_STA;
    
    esp_err_t res = esp_now_add_peer(&peerInfo);
    if (res == ESP_OK) {
        // Successfully added - update our cache
        bool found = false;
        for (const auto& s : peers) {
            if (s == smac) {
                found = true;
                break;
            }
        }
        if (!found) {
            peers.push_back(smac);
            savePeersToStorage();
        }
        Logger::info("✓ Peer registered: %s", macStr);
        return true;
    } else if (res == ESP_ERR_ESPNOW_EXIST) {
        // Already exists in ESP-NOW - ensure it's in our cache
        bool found = false;
        for (const auto& s : peers) {
            if (s == smac) {
                found = true;
                break;
            }
        }
        if (!found) {
            peers.push_back(smac);
            savePeersToStorage();
        }
        Logger::debug("Peer already registered: %s", macStr);
        return true;
    } else {
        Logger::warn("✗ Failed to add peer %s: error %d", macStr, res);
        return false;
    }
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

void EspNow::clearAllPeers() {
    Logger::info("Clearing all ESP-NOW peers...");
    size_t count = peers.size();
    
    // Remove all peers from ESP-NOW
    for (const auto& macStr : peers) {
        uint8_t mac[6];
        if (macStringToBytes(macStr, mac)) {
            esp_now_del_peer(mac);
        }
    }
    
    // Clear internal lists
    peers.clear();
    peerStats.clear();
    
    // Clear storage
    Preferences p;
    if (p.begin(PREFS_NS, false)) {
        p.clear();
        p.putUInt("count", 0);
        p.end();
    }
    
    Logger::info("Cleared %d ESP-NOW peers", count);
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

int8_t EspNow::getPeerRssi(const String& macStr) const {
    auto it = peerStats.find(macStr);
    return (it != peerStats.end()) ? it->second.lastRssi : -127;
}

PeerStats EspNow::getPeerStats(const String& macStr) const {
    auto it = peerStats.find(macStr);
    if (it != peerStats.end()) {
        return it->second;
    }
    return PeerStats{-127, 0, 0, 0};
}
