#include "EspNow.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include "../../shared/src/EspNowMessage.h"
#include "../nodes/NodeRegistry.h"
#include "../utils/Logger.h"

// Simple singleton to bridge static callbacks
static EspNow* s_self = nullptr;

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

EspNow::~EspNow() {
    esp_now_deinit();
}

bool EspNow::begin() {
    // Ensure STA mode for ESP-NOW
    WiFi.mode(WIFI_STA);
    // Disable Wi-Fi power save to improve ESP-NOW RX reliability
    WiFi.setSleep(false);
    // Boost TX power for range/reliability
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.disconnect(); // Ensure we're not connected to an AP
    
    Logger::info("ESP-NOW: Initializing...");
    
    if (esp_now_init() != ESP_OK) {
        Logger::error("ESP-NOW: esp_now_init failed");
        return false;
    }
    
    // Set a fixed channel (1) AFTER esp_now_init for ESP-NOW communication
    // Both coordinator and nodes must be on the same channel
    esp_wifi_set_promiscuous(true);
    esp_err_t chRes = esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    uint8_t primary = 0; wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
    esp_wifi_get_channel(&primary, &second);
    Logger::info("ESP-NOW: Set to channel req=1 res=%d now=%d second=%d", (int)chRes, (int)primary, (int)second);
    
    s_self = this;
    esp_now_register_recv_cb(&EspNow::handleEspNowReceive);
    // Also log send results for additional visibility
    esp_now_register_send_cb([](const uint8_t* mac, esp_now_send_status_t status){
        if (!mac) {
            Logger::warn("ESP-NOW: send_cb (null mac) status=%d", (int)status);
            return;
        }
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        Logger::info("ESP-NOW: send_cb to %s status=%d", macStr, (int)status);
    });
    
    Logger::info("ESP-NOW: Receive callback registered, listening for broadcasts");
    // Add a broadcast peer for robust reception/sending of broadcast frames
    esp_now_peer_info_t bpeer{};
    memset(&bpeer, 0, sizeof(bpeer));
    memset(bpeer.peer_addr, 0xFF, 6);
    bpeer.channel = primary; // current channel
    bpeer.encrypt = false;
    esp_err_t bres = esp_now_add_peer(&bpeer);
    Logger::info("ESP-NOW: add broadcast peer result=%d", (int)bres);
    
    // Test: print MAC address
    uint8_t mac[6];
    WiFi.macAddress(mac);
    Logger::info("ESP-NOW: Coordinator MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return true;
}

void EspNow::loop() {
    // No-op for now; could handle timeouts/retries here
    if (pairingEnabled && millis() > pairingEndTime) {
        pairingEnabled = false;
    }
}

bool EspNow::sendLightCommand(const String& nodeId, uint8_t brightness, uint16_t fadeMs) {
    uint8_t mac[6];
    if (!macStringToBytes(nodeId, mac)) {
        Logger::warn("sendLightCommand: invalid MAC string %s", nodeId.c_str());
        return false;
    }

    SetLightMessage msg;
    msg.cmd_id = String((unsigned long)millis()) + String("-") + bytesToHex(mac, 6);
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
    // Add debug logging to verify we're receiving
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    Logger::info("ESP-NOW: Received %d bytes from %s", len, macStr);
    // Dump first up to 64 bytes as hex for troubleshooting
    char dump[3 * 64 + 1];
    int dumpLen = len < 64 ? len : 64;
    for (int i = 0; i < dumpLen; ++i) {
        snprintf(&dump[i * 3], 4, "%02X ", data[i]);
    }
    dump[dumpLen * 3] = '\0';
    Logger::info("ESP-NOW: RX dump: %s", dump);
    
    if (!s_self) {
        Logger::error("ESP-NOW: s_self is null!");
        return;
    }
    s_self->processReceivedData(mac, data, len);
}

void EspNow::processReceivedData(const uint8_t* mac, const uint8_t* data, int len) {
    // Format MAC as ID string
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    String nodeId = String(macStr);

    // Basic forwarding: if pairing is active and the message is a join_request, forward raw data
    String payload((const char*)data, len);
    MessageType mt = MessageFactory::getMessageType(payload);

    if (isPairingEnabled() && mt == MessageType::JOIN_REQUEST) {
        Logger::info("Pairing: join_request from %s", macStr);
        if (pairingCallback) {
            pairingCallback(mac, data, (size_t)len);
        } else {
            Logger::warn("Pairing active but no pairingCallback registered");
        }
        return;
    }

    if (messageCallback) {
        messageCallback(nodeId, (const uint8_t*)payload.c_str(), payload.length());
    }
}

bool EspNow::sendToMac(const uint8_t mac[6], const String& json) {
    esp_err_t res = esp_now_send(mac, (const uint8_t*)json.c_str(), json.length());
    if (res != ESP_OK) {
        Logger::warn("esp_now_send failed: %d", res);
        return false;
    }
    return true;
}
