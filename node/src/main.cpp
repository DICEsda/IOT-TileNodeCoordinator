// Standalone test mode for SK6812B control without coordinator
#ifdef STANDALONE_TEST

#include <Arduino.h>
#include "led/LedController.h"
#include "input/ButtonInput.h"
#include "config/PinConfig.h"
#include "utils/OtaUpdater.h"

static constexpr uint16_t LED_NUM_PIXELS = 8; // adjust to your strip

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
    uint32_t pairingTimeoutMs = 30000; // 30 seconds

    void setMode(uint8_t m) {
        mode = m % 4;
        switch (mode) {
            case 0: // Off
                led->setBrightness(0);
                led->setColor(0,0,0);
                break;
            case 1: // Warm white low
                led->setBrightness(32);
                led->setColor(255, 180, 120, 300);
                break;
            case 2: // Warm white high
                led->setBrightness(128);
                led->setColor(255, 200, 150, 300);
                break;
            case 3: // Rainbow demo
                led->setBrightness(96);
                // immediate color will be set by animation loop
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
            Serial.println("Exit pairing mode");
        } else {
            state = UiState::PAIRING;
            pairingStartMs = millis();
            lastAnimMs = 0;
            Serial.println("Enter pairing mode (hold for 2s)");
        }
    }

    void pairingAnimation() {
        // Blink cyan smoothly at ~1Hz
        uint32_t now = millis();
        float t = (now % 1000) / 1000.0f; // 0..1
        // triangle wave 0..1..0
        float tri = t < 0.5f ? (t * 2.0f) : (2.0f - t * 2.0f);
        uint8_t b = 40 + (uint8_t)(tri * 120);
        led->setBrightness(b);
        led->setColor(0, 255, 255); // cyan
    }

    void pairingFailedExit() {
        Serial.println("Pairing timeout: showing failure flash");
        // Red-ish flash sequence (2 pulses)
        for (int i = 0; i < 2; ++i) {
            led->setBrightness(150);
            led->setColor(255, 50, 20); // warm red-orange
            delay(180);
            led->setBrightness(0);
            led->setColor(0, 0, 0);
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
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_sleep.h>
#include <driver/ledc.h>
#include <driver/gpio.h>
#include <esp_timer.h>
#include <esp_random.h>

#include "EspNowMessage.h"
#include "ConfigManager.h"

// Pin definitions (adjust based on your hardware)
#define PWM_PIN 2
#define BUTTON_PIN 0
#define LED_PIN 1

// Node state machine
enum class NodeState {
    BOOT,
    PAIRING,
    OPERATIONAL,
    UPDATE,
    REBOOT
};

class SmartTileNode {
private:
    NodeState currentState;
    ConfigManager config;
    
    // ESP-NOW
    uint8_t coordinatorMac[6];
    bool espNowInitialized;
    
    // PWM
    int pwmChannel;
    int pwmResolution;
    int pwmFrequency;
    uint8_t currentDuty;
    uint8_t targetDuty;
    uint32_t fadeStartTime;
    uint16_t fadeDuration;
    bool fading;
    
    // Temperature removed; coordinator owns sensors and derating
    
    // Power management
    uint32_t lastRxWindow;
    uint16_t rxWindowMs;
    uint16_t rxPeriodMs;
    uint32_t lastTelemetry;
    uint16_t telemetryInterval;
    
    // Button
    uint32_t lastButtonPress;
    bool buttonPressed;
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
    void handleBoot();
    void handlePairing();
    void handleOperational();
    // Derate handling removed; coordinator clamps brightness
    void handleUpdate();
    void handleReboot();
    
    // ESP-NOW
    bool initEspNow();
    void onDataRecv(const uint8_t* mac, const uint8_t* data, int len);
    void onDataSent(const uint8_t* mac, esp_now_send_status_t status);
    bool sendMessage(const EspNowMessage& message);
    void processReceivedMessage(const String& json);
    
    // PWM control
    bool initPWM();
    void setDuty(uint8_t value, uint16_t fadeMs = 0);
    void updateFade();
    // Thermal derating removed
    
    // Temperature sensing removed
    
    // Power management
    void enterLightSleep();
    void scheduleRxWindow();
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
};

SmartTileNode::SmartTileNode() 
    : currentState(NodeState::BOOT)
    , config("node")
    , espNowInitialized(false)
    , pwmChannel(0)
    , pwmResolution(12)
    , pwmFrequency(1000)
    , currentDuty(0)
    , targetDuty(0)
    , fadeStartTime(0)
    , fadeDuration(0)
    , fading(false)
    , lastRxWindow(0)
    , rxWindowMs(20)
    , rxPeriodMs(100)
    , lastTelemetry(0)
    , telemetryInterval(30)
    , lastButtonPress(0)
    , buttonPressed(false)
    , pairingStartTime(0)
    , inPairingMode(false)
    , firmwareVersion("c3-1.0.0")
    , lastCommandTime(0) {
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
    
    // Initialize hardware
    if (!initPWM()) {
        logMessage("ERROR", "Failed to initialize PWM");
        return false;
    }
    
    // Initialize button
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize ESP-NOW
    if (!initEspNow()) {
        logMessage("ERROR", "Failed to initialize ESP-NOW");
        return false;
    }
    
    // Determine initial state
    if (config.exists(ConfigKeys::NODE_ID) && config.exists(ConfigKeys::LIGHT_ID)) {
        currentState = NodeState::OPERATIONAL;
        logMessage("INFO", "Starting in OPERATIONAL mode");
    } else {
        currentState = NodeState::PAIRING;
        logMessage("INFO", "Starting in PAIRING mode");
    }
    
    return true;
}

void SmartTileNode::loop() {
    handleButton();
    
    switch (currentState) {
        case NodeState::BOOT:
            handleBoot();
            break;
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
    
    // Update PWM fade
    updateFade();
    
    // Send telemetry periodically
    if (millis() - lastTelemetry > telemetryInterval * 1000) {
        sendTelemetry();
        lastTelemetry = millis();
    }
    
    // Power management - enter light sleep if not in critical operations
    if (currentState == NodeState::OPERATIONAL && !fading && !inPairingMode) {
        if (!isRxWindowActive()) {
            enterLightSleep();
        }
    }
    
    delay(10); // Small delay to prevent watchdog issues
}

void SmartTileNode::handleBoot() {
    // Boot sequence completed, move to appropriate state
    if (config.exists(ConfigKeys::NODE_ID)) {
        currentState = NodeState::OPERATIONAL;
    } else {
        currentState = NodeState::PAIRING;
    }
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
    
    // Send join request periodically during pairing
    static uint32_t lastJoinRequest = 0;
    if (millis() - lastJoinRequest > 5000) { // Every 5 seconds
        JoinRequestMessage joinReq;
        joinReq.mac = getMacAddress();
        joinReq.caps.pwm = true;
        joinReq.caps.temp_spi = true;
        joinReq.fw = firmwareVersion;
        joinReq.token = String(esp_random(), HEX);
        
        // Send broadcast join request
        esp_now_send(nullptr, (uint8_t*)joinReq.toJson().c_str(), joinReq.toJson().length());
        lastJoinRequest = millis();
        
        logMessage("DEBUG", "Sent join request");
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
    WiFi.mode(WIFI_STA);
    
    if (esp_now_init() != ESP_OK) {
        return false;
    }
    
    // NOTE: Callbacks should be bound to the object instance; omitted here as coordinator is not used in standalone
    
    espNowInitialized = true;
    return true;
}

void SmartTileNode::onDataRecv(const uint8_t* mac, const uint8_t* data, int len) {
    String message = String((char*)data, len);
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
            config.setInt(ConfigKeys::PWM_FREQ_HZ, accept->cfg.pwm_freq);
            config.setInt(ConfigKeys::RX_WINDOW_MS, accept->cfg.rx_window_ms);
            config.setInt(ConfigKeys::RX_PERIOD_MS, accept->cfg.rx_period_ms);
            
            saveConfiguration();
            
            // Send status and switch to operational
            sendTelemetry();
            currentState = NodeState::OPERATIONAL;
            stopPairing();
            
            logMessage("INFO", "Successfully paired with coordinator");
            break;
        }
        case MessageType::SET_LIGHT: {
            SetLightMessage* setLight = static_cast<SetLightMessage*>(message);
            if (setLight->light_id == lightId) {
                targetDuty = setLight->value;
                setDuty(targetDuty, setLight->fade_ms);
                lastCmdId = setLight->cmd_id;
                lastCommandTime = millis();
                
                // Send acknowledgment
                AckMessage ack;
                ack.cmd_id = setLight->cmd_id;
                sendMessage(ack);
                
                logMessage("INFO", "Received set_light command: " + String(targetDuty));
            }
            break;
        }
        default:
            logMessage("WARN", "Unknown message type received");
            break;
    }
    
    delete message;
}

bool SmartTileNode::initPWM() {
    ledcSetup(pwmChannel, pwmFrequency, pwmResolution);
    ledcAttachPin(PWM_PIN, pwmChannel);
    ledcWrite(pwmChannel, 0);
    
    return true;
}

void SmartTileNode::setDuty(uint8_t value, uint16_t fadeMs) {
    if (fadeMs == 0) {
        currentDuty = value;
        ledcWrite(pwmChannel, value << (pwmResolution - 8));
        fading = false;
    } else {
        targetDuty = value;
        fadeStartTime = millis();
        fadeDuration = fadeMs;
        fading = true;
    }
}

void SmartTileNode::updateFade() {
    if (!fading) return;
    
    uint32_t elapsed = millis() - fadeStartTime;
    if (elapsed >= fadeDuration) {
        currentDuty = targetDuty;
        ledcWrite(pwmChannel, currentDuty << (pwmResolution - 8));
        fading = false;
    } else {
        float progress = (float)elapsed / fadeDuration;
        uint8_t newDuty = currentDuty + (targetDuty - currentDuty) * progress;
        ledcWrite(pwmChannel, newDuty << (pwmResolution - 8));
    }
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

void SmartTileNode::scheduleRxWindow() {
    lastRxWindow = millis();
}

bool SmartTileNode::isRxWindowActive() {
    return (millis() - lastRxWindow) < rxWindowMs;
}

void SmartTileNode::handleButton() {
    bool currentButtonState = !digitalRead(BUTTON_PIN);
    
    if (currentButtonState && !buttonPressed) {
        buttonPressed = true;
        lastButtonPress = millis();
        
        if (currentState == NodeState::OPERATIONAL) {
            // Button press in operational mode - could be used for manual control
            logMessage("INFO", "Button pressed in operational mode");
        } else if (currentState == NodeState::PAIRING) {
            startPairing();
        }
    } else if (!currentButtonState && buttonPressed) {
        buttonPressed = false;
    }
}

void SmartTileNode::startPairing() {
    inPairingMode = true;
    pairingStartTime = millis();
    digitalWrite(LED_PIN, HIGH);
    logMessage("INFO", "Pairing mode started");
}

void SmartTileNode::stopPairing() {
    inPairingMode = false;
    digitalWrite(LED_PIN, LOW);
    logMessage("INFO", "Pairing mode stopped");
}

void SmartTileNode::sendTelemetry() {
    NodeStatusMessage status;
    status.node_id = nodeId;
    status.light_id = lightId;
    status.temp_c = 0.0;
    status.duty = currentDuty;
    status.fw = firmwareVersion;
    status.vbat_mv = readBatteryVoltage();
    
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
    
    pwmFrequency = config.getInt(ConfigKeys::PWM_FREQ_HZ, Defaults::PWM_FREQ_HZ);
    pwmResolution = config.getInt(ConfigKeys::PWM_RESOLUTION_BITS, Defaults::PWM_RESOLUTION_BITS);
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

void SmartTileNode::logMessage(const String& level, const String& message) {
    uint32_t timestamp = millis();
    Serial.printf("[%lu] [%s] %s\n", timestamp, level.c_str(), message.c_str());
}

// Global instance
SmartTileNode node;

void setup() {
    if (!node.begin()) {
        Serial.println("Failed to initialize node");
        while (1) {
            delay(1000);
        }
    }
}

void loop() {
    node.loop();
}
