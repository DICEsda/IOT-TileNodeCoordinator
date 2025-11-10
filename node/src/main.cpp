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
// Temperature sensor
#include "sensor/TMP177Sensor.h"

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
    
    // RGBW LED strip
    LedController leds{4}; // PRD: 4 pixels per node
    uint8_t curR = 0, curG = 0, curB = 0, curW = 0;
    bool statusOverrideActive = false;
    uint32_t statusOverrideUntilMs = 0;
    
    // Link/activity tracking
    uint32_t lastLinkActivityMs = 0; // updated on any RX, or successful unicast TX
    
    // Power management
    uint32_t lastRxWindow;
    uint16_t rxWindowMs;
    uint16_t rxPeriodMs;
    uint32_t lastTelemetry;
    uint16_t telemetryInterval;
    
    // Button
    ButtonInput button;
    uint32_t pairingStartTime;
    bool inPairingMode;
    uint32_t lastJoinSentMs;
    
    // Temperature sensor
    TMP177Sensor tempSensor;
    bool tempSensorAvailable;
    
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
    void sendJoinRequestNow();
    bool sendMessage(const EspNowMessage& message, const uint8_t* destMac = nullptr);
    void processReceivedMessage(const String& json);
    bool ensureEncryptedPeer(const uint8_t mac[6], const String& lmkHex);
    static bool parseHex16(const String& hex, uint8_t out[16]);
    
    // LED control
    void applyColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint16_t fadeMs);
    
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
    
    // Configuration
    void loadConfiguration();
    void saveConfiguration();
    
    // Utility
    String generateCmdId();
    String getMacAddress();
    void logMessage(const String& level, const String& message);

    // Link helpers
    bool isBroadcast(const uint8_t* mac) const;
    bool isLinkAlive() const;
};

SmartTileNode::SmartTileNode() 
    : currentState(NodeState::PAIRING)
    , config("node")
    , espNowInitialized(false)
    , lastRxWindow(0)
    , rxWindowMs(20)
    , rxPeriodMs(100)
    , lastTelemetry(0)
    , telemetryInterval(30)
    , pairingStartTime(0)
    , inPairingMode(false)
    , lastJoinSentMs(0)
    , firmwareVersion("c3-1.0.0")
    , lastCommandTime(0)
    , tempSensorAvailable(false) {
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
    
    // Initialize temperature sensor
    tempSensorAvailable = tempSensor.begin(Pins::I2C_SDA, Pins::I2C_SCL);
    if (tempSensorAvailable) {
        logMessage("INFO", "TMP177 temperature sensor initialized");
    } else {
        logMessage("WARN", "TMP177 sensor not available");
    }
    
    // Initialize ESP-NOW
    if (!initEspNow()) {
        logMessage("ERROR", "Failed to initialize ESP-NOW");
        return false;
    }
    
    // Determine initial state
    if (config.exists(ConfigKeys::NODE_ID) && config.exists(ConfigKeys::LIGHT_ID)) {
        currentState = NodeState::OPERATIONAL;
        leds.setStatus(LedController::StatusMode::Idle);
        Serial.println("Node: OPERATIONAL (awaiting link)");
    } else {
        currentState = NodeState::PAIRING;
        // Don't auto-start pairing - wait for button press
        leds.setStatus(LedController::StatusMode::Idle);
        Serial.println("Node: unpaired. Hold button for 2s to enter pairing mode.");
    }
    
    // Force periodic telemetry every 5 seconds per requirement
    telemetryInterval = 5;
    
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
            break;
        // Derate state removed; coordinator handles thermal clamping
        case NodeState::UPDATE:
            handleUpdate();
            break;
        case NodeState::REBOOT:
            handleReboot();
            break;
    }
    
    // While operational and not overriding/pairing, reflect link health on LEDs
    if (currentState == NodeState::OPERATIONAL && !inPairingMode && !statusOverrideActive) {
        leds.setStatus(isLinkAlive() ? LedController::StatusMode::Connected
                                     : LedController::StatusMode::Idle);
    }

    // Send telemetry periodically
    if (millis() - lastTelemetry > telemetryInterval * 1000) {
        sendTelemetry();
        lastTelemetry = millis();
    }
    
    // Power management - avoid light sleep if we are animating to keep FPS high
    bool animating = leds.isAnimating();
    if (currentState == NodeState::OPERATIONAL && !inPairingMode && !animating) {
        if (!isRxWindowActive()) {
            enterLightSleep();
        }
    }

    // Smaller delay when animating for smoother visuals
    delay(animating ? 1 : 10);
}

