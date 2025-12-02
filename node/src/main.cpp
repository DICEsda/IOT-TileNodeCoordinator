// Standalone test mode for SK6812B control without coordinator
#ifdef STANDALONE_TEST

#include <Arduino.h>
#include "led/LedController.h"
#include "input/ButtonInput.h"
#include "config/PinConfig.h"
#include "utils/OtaUpdater.h"

static constexpr uint16_t LED_NUM_PIXELS = 4; // default per PRD; adjust to your strip

enum class UiState { NORMAL, PAIRING };

class StandaloneNode {
public:
    void begin() {
        Serial.begin(115200);
        delay(200);
        led = new LedController(LED_NUM_PIXELS);
        led->begin();
        led->setBrightness(64);
        setMode(0);

        button.begin(Pins::BUTTON);
        button.onPress([this]() { this->onShortPress(); });
        button.onLongPress([this]() { this->togglePairing(); }, 2000);
    }

    void loop() {
        button.loop();
        led->update();
        if (state == UiState::PAIRING) {
            pairingAnimation();
            // Auto-timeout pairing; show failure flash
            if (millis() - pairingStartMs >= pairingTimeoutMs) {
                pairingFailedExit();
            }
        } else if (mode == 3) {
            // White breathing demo when not pairing
            whiteBreatheAnimation();
        }
        handleSerial();
    }

private:
    LedController* led = nullptr;
    ButtonInput button;
    UiState state = UiState::NORMAL;
    uint8_t mode = 0; // cycles through demo modes
    uint32_t lastAnimMs = 0;
    uint8_t rainbowHue = 0;
    String serialBuf;
    uint32_t pairingStartMs = 0;
    uint32_t pairingTimeoutMs = 10000; // 10 seconds

    void setMode(uint8_t m) {
        mode = m % 4;
        switch (mode) {
            case 0: // Off
                led->setBrightness(0);
                led->setColor(0, 0, 0, 0);
                break;
            case 1: // White low (use dedicated white channel)
                led->setBrightness(32);
                led->setColor(0, 0, 0, 255, 300);
                break;
            case 2: // White high (use dedicated white channel)
                led->setBrightness(128);
                led->setColor(0, 0, 0, 255, 300);
                break;
            case 3: // White breathe demo (use W channel only)
                led->setBrightness(96);
                // color/brightness will be updated by animation loop
                break;
        }
    }

    void onShortPress() {
        if (state == UiState::PAIRING) return; // ignore mode change during pairing
        setMode(mode + 1);
    }

    void togglePairing() {
        if (state == UiState::PAIRING) {
            state = UiState::NORMAL;
            setMode(mode); // restore current mode visuals
            // Short green confirmation flash (status)
            led->setBrightness(140);
            led->setColor(0, 255, 0, 0); // green via RGB
            delay(150);
            led->setBrightness(0);
            led->setColor(0, 0, 0, 0);
            delay(80);
            Serial.println("Exit pairing mode");
        } else {
            state = UiState::PAIRING;
            pairingStartMs = millis();
            lastAnimMs = 0;
            Serial.println("Enter pairing mode (hold for 2s)");
        }
    }

    void pairingAnimation() {
        // Pulse blue smoothly at ~1Hz using RGB channel (status)
        uint32_t now = millis();
        float t = (now % 1000) / 1000.0f; // 0..1
        // triangle wave 0..1..0
        float tri = t < 0.5f ? (t * 2.0f) : (2.0f - t * 2.0f);
        uint8_t b = 40 + (uint8_t)(tri * 120);
        led->setBrightness(b);
        led->setColor(0, 0, 255, 0); // blue via RGB
    }

    void whiteBreatheAnimation() {
        // Smooth breathing on white channel (~2s period)
        uint32_t now = millis();
        float t = (now % 2000) / 2000.0f; // 0..1 over 2 seconds
        float tri = t < 0.5f ? (t * 2.0f) : (2.0f - t * 2.0f);
        uint8_t b = 16 + (uint8_t)(tri * 112); // 16..128
        led->setBrightness(b);
        led->setColor(0, 0, 0, 255); // keep W at max; brightness modulates output
    }

    void pairingFailedExit() {
        Serial.println("Pairing timeout: showing failure flash");
        // Red flash sequence (2 pulses) using RGB channel (status)
        for (int i = 0; i < 2; ++i) {
            led->setBrightness(150);
            led->setColor(255, 0, 0, 0); // red
            delay(180);
            led->setBrightness(0);
            led->setColor(0, 0, 0, 0);
            delay(120);
        }
        state = UiState::NORMAL;
        setMode(mode);
    }

    void handleSerial() {
        while (Serial.available()) {
            char c = (char)Serial.read();
            if (c == '\n' || c == '\r') {
                processLine(serialBuf);
                serialBuf = "";
            } else if (serialBuf.length() < 255) {
                serialBuf += c;
            }
        }
    }

    void processLine(const String& line) {
        // ota <ssid> <pass> <url> [md5]
        if (line.startsWith("ota ")) {
            Serial.println("OTA requested");
            auto rest = line.substring(4);
            int s1 = rest.indexOf(' ');
            int s2 = rest.indexOf(' ', s1 + 1);
            int s3 = rest.indexOf(' ', s2 + 1);
            if (s1 < 0 || s2 < 0) {
                Serial.println("Usage: ota <ssid> <pass> <url> [md5]");
                return;
            }
            String ssid = rest.substring(0, s1);
            String pass = rest.substring(s1 + 1, s2);
            String url = s3 > 0 ? rest.substring(s2 + 1, s3) : rest.substring(s2 + 1);
            String md5 = s3 > 0 ? rest.substring(s3 + 1) : "";
            Serial.printf("Connecting WiFi SSID='%s'...\n", ssid.c_str());
            if (!OtaUpdater::ensureWifi(ssid.c_str(), pass.c_str())) {
                Serial.println("WiFi connect failed");
                return;
            }
            Serial.printf("Downloading: %s\n", url.c_str());
            auto res = OtaUpdater::updateFromUrl(url.c_str(), md5.length() ? md5.c_str() : nullptr);
            Serial.printf("OTA: ok=%d http=%d msg=%s\n", (int)res.ok, res.httpCode, res.message.c_str());
            if (res.ok) {
                delay(500);
                ESP.restart();
            }
        }
    }
};

static StandaloneNode app;

void setup() { app.begin(); }
void loop() { app.loop(); }

#else

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <esp_sleep.h>
#include <esp_random.h>

#include "EspNowMessage.h"
#include "ConfigManager.h"
// RGBW LED + button
#include "led/LedController.h"
#include "input/ButtonInput.h"
#include "config/PinConfig.h"
#include <Adafruit_TMP117.h>

// Node state machine
enum class NodeState { PAIRING, OPERATIONAL, UPDATE, REBOOT };

// Forward declarations for ESP-NOW v2 static callbacks
class SmartTileNode;
static SmartTileNode* gNodeInstance = nullptr;
static void espnowRecv(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len);
static void espnowSent(const uint8_t* mac, esp_now_send_status_t status);

class SmartTileNode {
private:
    NodeState currentState;
    ConfigManager config;
    
    // ESP-NOW
    uint8_t coordinatorMac[6];
    bool espNowInitialized;
    uint8_t currentScanChannel;      // Current index in channel scan order
    uint32_t lastChannelScanTime;    // Last time we switched channels
    static constexpr uint8_t SCAN_CHANNELS[] = {1, 6, 11, 2, 3, 4, 5, 7, 8, 9, 10, 12, 13};
    static constexpr uint8_t NUM_SCAN_CHANNELS = 13;
    
    // RGBW LED strip
    LedController leds{4}; // PRD: 4 pixels per node
    uint8_t curR = 0, curG = 0, curB = 0, curW = 0;
    bool statusOverrideActive = false;
    uint32_t statusOverrideUntilMs = 0;
    
    // Temperature sensor (Adafruit TMP117)
    Adafruit_TMP117 tempSensor;
    bool tempSensorAvailable = false;
    
    // Power management
    uint32_t lastRxWindow;
    uint16_t rxWindowMs;
    uint16_t rxPeriodMs;
    uint32_t lastTelemetry;
    uint16_t telemetryInterval;
    
    // Channel management
    bool channelLocked = false; // Set to true once we find coordinator
    uint8_t lockedChannel = 0;  // The channel we locked to
    
    // Button
    ButtonInput button;
    uint32_t pairingStartTime;
    bool inPairingMode;
    
    // Node info
    String nodeId;
    String lightId;
    String firmwareVersion;
    
    // Command tracking
    String lastCmdId;
    uint32_t lastCommandTime;
    
public:
    SmartTileNode();
    ~SmartTileNode();
    
    bool begin();
    void loop();
    
private:
    // State machine
    void handlePairing();
    void handleOperational();
    // Derate handling removed; coordinator clamps brightness
    void handleUpdate();
    void handleReboot();
    
    // ESP-NOW
    bool initEspNow();
public:
    // Expose callbacks for static trampolines
    void onDataRecv(const uint8_t* mac, const uint8_t* data, int len);
    void onDataSent(const uint8_t* mac, esp_now_send_status_t status);
private:
    bool sendMessage(const EspNowMessage& message, const uint8_t* destMac = nullptr);
    void processReceivedMessage(const String& json);
    bool ensureEncryptedPeer(const uint8_t mac[6], const String& lmkHex);
    static bool parseHex16(const String& hex, uint8_t out[16]);
    
    // LED control
    void applyColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint16_t fadeMs);
    
    // Temperature sensing removed
    
    // Power management
    void enterLightSleep();
    bool isRxWindowActive();
    
    // Button handling
    void handleButton();
    void startPairing();
    void stopPairing();
    
    // Telemetry
    void sendTelemetry();
    uint16_t readBatteryVoltage();
    
    // Reconnection logic
    uint32_t lastCoordinatorResponse;
    uint32_t telemetrySentCount;
    static const uint32_t COORDINATOR_TIMEOUT_MS = 300000; // 5 minutes without response
    static const uint32_t TELEMETRY_THRESHOLD = 50; // 50 telemetry attempts before re-pair
    void checkCoordinatorConnection();
    
    // Configuration
    void loadConfiguration();
    void saveConfiguration();
    
    // Utility
    String generateCmdId();
    String getMacAddress();
    void logMessage(const String& level, const String& message);
};