void SmartTileNode::sendJoinRequestNow() {
    JoinRequestMessage joinReq;
    joinReq.mac = getMacAddress();
    joinReq.fw = firmwareVersion;
        joinReq.caps.rgbw = true;
        joinReq.caps.led_count = leds.numPixels();
        joinReq.caps.temp_i2c = false; // No sensor in standalone mode
        joinReq.caps.deep_sleep = true;
        joinReq.caps.button = true;
    joinReq.token = String(esp_random(), HEX);

    String payload = joinReq.toJson();
    if (payload.length() > 250) {
        logMessage("ERROR", "JOIN_REQUEST too large!");
        return;
    }
    
    // Quick green flash to indicate sending
    leds.setStatus(LedController::StatusMode::None);
    leds.setBrightness(120);
    leds.setColor(0, 255, 0, 0); // Green
    leds.update();
    delay(50); // Brief flash
    
    // Always broadcast for pairing
    uint8_t targetMac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
    esp_now_send(targetMac, (uint8_t*)payload.c_str(), payload.length());
    
    // Restore pairing animation
    leds.setStatus(LedController::StatusMode::Pairing);
}

void SmartTileNode::handlePairing() {
    if (!inPairingMode) {
        // Wait for button press to start pairing
        return;
    }
    
    // Check if pairing window expired
    uint32_t now = millis();
    if (now - pairingStartTime > (uint32_t)config.getInt(ConfigKeys::PAIRING_WINDOW_S, 120) * 1000U) {
        stopPairing();
        return;
    }
    
    // Optimized adaptive join interval with exponential backoff
    uint32_t elapsed = now - pairingStartTime;
    uint32_t baseInterval;
    
    // Aggressive in first 5 seconds, then slower with exponential backoff
    if (elapsed < 5000U) {
        baseInterval = 600U;  // Very fast initially
    } else if (elapsed < 15000U) {
        baseInterval = 1200U; // Medium frequency
    } else if (elapsed < 30000U) {
        baseInterval = 3000U; // Slower
    } else {
        baseInterval = 6000U; // Very slow after 30s
    }
    
    // Add jitter to avoid collision with other nodes
    uint32_t jitter = (uint32_t)(esp_random() % 400U);
    
    if (now - lastJoinSentMs >= baseInterval + jitter) {
        JoinRequestMessage joinReq;
        joinReq.mac = getMacAddress();
        joinReq.fw = firmwareVersion;
        joinReq.caps.rgbw = true;
        joinReq.caps.led_count = leds.numPixels();
        joinReq.caps.temp_i2c = tempSensorAvailable;
        joinReq.caps.deep_sleep = true;
        joinReq.caps.button = true;
        joinReq.token = String(esp_random(), HEX);

        String payload = joinReq.toJson();
        
        // ✓ Checklist: Message Size check
        if (payload.length() > 250) {
            logMessage("ERROR", "JOIN_REQUEST too large!");
            return;
        }
        
        // Quick green flash to indicate sending
        leds.setStatus(LedController::StatusMode::None);
        leds.setBrightness(120);
        leds.setColor(0, 255, 0, 0); // Green
        leds.update();
        delay(50); // Brief flash
        
        // Always broadcast JOIN_REQUEST for maximum reliability
        uint8_t targetMac[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
        esp_err_t result = esp_now_send(targetMac, (uint8_t*)payload.c_str(), payload.length());
        
        // Only retry once on failure to avoid spam
        if (result != ESP_OK) {
            delay(10); // Small delay before retry
            result = esp_now_send(targetMac, (uint8_t*)payload.c_str(), payload.length());
        }
        
        // Restore pairing animation
        leds.setStatus(LedController::StatusMode::Pairing);
        
        lastJoinSentMs = now;
        
        // Only log every 3rd attempt to reduce overhead
        static uint8_t logCounter = 0;
        if (++logCounter % 3 == 0) {
            logMessage("DEBUG", String("JOIN_REQUEST sent (") + String(payload.length()) + 
                       " bytes), attempts=" + String(logCounter));
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

    // Pre-set channel BEFORE esp_now_init for maximum compatibility
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);

    // Optionally set a conservative ESPNOW PHY rate (if available)
    #ifdef CONFIG_IDF_TARGET_ESP32C3
    #ifdef WIFI_PHY_RATE_1M_L
    esp_wifi_config_espnow_rate(WIFI_IF_STA, WIFI_PHY_RATE_1M_L);
    #endif
    #endif
    
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

    // Set PMK to ensure interoperability (even with unencrypted peers)
    static const uint8_t PMK[16] = {'S','M','A','R','T','T','I','L','E','_','P','M','K','_','0','1'};
    esp_err_t pmkRes = esp_now_set_pmk(PMK);
    if (pmkRes == ESP_OK) {
        logMessage("INFO", "  PMK set");
    } else {
        logMessage("WARN", String("  PMK set failed: ") + String((int)pmkRes));
    }
    
    // Set WiFi channel to 1 (must match coordinator)
    logMessage("INFO", "[4/9] Setting WiFi channel to 1...");
    esp_wifi_set_promiscuous(true);
    esp_err_t chRes = esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
    esp_wifi_set_promiscuous(false);
    
    uint8_t primary = 0; 
    wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
    esp_wifi_get_channel(&primary, &second);
    
    if (primary != 1) {
        logMessage("ERROR", String("Failed to set channel! Now on: ") + String((int)primary));
        return false;
    }
    logMessage("INFO", String("  Channel: ") + String((int)primary));
    
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
    peerInfo.channel = 1;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;
    
    esp_err_t bres = esp_now_add_peer(&peerInfo);
    if (bres == ESP_OK || bres == ESP_ERR_ESPNOW_EXIST) {
        logMessage("INFO", "  Broadcast peer added (FF:FF:FF:FF:FF:FF)");
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
    // Quick filter: drop obviously non-JSON frames or invalid data
    if (!data || len <= 0 || len > 250) return;
    if (((const char*)data)[0] != '{') {
        return; // not our JSON protocol
    }

    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    // Debug: log received length and first few chars
    Serial.printf("RX %d bytes from %s: ", len, macStr);
    // Show first 40 chars to identify message type
    for (int i = 0; i < len && i < 40; i++) {
        Serial.print((char)data[i]);
    }
    if (len > 40) Serial.print("...");
    Serial.println();
    
    // Create String from data, stopping at first null byte or end of valid JSON
    // Find actual end of JSON (last '}' character)
    int actualLen = len;
    for (int i = len - 1; i >= 0; i--) {
        if (data[i] == '}') {
            actualLen = i + 1;
            break;
        }
        // Also stop if we find non-whitespace, non-null characters after '}'  
        if (data[i] != 0 && data[i] != ' ' && data[i] != '\r' && data[i] != '\n' && data[i] != '\t') {
            break;
        }
    }
    
    String message;
    message.reserve(actualLen + 1);
    for (int i = 0; i < actualLen; i++) {
        message += (char)data[i];
    }

    // Fast path: check for pairing ping first (avoid full string construction)
    // BUT: don't return early if it's a join_accept (critical message)
    if (inPairingMode && message.indexOf("pairing_ping") >= 0) {
        uint32_t now = millis();
        // Rate limit responses to avoid flooding
        if (now - lastJoinSentMs > 600U) {
            Serial.println("RX pairing beacon -> responding");
            sendJoinRequestNow();
            lastJoinSentMs = now;
        }
        return; // don't process further
    }
    
    // Check if this is a join_accept - always process these
    bool isJoinAccept = (message.indexOf("join_accept") >= 0);

    // Optimize: check coordinator MAC filtering with early exit
    bool coordKnown = false;
    for (int i = 0; i < 6; i++) {
        if (coordinatorMac[i] != 0) {
            coordKnown = true;
            break;
        }
    }
    
    if (coordKnown) {
        bool fromCoordinator = true;
        for (int i = 0; i < 6; i++) {
            if (coordinatorMac[i] != mac[i]) {
                fromCoordinator = false;
                break;
            }
        }
        
        // Drop non-coordinator traffic unless it's a join_accept
        if (!fromCoordinator && !isJoinAccept) {
            return;
        }
    }
    
    // Remember coordinator MAC upon join_accept
    if (isJoinAccept) {
        memcpy(coordinatorMac, mac, 6);
        Serial.printf("Coordinator MAC: %s\n", macStr);
        Serial.printf("RX JOIN_ACCEPT from coordinator\n");
    }

    // mark RX window and link activity (only for real messages, not pings)
    lastRxWindow = millis();
    lastLinkActivityMs = lastRxWindow;

    processReceivedMessage(message);
}

void SmartTileNode::onDataSent(const uint8_t* mac, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        // Consider link alive only for unicast (non-broadcast) sends
        if (mac && !isBroadcast(mac)) {
            lastLinkActivityMs = millis();
        }
    }
    // After a send completes, if not in pairing/override, reflect link health
    if (currentState == NodeState::OPERATIONAL && !inPairingMode && !statusOverrideActive) {
        leds.setStatus(isLinkAlive() ? LedController::StatusMode::Connected
                                     : LedController::StatusMode::Idle);
    }
}

void SmartTileNode::processReceivedMessage(const String& json) {
    // Fast check: ignore common non-processing messages
    const char* jstr = json.c_str();
    if (strstr(jstr, "pairing_ping") || strstr(jstr, "\"ping\"") || strstr(jstr, "coordinator_alive")) {
        return;
    }
    
    // Debug: log what we're processing (full message)
    Serial.printf("Processing message (%d chars): %s\n", json.length(), json.c_str());
    
    EspNowMessage* message = MessageFactory::createMessage(json);
    if (!message) {
        Serial.println("ERROR: Failed to parse message!");
        Serial.printf("  Message was: %s\n", json.c_str());
        Serial.printf("  Length: %d bytes\n", json.length());
        return;
    }
    
    Serial.printf("Message type: %d\n", (int)message->type);
    
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

            // Configure peer (unencrypted in ESPNowW path; lmk may be empty)
            ensureEncryptedPeer(coordinatorMac, accept->lmk);
            
            saveConfiguration();
            
            // Send status and switch to connected operational mode
            currentState = NodeState::OPERATIONAL;
            stopPairing();
            // Brief green flash to confirm pairing success (full brightness)
            leds.setStatus(LedController::StatusMode::None);
            leds.setBrightness(255);
            leds.setColor(0, 255, 0, 0);
            delay(120);
            // Resume Connected animation (solid green)
            leds.setStatus(LedController::StatusMode::Connected);
            
            // Immediately send telemetry after pairing
            lastTelemetry = millis(); // Reset timer
            sendTelemetry();
            
            Serial.println("Paired successfully!");
            Serial.printf("Node ID: %s, Light ID: %s\n", nodeId.c_str(), lightId.c_str());
            break;
        }
        case MessageType::SET_LIGHT: {
            SetLightMessage* setLight = static_cast<SetLightMessage*>(message);
            if (setLight->light_id == lightId) {
                // Respect override_status: temporarily disable status patterns
                if (setLight->override_status) {
                    statusOverrideActive = true;
                    statusOverrideUntilMs = millis() + setLight->ttl_ms;
                    leds.setStatus(LedController::StatusMode::None);
                }

                uint8_t r = setLight->r, g = setLight->g, b = setLight->b, w = setLight->w;
                if (r == 0 && g == 0 && b == 0 && w == 0) {
                    // fallback: map value to white channel
                    w = setLight->value;
                }
                applyColor(r, g, b, w, setLight->fade_ms);
                lastCmdId = setLight->cmd_id;
                lastCommandTime = millis();
                
                // Send acknowledgment
                AckMessage ack;
                ack.cmd_id = setLight->cmd_id;
                sendMessage(ack);
                
            }
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

bool SmartTileNode::isBroadcast(const uint8_t* mac) const {
    if (!mac) return true;
    for (int i = 0; i < 6; ++i) if (mac[i] != 0xFF) return false;
    return true;
}

bool SmartTileNode::isLinkAlive() const {
    if (lastLinkActivityMs == 0) return false;
    return (millis() - lastLinkActivityMs) < 10000U; // 10s window
}

void SmartTileNode::handleButton() {
    button.loop();
    // Clear status override when TTL expires
    if (statusOverrideActive && millis() > statusOverrideUntilMs) {
        statusOverrideActive = false;
        leds.setStatus(currentState == NodeState::PAIRING ? LedController::StatusMode::Pairing
                                                          : LedController::StatusMode::Connected);
    }
}

void SmartTileNode::startPairing() {
    inPairingMode = true;
    pairingStartTime = millis();
    leds.setStatus(LedController::StatusMode::Pairing);
    Serial.println("Pairing mode: active");
}

void SmartTileNode::stopPairing() {
    inPairingMode = false;
    leds.setStatus(currentState == NodeState::OPERATIONAL ? LedController::StatusMode::Connected
                                                          : LedController::StatusMode::Idle);
    Serial.println("Pairing mode: stopped");
}

void SmartTileNode::sendTelemetry() {
    // Skip telemetry if not paired yet
    if (nodeId.length() == 0 || lightId.length() == 0) {
        Serial.println("Telemetry: skipped (not paired)");
        return;
    }
    
    Serial.printf("Sending telemetry to coordinator (nodeId=%s)\n", nodeId.c_str());
    
    NodeStatusMessage status;
    status.node_id = nodeId;
    status.light_id = lightId;
    status.avg_r = curR; status.avg_g = curG; status.avg_b = curB; status.avg_w = curW;
    status.status_mode = (currentState == NodeState::PAIRING) ? "pairing"
                        : (statusOverrideActive ? "override" : "operational");
    status.fw = firmwareVersion;
    status.vbat_mv = readBatteryVoltage();
    
    // Read temperature sensor
    if (tempSensorAvailable) {
        status.temperature = tempSensor.readTemperature();
        Serial.printf("  Temperature: %.2f°C\n", status.temperature);
    }
    
    // Read button state (from ButtonInput class - check if pressed)
    status.button_pressed = digitalRead(Pins::BUTTON) == LOW; // Assuming active-low button
    Serial.printf("  Button: %s\n", status.button_pressed ? "PRESSED" : "Released");
    Serial.printf("  RGBW: (%d,%d,%d,%d)\n", curR, curG, curB, curW);
    Serial.printf("  Battery: %d mV\n", status.vbat_mv);
    
    // Don't flash LED during telemetry - keep it solid green
    // Just send the message
    bool sent = sendMessage(status);
    Serial.printf("Telemetry send: %s\n", sent ? "OK" : "FAILED");
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

void SmartTileNode::logMessage(const String& level, const String& message) {
    Serial.printf("[%s] %s\n", level.c_str(), message.c_str());
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