SmartTileNode::SmartTileNode() 
    : currentState(NodeState::PAIRING)
    , config("node")
    , espNowInitialized(false)
    , currentScanChannel(0)
    , lastChannelScanTime(0)
    , tempSensorAvailable(false)
    , lastRxWindow(0)
    , rxWindowMs(20)
    , rxPeriodMs(100)
    , lastTelemetry(0)
    , telemetryInterval(2)  // 2 seconds - balance between real-time updates and channel load
    , pairingStartTime(0)
    , inPairingMode(false)
    , firmwareVersion("c3-1.0.0")
    , lastCommandTime(0)
    , lastCoordinatorResponse(0)
    , telemetrySentCount(0) {
}

SmartTileNode::~SmartTileNode() {
    config.end();
}

bool SmartTileNode::begin() {
    Serial.begin(115200);
    delay(1000);
    
    logMessage("INFO", "Smart Tile Node starting...");
    
    // Initialize configuration
    if (!config.begin()) {
        logMessage("ERROR", "Failed to initialize configuration");
        return false;
    }
    
    // Load configuration
    loadConfiguration();
    
    // Initialize LEDs
    leds.begin();
    leds.setBrightness(60);
    
    // Initialize button
    button.begin(Pins::BUTTON);
    button.onLongPress([this]() { this->startPairing(); }, 2000);
    
    // Initialize I2C and temperature sensor (Adafruit TMP117)
    Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);
    Wire.setClock(100000); // 100kHz for stability
    
    // I2C Scanner - Debug output
    logMessage("INFO", String("Scanning I2C bus (SDA=") + String(Pins::I2C_SDA) + ", SCL=" + String(Pins::I2C_SCL) + ")...");
    int devicesFound = 0;
    for (uint8_t addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        uint8_t error = Wire.endTransmission();
        if (error == 0) {
            devicesFound++;
            logMessage("INFO", String("I2C device found at 0x") + String(addr, HEX));
        }
    }
    if (devicesFound == 0) {
        logMessage("WARN", "No I2C devices found! Check wiring and pin configuration.");
    } else {
        logMessage("INFO", String("Found ") + String(devicesFound) + " I2C device(s)");
    }
    
    // Try to initialize TMP117 (default address is 0x48)
    if (tempSensor.begin()) {
        tempSensorAvailable = true;
        logMessage("INFO", "Adafruit TMP117 temperature sensor initialized");
    } else {
        tempSensorAvailable = false;
        logMessage("WARN", "TMP117 sensor not found at default address 0x48 - will report 0.0C");
    }
    
    // Initialize ESP-NOW
    if (!initEspNow()) {
        logMessage("ERROR", "Failed to initialize ESP-NOW");
        return false;
    }
    
    // ALWAYS start in PAIRING mode until we receive JOIN_ACCEPT
    // This ensures the node can pair even without a button
    currentState = NodeState::PAIRING;
    lastCoordinatorResponse = millis();
    
    // CRITICAL: Start pairing mode (sets inPairingMode flag)
    startPairing();
    
    if (config.exists(ConfigKeys::NODE_ID) && config.exists(ConfigKeys::LIGHT_ID)) {
        logMessage("INFO", "Found stored credentials, starting in PAIRING mode (will auto-reconnect on JOIN_ACCEPT)");
    } else {
        logMessage("INFO", "No credentials found, starting in PAIRING mode (waiting for JOIN_ACCEPT)");
    }
    
    return true;
}

void SmartTileNode::loop() {
    handleButton();
    leds.update();
    
    switch (currentState) {
        case NodeState::PAIRING:
            handlePairing();
            break;
        case NodeState::OPERATIONAL:
            handleOperational();
            checkCoordinatorConnection(); // Check if coordinator is still responsive
            break;
        // Derate state removed; coordinator handles thermal clamping
        case NodeState::UPDATE:
            handleUpdate();
            break;
        case NodeState::REBOOT:
            handleReboot();
            break;
    }
    
    // Send telemetry periodically
    if (millis() - lastTelemetry > telemetryInterval * 500) {
        sendTelemetry();
        lastTelemetry = millis();
        telemetrySentCount++;
    }
    
    // Power management DISABLED - light sleep causes USB disconnect/reboot behavior
    // TODO: Re-enable for battery-powered nodes without USB
    // bool animating = leds.isAnimating();
    // if (currentState == NodeState::OPERATIONAL && !inPairingMode && !animating) {
    //     if (!isRxWindowActive()) {
    //         enterLightSleep();
    //     }
    // }

    // Smaller delay when animating for smoother visuals
    delay(leds.isAnimating() ? 1 : 10);
}

void SmartTileNode::handlePairing() {
    if (!inPairingMode) {
        // Wait for button press to start pairing
        return;
    }
    
    // Check if pairing window expired
    if (millis() - pairingStartTime > config.getInt(ConfigKeys::PAIRING_WINDOW_S, 120) * 1000) {
        stopPairing();
        return;
    }
    
    // Channel scanning DISABLED - hardcoded to channel 1 to match coordinator
    // No need to scan channels since coordinator is always on channel 1
    
    // Channel hop during pairing to find coordinator (only if not locked)
    static uint32_t lastChannelHop = 0;
    static uint8_t currentChannelIndex = 0;
    static const uint8_t CHANNELS[] = {1, 6, 11, 2, 3, 4, 5, 7, 8, 9, 10};
    static const int NUM_CHANNELS = sizeof(CHANNELS) / sizeof(CHANNELS[0]);
    
    // Only hop channels if we haven't locked onto coordinator yet
    if (!channelLocked && millis() - lastChannelHop > 500) {
        currentChannelIndex = (currentChannelIndex + 1) % NUM_CHANNELS;
        uint8_t newChannel = CHANNELS[currentChannelIndex];
        
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(newChannel, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_promiscuous(false);
        
        // Update broadcast peer to use channel 0 (current)
        uint8_t bcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        esp_now_del_peer(bcast);
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, bcast, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;
        peerInfo.ifidx = WIFI_IF_STA;
        esp_now_add_peer(&peerInfo);
        
        logMessage("DEBUG", String("Channel hop to ") + String(newChannel));
        lastChannelHop = millis();
    }
    
    // Send join request periodically during pairing
    static uint32_t lastJoinRequest = 0;
    if (millis() - lastJoinRequest > 600) { // Every 600ms to sync with channel hops
        JoinRequestMessage joinReq;
        joinReq.mac = getMacAddress();
        joinReq.fw = firmwareVersion;
        joinReq.caps.rgbw = true;
        joinReq.caps.led_count = leds.numPixels();
        joinReq.caps.temp_i2c = false;
        joinReq.caps.deep_sleep = true;
        joinReq.caps.button = true;
        joinReq.token = String(esp_random(), HEX);

        String payload = joinReq.toJson();
        
        // ✓ Checklist: Message Size check
        if (payload.length() > 250) {
            logMessage("ERROR", "JOIN_REQUEST too large!");
            return;
        }
        
        // Log the actual payload being sent
        uint8_t curCh = 0;
        wifi_second_chan_t sec;
        esp_wifi_get_channel(&curCh, &sec);
        logMessage("INFO", String("Sending JOIN_REQUEST on ch") + String(curCh) + ": " + payload);
        
        // Send to broadcast MAC address using native ESP-NOW v2
        uint8_t broadcastMac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        
        // Ensure broadcast peer exists
        if (!esp_now_is_peer_exist(broadcastMac)) {
            logMessage("WARN", "Broadcast peer missing! Re-adding...");
            esp_now_peer_info_t peerInfo = {};
            memcpy(peerInfo.peer_addr, broadcastMac, 6);
            peerInfo.channel = 0; // Use current channel
            peerInfo.encrypt = false;
            peerInfo.ifidx = WIFI_IF_STA;
            esp_now_add_peer(&peerInfo);
        }
        
        esp_err_t result = esp_now_send(broadcastMac, (uint8_t*)payload.c_str(), payload.length());
        
        lastJoinRequest = millis();
        if (result == ESP_OK) {
            logMessage("INFO", String("JOIN_REQUEST sent successfully (") + String(payload.length()) + " bytes)");
        } else {
            logMessage("ERROR", String("JOIN_REQUEST send failed: ") + String((int)result));
        }
    }
}

void SmartTileNode::handleOperational() {
    // Normal operation - ESP-NOW messages are handled in callback
}

// Derate handler removed

void SmartTileNode::handleUpdate() {
    // OTA update logic would go here
    // For now, just reboot
    currentState = NodeState::REBOOT;
}

void SmartTileNode::handleReboot() {
    logMessage("INFO", "Rebooting...");
    delay(1000);
    ESP.restart();
}

bool SmartTileNode::initEspNow() {
    logMessage("INFO", "===========================================");
    logMessage("INFO", "ESP-NOW V2.0 INITIALIZATION (NODE)");
    logMessage("INFO", "===========================================");
    
    // ✓ Checklist: Wi-Fi Mode - Set to STA only
    logMessage("INFO", "[1/9] Setting WiFi mode to STA...");
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.disconnect();
    delay(100);
    
    if (WiFi.getMode() != WIFI_STA) {
        logMessage("ERROR", "WiFi mode is not STA!");
        return false;
    }
    
    // ✓ Checklist: MAC Address - Get programmatically
    logMessage("INFO", "[2/9] Getting MAC address...");
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    logMessage("INFO", String("  Node MAC: ") + String(macStr));
    
    // ✓ Checklist: ESP-NOW Version - Initialize v2.0
    logMessage("INFO", "[3/9] Initializing ESP-NOW v2.0...");
    esp_err_t initResult = esp_now_init();
    if (initResult != ESP_OK) {
        logMessage("ERROR", String("esp_now_init() failed: ") + String((int)initResult));
        return false;
    }
    logMessage("INFO", "  ESP-NOW v2.0 initialized");
    
    // Scan common WiFi channels to find coordinator
    // The coordinator's channel depends on the WiFi router it's connected to
    logMessage("INFO", "[4/9] Scanning for coordinator channel...");
    
    // Common non-overlapping WiFi channels (most routers use one of these)
    static const uint8_t CHANNELS_TO_SCAN[] = {1, 6, 11, 2, 3, 4, 5, 7, 8, 9, 10, 12, 13};
    static const int NUM_CHANNELS = sizeof(CHANNELS_TO_SCAN) / sizeof(CHANNELS_TO_SCAN[0]);
    
    uint8_t foundChannel = 1; // Default to channel 1
    
    // Start with channel 1 as default - we'll update when we receive from coordinator
    esp_wifi_set_promiscuous(true);
    esp_err_t chRes = esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    
    uint8_t primary = 0; 
    wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
    esp_wifi_get_channel(&primary, &second);
    
    logMessage("INFO", String("  Starting on channel ") + String((int)primary) + " (will auto-detect from coordinator)");
    
    // Set WiFi protocol for compatibility
    logMessage("INFO", "[5/9] Setting WiFi protocol...");
    esp_err_t protocolRes = esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N);
    if (protocolRes == ESP_OK) {
        logMessage("INFO", "  WiFi protocol set (802.11b/g/n)");
    }
    
    // ✓ Checklist: Callbacks - Register v2 callbacks
    logMessage("INFO", "[6/9] Registering ESP-NOW v2 callbacks...");
    gNodeInstance = this;
    
    esp_err_t recvResult = esp_now_register_recv_cb(espnowRecv);
    if (recvResult != ESP_OK) {
        logMessage("ERROR", String("Failed to register recv callback: ") + String((int)recvResult));
        return false;
    }
    
    esp_err_t sendResult = esp_now_register_send_cb(espnowSent);
    if (sendResult != ESP_OK) {
        logMessage("ERROR", String("Failed to register send callback: ") + String((int)sendResult));
        return false;
    }
    logMessage("INFO", "  Callbacks registered");
    
    // ✓ Checklist: Peer Registration - Add broadcast peer
    logMessage("INFO", "[7/9] Adding broadcast peer...");
    uint8_t bcast[6]; 
    memset(bcast, 0xFF, 6);
    
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, bcast, 6);
    peerInfo.channel = 0; // 0 = use current interface channel (most flexible)
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;
    
    esp_err_t bres = esp_now_add_peer(&peerInfo);
    if (bres == ESP_OK || bres == ESP_ERR_ESPNOW_EXIST) {
        logMessage("INFO", "  Broadcast peer added (FF:FF:FF:FF:FF:FF) on channel 0");
    } else {
        logMessage("ERROR", String("Failed to add broadcast peer: ") + String((int)bres));
    }
    
    logMessage("INFO", "[8/9] Setting TX power to maximum...");
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    
    logMessage("INFO", "[9/9] ESP-NOW v2.0 initialization complete");
    logMessage("INFO", "===========================================");
    
    espNowInitialized = true;
    return true;
}

void SmartTileNode::onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    // Log every received message for debugging
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    logMessage("DEBUG", String("RX ") + String(len) + "B from " + String(macStr));
    
    String message = String((char*)data, len);
    logMessage("DEBUG", String("RX data: ") + message);
    
    // Get the channel we received this message on - this is the coordinator's channel!
    uint8_t rxChannel = 0;
    wifi_second_chan_t sec;
    esp_wifi_get_channel(&rxChannel, &sec);
    
    // Lock channel once we receive from coordinator (stops channel hopping)
    if (!channelLocked && rxChannel > 0) {
        channelLocked = true;
        lockedChannel = rxChannel;
        logMessage("INFO", String("CHANNEL LOCKED to ") + String(rxChannel) + " - coordinator found!");
    }
    
    // Remember coordinator MAC
    bool isNewCoordinator = (coordinatorMac[0] == 0 && coordinatorMac[1] == 0 && 
                             coordinatorMac[2] == 0 && coordinatorMac[3] == 0 && 
                             coordinatorMac[4] == 0 && coordinatorMac[5] == 0);
    
    if (mac) {
        if (isNewCoordinator) {
            memcpy(coordinatorMac, mac, 6);
            logMessage("INFO", String("Learned coordinator MAC: ") + String(macStr) + " on channel " + String(rxChannel));
        }
        
        // Ensure coordinator peer is registered with channel 0 (current interface channel)
        // Channel 0 means "use the current WiFi interface channel" which is most reliable
        esp_now_peer_info_t peerInfo = {};
        if (esp_now_get_peer(coordinatorMac, &peerInfo) == ESP_OK) {
            // Peer exists - check if channel needs update to 0
            if (peerInfo.channel != 0) {
                esp_now_del_peer(coordinatorMac);
                memcpy(peerInfo.peer_addr, coordinatorMac, 6);
                peerInfo.channel = 0; // 0 = use current interface channel
                peerInfo.encrypt = false;
                peerInfo.ifidx = WIFI_IF_STA;
                esp_now_add_peer(&peerInfo);
                logMessage("INFO", String("Updated coordinator peer to channel 0"));
            }
        } else {
            // Peer doesn't exist - add it with channel 0
            memcpy(peerInfo.peer_addr, coordinatorMac, 6);
            peerInfo.channel = 0; // 0 = use current interface channel
            peerInfo.encrypt = false;
            peerInfo.ifidx = WIFI_IF_STA;
            esp_now_add_peer(&peerInfo);
            logMessage("INFO", String("Added coordinator peer on channel 0"));
        }
    }
    
    // mark RX window and coordinator response
    lastRxWindow = millis();
    lastCoordinatorResponse = millis();
    telemetrySentCount = 0; // Reset counter on any response
    
    processReceivedMessage(message);
}

void SmartTileNode::onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        logMessage("DEBUG", "Message sent successfully");
    } else {
        logMessage("WARN", "Message send failed");
    }
}

void SmartTileNode::processReceivedMessage(const String& json) {
    // Ignore health check pings from coordinator (not part of ESP-NOW message protocol)
    if (json.indexOf("\"ping\"") >= 0 || json.indexOf("\"pairing_ping\"") >= 0) {
        // Silently ignore - these are just keep-alive messages
        return;
    }
    
    EspNowMessage* message = MessageFactory::createMessage(json);
    if (!message) {
        logMessage("ERROR", "Failed to parse message");
        return;
    }
    
    switch (message->type) {
        case MessageType::JOIN_ACCEPT: {
            JoinAcceptMessage* accept = static_cast<JoinAcceptMessage*>(message);
            nodeId = accept->node_id;
            lightId = accept->light_id;
            
            config.setString(ConfigKeys::NODE_ID, nodeId);
            config.setString(ConfigKeys::LIGHT_ID, lightId);
            config.setString(ConfigKeys::LMK, accept->lmk);
            
            // Update configuration from coordinator
            config.setInt(ConfigKeys::RX_WINDOW_MS, accept->cfg.rx_window_ms);
            config.setInt(ConfigKeys::RX_PERIOD_MS, accept->cfg.rx_period_ms);
            
            // CRITICAL: Switch to coordinator's WiFi channel
            if (accept->wifi_channel > 0 && accept->wifi_channel <= 13) {
                logMessage("INFO", String("Switching to coordinator's WiFi channel: ") + String(accept->wifi_channel));
                esp_wifi_set_promiscuous(true);
                esp_wifi_set_channel(accept->wifi_channel, WIFI_SECOND_CHAN_NONE);
                esp_wifi_set_promiscuous(false);
                
                // Remove ALL peers before switching
                uint8_t broadcast[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
                esp_now_del_peer(broadcast);
                if (coordinatorMac[0] != 0) {
                    esp_now_del_peer(coordinatorMac);
                }
                
                // Re-add broadcast peer on new channel
                esp_now_peer_info_t bcastPeer = {};
                memcpy(bcastPeer.peer_addr, broadcast, 6);
                bcastPeer.channel = accept->wifi_channel;
                bcastPeer.encrypt = false;
                bcastPeer.ifidx = WIFI_IF_STA;
                esp_now_add_peer(&bcastPeer);
                
                // Re-add coordinator peer on new channel
                esp_now_peer_info_t coordPeer = {};
                memcpy(coordPeer.peer_addr, coordinatorMac, 6);
                coordPeer.channel = accept->wifi_channel;
                coordPeer.encrypt = false;
                coordPeer.ifidx = WIFI_IF_STA;
                esp_now_add_peer(&coordPeer);
                
                logMessage("INFO", String("All peers updated to channel ") + String(accept->wifi_channel));
            } else {
                // No channel switch needed, just ensure coordinator peer exists
                ensureEncryptedPeer(coordinatorMac, accept->lmk);
            }
            
            saveConfiguration();
            
            // Reset reconnection tracking
            lastCoordinatorResponse = millis();
            telemetrySentCount = 0;
            
            // Send status and switch to operational
            sendTelemetry();
            currentState = NodeState::OPERATIONAL;
            stopPairing();
            leds.setStatus(LedController::StatusMode::None);
            
            logMessage("INFO", "Successfully paired with coordinator");
            break;
        }
        case MessageType::SET_LIGHT: {
            SetLightMessage* setLight = static_cast<SetLightMessage*>(message);
            // Accept command if light_id matches OR if light_id is empty (broadcast/any)
            if (setLight->light_id == lightId || setLight->light_id.isEmpty()) {
                // Only transition to OPERATIONAL if we have valid credentials (received JOIN_ACCEPT before)
                // This prevents incorrect transitions from stray broadcasts
                if ((currentState == NodeState::PAIRING || inPairingMode) && !lightId.isEmpty()) {
                    logMessage("INFO", "Received command while pairing with valid credentials - switching to OPERATIONAL");
                    currentState = NodeState::OPERATIONAL;
                    stopPairing();
                } else if (currentState == NodeState::PAIRING && lightId.isEmpty()) {
                    // Ignore commands if we don't have a lightId yet - wait for JOIN_ACCEPT
                    logMessage("WARN", "Ignoring set_light - no lightId yet, waiting for JOIN_ACCEPT");
                    delete message;
                    return;
                }
                
                // Always clear status animation when receiving manual commands
                leds.setStatus(LedController::StatusMode::None);
                statusOverrideActive = true;
                statusOverrideUntilMs = millis() + (setLight->ttl_ms > 0 ? setLight->ttl_ms : 10000);

                uint8_t r = setLight->r, g = setLight->g, b = setLight->b, w = setLight->w;
                if (r == 0 && g == 0 && b == 0 && w == 0) {
                    // fallback: map value to white channel
                    w = setLight->value;
                }
                
                // Check if this is a per-pixel command or all pixels
                if (setLight->pixel >= 0 && setLight->pixel < leds.numPixels()) {
                    // Per-pixel control
                    leds.setPixelColor(setLight->pixel, r, g, b, w);
                    leds.show();
                    // Update current color tracking (use this pixel's color for telemetry)
                    curR = r; curG = g; curB = b; curW = w;
                    logMessage("INFO", String("Set pixel ") + String(setLight->pixel) + " to RGBW(" + String(r) + "," + String(g) + "," + String(b) + "," + String(w) + ")");
                } else {
                    // All pixels
                    applyColor(r, g, b, w, setLight->fade_ms);
                }
                
                lastCmdId = setLight->cmd_id;
                lastCommandTime = millis();
                
                // Send acknowledgment
                AckMessage ack;
                ack.cmd_id = setLight->cmd_id;
                sendMessage(ack);
                
                logMessage("INFO", "Received set_light command");
            }
            break;
        }
        case MessageType::ACK: {
            // Coordinator acknowledged our telemetry - reset connection timeout
            lastCoordinatorResponse = millis();
            telemetrySentCount = 0;
            logMessage("DEBUG", "Received ACK from coordinator");
            break;
        }
        default:
            logMessage("WARN", "Unknown message type received");
            break;
    }
    
    delete message;
}

void SmartTileNode::applyColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint16_t fadeMs) {
    curR = r; curG = g; curB = b; curW = w;
    leds.setColor(r, g, b, w, fadeMs);
}

// Thermal derating function removed

// Temperature sensor functions removed
// Removed dangling references to temperature sensor initialization for build correctness

void SmartTileNode::enterLightSleep() {
    // Configure light sleep with RX window
    esp_sleep_enable_timer_wakeup(rxPeriodMs * 1000);
    esp_light_sleep_start();
    
    lastRxWindow = millis();
}

bool SmartTileNode::isRxWindowActive() {
    return (millis() - lastRxWindow) < rxWindowMs;
}

void SmartTileNode::handleButton() {
    button.loop();
    // Clear status override when TTL expires - but stay in None mode (don't revert to animations)
    if (statusOverrideActive && millis() > statusOverrideUntilMs) {
        statusOverrideActive = false;
        // Only go back to pairing animation if we're actually in PAIRING state
        // In OPERATIONAL, stay in None mode so manual control persists
        if (currentState == NodeState::PAIRING && inPairingMode) {
            leds.setStatus(LedController::StatusMode::Pairing);
        }
        // Otherwise keep StatusMode::None so LED stays at last commanded color
    }
}

void SmartTileNode::startPairing() {
    inPairingMode = true;
    pairingStartTime = millis();
    leds.setStatus(LedController::StatusMode::Pairing);
    logMessage("INFO", "Pairing mode started");
}

void SmartTileNode::stopPairing() {
    inPairingMode = false;
    leds.setStatus(LedController::StatusMode::None);
    // Set default WHITE color so the blue animation doesn't persist
    leds.setColor(0, 0, 0, 255, 0);  // W channel ON immediately
    curR = 0; curG = 0; curB = 0; curW = 255;
    logMessage("INFO", "Pairing mode stopped - default WHITE enabled");
}

void SmartTileNode::sendTelemetry() {
    NodeStatusMessage status;
    status.node_id = nodeId;
    status.light_id = nodeId; // Use node_id as light_id for compatibility
    status.avg_r = curR; status.avg_g = curG; status.avg_b = curB; status.avg_w = curW;
    status.status_mode = (currentState == NodeState::PAIRING) ? "pairing"
                        : (statusOverrideActive ? "override" : "operational");
    status.fw = firmwareVersion;
    status.vbat_mv = readBatteryVoltage();
    
    // Read temperature from Adafruit TMP117
    if (tempSensorAvailable) {
        sensors_event_t temp;
        tempSensor.getEvent(&temp);
        status.temperature = temp.temperature;
        logMessage("INFO", String("TMP117 Temperature: ") + String(status.temperature, 2) + "C");
    } else {
        status.temperature = 0.0f;
        logMessage("WARN", "TMP117 not available - reporting 0.0C");
    }
    
    sendMessage(status);
}

uint16_t SmartTileNode::readBatteryVoltage() {
    // Placeholder for battery voltage reading
    // Implement based on your hardware
    return 3700; // Simulated voltage
}

void SmartTileNode::loadConfiguration() {
    nodeId = config.getString(ConfigKeys::NODE_ID);
    lightId = config.getString(ConfigKeys::LIGHT_ID);
    
    telemetryInterval = config.getInt(ConfigKeys::TELEMETRY_INTERVAL_S, Defaults::TELEMETRY_INTERVAL_S);
    rxWindowMs = config.getInt(ConfigKeys::RX_WINDOW_MS, Defaults::RX_WINDOW_MS);
    rxPeriodMs = config.getInt(ConfigKeys::RX_PERIOD_MS, Defaults::RX_PERIOD_MS);
    // Derating thresholds not used on node anymore
}

void SmartTileNode::saveConfiguration() {
    // Configuration is automatically saved when set methods are called
    logMessage("INFO", "Configuration saved");
}

String SmartTileNode::generateCmdId() {
    return String(esp_random(), HEX) + "-" + String(millis(), HEX);
}

String SmartTileNode::getMacAddress() {
    uint8_t mac[6];
    esp_wifi_get_mac(WIFI_IF_STA, mac);
    
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", 
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    return String(macStr);
}

bool SmartTileNode::sendMessage(const EspNowMessage& message, const uint8_t* destMac) {
    String json = message.toJson();
    
    // ✓ Checklist: Message Size - Check before sending
    if (json.length() > 250) {
        logMessage("ERROR", String("Message too large: ") + String(json.length()) + " bytes");
        return false;
    }
    
    const uint8_t* target = destMac;
    uint8_t broadcast[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    if (!target) {
        bool known = false;
        for (int i=0;i<6;i++) { if (coordinatorMac[i] != 0) { known = true; break; } }
        target = known ? coordinatorMac : broadcast;
    }
    
    // CRITICAL: Ensure peer exists before sending
    if (!esp_now_is_peer_exist(target)) {
        // Add peer if it doesn't exist
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, target, 6);
        peerInfo.channel = 0; // 0 = use current interface channel (most reliable)
        peerInfo.encrypt = false;
        peerInfo.ifidx = WIFI_IF_STA;
        
        esp_err_t addRes = esp_now_add_peer(&peerInfo);
        if (addRes != ESP_OK && addRes != ESP_ERR_ESPNOW_EXIST) {
            logMessage("WARN", String("Failed to add peer: ") + String((int)addRes));
            return false;
        }
        logMessage("INFO", String("Added peer on channel 0"));
    }
    
    // ✓ Checklist: Use native ESP-NOW v2 API
    esp_err_t res = esp_now_send(target, (uint8_t*)json.c_str(), json.length());
    if (res != ESP_OK) {
        logMessage("WARN", String("esp_now_send failed: ") + String((int)res));
        return false;
    }
    return true;
}

bool SmartTileNode::ensureEncryptedPeer(const uint8_t mac[6], const String& /*lmkHex*/) {
    if (!mac) return false;
    
    // ✓ Checklist: Peer Registration - Use native ESP-NOW v2
    // Remove old peer if exists
    esp_now_del_peer(mac);
    
    // Add as unencrypted peer
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 1;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;
    
    esp_err_t res = esp_now_add_peer(&peerInfo);
    if (res == ESP_OK || res == ESP_ERR_ESPNOW_EXIST) {
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        logMessage("INFO", String("Peer registered: ") + String(macStr));
        return true;
    }
    logMessage("ERROR", String("Failed to add peer: ") + String((int)res));
    return false;
}

bool SmartTileNode::parseHex16(const String& hex, uint8_t out[16]) {
    if (!out) return false;
    String s = hex;
    // remove potential separators
    s.replace(":", ""); s.replace("-", ""); s.replace(" ", "");
    if (s.length() < 32) return false;
    for (int i=0;i<16;i++) {
        char b1 = s[2*i];
        char b2 = s[2*i+1];
        auto toNib = [](char c)->int{
            if (c>='0'&&c<='9') return c-'0';
            if (c>='a'&&c<='f') return 10 + (c - 'a');
            if (c>='A'&&c<='F') return 10 + (c - 'A');
            return -1;
        };
        int n1 = toNib(b1), n2 = toNib(b2);
        if (n1<0||n2<0) return false;
        out[i] = (uint8_t)((n1<<4)|n2);
    }
    return true;
}

void SmartTileNode::checkCoordinatorConnection() {
    // Check if we've lost connection with coordinator
    uint32_t timeSinceResponse = millis() - lastCoordinatorResponse;
    
    // If we haven't heard from coordinator in a while AND we've sent multiple telemetry messages
    if (timeSinceResponse > COORDINATOR_TIMEOUT_MS && telemetrySentCount >= TELEMETRY_THRESHOLD) {
        logMessage("WARN", String("No coordinator response for ") + String(timeSinceResponse/1000) + 
                   "s after " + String(telemetrySentCount) + " telemetry attempts");
        logMessage("INFO", "Attempting to re-pair with coordinator...");
        
        // Clear stored configuration to force re-pairing
        config.remove(ConfigKeys::NODE_ID);
        config.remove(ConfigKeys::LIGHT_ID);
        
        // Reset counters
        lastCoordinatorResponse = millis();
        telemetrySentCount = 0;
        
        // Switch to pairing mode
        currentState = NodeState::PAIRING;
        startPairing();
        
        logMessage("INFO", "Switched to pairing mode - waiting for coordinator");
    }
}

void SmartTileNode::logMessage(const String& level, const String& message) {
    uint32_t timestamp = millis();
    Serial.printf("[%lu] [%s] %s\n", timestamp, level.c_str(), message.c_str());
}

// Global instance
SmartTileNode node;

void setup() {
    // Give USB-Serial time to enumerate
    delay(2000);
    Serial.begin(115200);
    delay(500);
    
    Serial.println("=== ESP32-C3 BOOT ===");
    Serial.println("Setup starting...");
    
    if (!node.begin()) {
        Serial.println("Failed to initialize node");
        while (1) {
            delay(1000);
        }
    }
    
    Serial.println("Setup complete!");
}

void loop() {
    node.loop();
}

// Static trampolines for ESP-NOW v2 callbacks
static void espnowRecv(const esp_now_recv_info_t* recv_info, const uint8_t* data, int len) {
    if (gNodeInstance && recv_info && recv_info->src_addr) {
        gNodeInstance->onDataRecv(recv_info->src_addr, data, len);
    }
}
static void espnowSent(const uint8_t* mac, esp_now_send_status_t status) {
    if (gNodeInstance) gNodeInstance->onDataSent(mac, status);
}

#endif // STANDALONE_TEST
