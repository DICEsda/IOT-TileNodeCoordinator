#include "Mqtt.h"
#include "../utils/Logger.h"
#include "../../shared/src/ConfigManager.h"
#include <ArduinoJson.h>

// Static instance pointer for callback
static Mqtt* mqttInstance = nullptr;

Mqtt::Mqtt() 
    : mqttClient(wifiClient)
    , brokerPort(1883)
    , commandCallback(nullptr) {
    mqttInstance = this;
}

Mqtt::~Mqtt() {
    mqttClient.disconnect();
    WiFi.disconnect();
}

bool Mqtt::begin() {
    Logger::info("Initializing MQTT client...");
    
    // Load configuration from NVS
    ConfigManager config("mqtt");
    if (!config.begin()) {
        Logger::warn("Failed to load MQTT config, using defaults");
    }
    
    wifiSsid = config.getString("wifi_ssid", "YourWiFiSSID");
    wifiPassword = config.getString("wifi_pass", "YourPassword");
    brokerHost = config.getString("broker_host", "192.168.1.100");
    brokerPort = config.getInt("broker_port", 1883);
    brokerUsername = config.getString("broker_user", "user1");
    brokerPassword = config.getString("broker_pass", "user1");
    
    config.end();
    
    // Connect to WiFi
    if (!connectWifi()) {
        Logger::error("Failed to connect to WiFi");
        return false;
    }
    
    // Setup MQTT client
    mqttClient.setServer(brokerHost.c_str(), brokerPort);
    mqttClient.setCallback(handleMqttMessage);
    
    // Connect to MQTT broker
    if (!connectMqtt()) {
        Logger::warn("Failed initial MQTT connection (will retry)");
        // Non-fatal - will retry in loop()
    }
    
    Logger::info("MQTT initialization complete");
    return true;
}

void Mqtt::loop() {
    // Reconnect WiFi if needed
    if (WiFi.status() != WL_CONNECTED) {
        Logger::warn("WiFi disconnected, reconnecting...");
        connectWifi();
        return;
    }
    
    // Reconnect MQTT if needed
    if (!mqttClient.connected()) {
        static uint32_t lastReconnect = 0;
        uint32_t now = millis();
        if (now - lastReconnect > 5000) {
            lastReconnect = now;
            connectMqtt();
        }
    }
    
    mqttClient.loop();
}

bool Mqtt::isConnected() const {
    return mqttClient.connected();
}

// PRD-compliant: site/{siteId}/node/{nodeId}/telemetry
void Mqtt::publishLightState(const String& lightId, uint8_t brightness) {
    if (!mqttClient.connected()) return;
    
    StaticJsonDocument<256> doc;
    doc["ts"] = millis() / 1000;
    doc["light_id"] = lightId;
    doc["brightness"] = brightness;
    
    String payload;
    serializeJson(doc, payload);
    
    // Assuming lightId maps to nodeId (simplified)
    String topic = "site/site001/node/" + lightId + "/telemetry";
    mqttClient.publish(topic.c_str(), payload.c_str());
}

// PRD-compliant: site/{siteId}/node/{nodeId}/telemetry
void Mqtt::publishThermalEvent(const String& nodeId, const NodeThermalData& data) {
    if (!mqttClient.connected()) return;
    
    StaticJsonDocument<512> doc;
    doc["ts"] = millis() / 1000;
    doc["node_id"] = nodeId;
    doc["temp_c"] = data.temperature;
    doc["is_derated"] = data.isDerated;
    doc["deration_level"] = data.derationLevel;
    
    String payload;
    serializeJson(doc, payload);
    
    String topic = "site/site001/node/" + nodeId + "/telemetry";
    mqttClient.publish(topic.c_str(), payload.c_str());
    
    Logger::info("Published thermal event for node %s", nodeId.c_str());
}

// PRD-compliant: site/{siteId}/coord/{coordId}/mmwave
void Mqtt::publishMmWaveEvent(const MmWaveEvent& event) {
    if (!mqttClient.connected()) return;
    
    StaticJsonDocument<512> doc;
    doc["ts"] = event.timestampMs / 1000;
    
    JsonArray events = doc.createNestedArray("events");
    JsonObject evt = events.createNestedObject();
    evt["zone"] = event.sensorId.toInt();
    evt["presence"] = event.presence;
    evt["confidence"] = 0.85;  // Placeholder
    
    String payload;
    serializeJson(doc, payload);
    
    String coordId = WiFi.macAddress();
    String topic = "site/site001/coord/" + coordId + "/mmwave";
    mqttClient.publish(topic.c_str(), payload.c_str());
    
    Logger::info("Published mmWave event");
}

// PRD-compliant: site/{siteId}/node/{nodeId}/telemetry
void Mqtt::publishNodeStatus(const String& nodeId, const NodeInfo& status) {
    if (!mqttClient.connected()) return;
    
    StaticJsonDocument<1024> doc;
    doc["ts"] = millis() / 1000;
    doc["node_id"] = nodeId;
    doc["light_id"] = status.lightId;
    doc["avg_r"] = 0;  // Would come from node telemetry
    doc["avg_g"] = 0;
    doc["avg_b"] = 0;
    doc["avg_w"] = status.lastDuty;
    doc["status_mode"] = "operational";
    doc["temp_c"] = status.temperature;
    doc["vbat_mv"] = 3700;  // Placeholder
    doc["fw"] = "c3-1.0.0";
    
    String payload;
    serializeJson(doc, payload);
    
    String topic = "site/site001/node/" + nodeId + "/telemetry";
    mqttClient.publish(topic.c_str(), payload.c_str());
}

void Mqtt::setWifiCredentials(const char* ssid, const char* password) {
    wifiSsid = ssid;
    wifiPassword = password;
}

void Mqtt::setBrokerConfig(const char* host, uint16_t port, const char* username, const char* password) {
    brokerHost = host;
    brokerPort = port;
    brokerUsername = username;
    brokerPassword = password;
}

void Mqtt::setCommandCallback(void (*callback)(const String& topic, const String& payload)) {
    commandCallback = callback;
}

bool Mqtt::connectWifi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }
    
    Logger::info("Connecting to WiFi: %s", wifiSsid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid.c_str(), wifiPassword.c_str());
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Logger::info("WiFi connected! IP: %s", WiFi.localIP().toString().c_str());
        return true;
    }
    
    Logger::error("WiFi connection failed");
    return false;
}

bool Mqtt::connectMqtt() {
    if (mqttClient.connected()) {
        return true;
    }
    
    Logger::info("Connecting to MQTT broker: %s:%d", brokerHost.c_str(), brokerPort);
    
    String clientId = "coord-" + WiFi.macAddress();
    bool connected = false;
    
    if (brokerUsername.length() > 0 && brokerPassword.length() > 0) {
        connected = mqttClient.connect(clientId.c_str(), brokerUsername.c_str(), brokerPassword.c_str());
    } else {
        connected = mqttClient.connect(clientId.c_str());
    }
    
    if (connected) {
        Logger::info("MQTT connected!");
        
        // Subscribe to coordinator commands (PRD-compliant)
        String coordId = WiFi.macAddress();
        String cmdTopic = "site/site001/coord/" + coordId + "/cmd";
        mqttClient.subscribe(cmdTopic.c_str());
        
        Logger::info("Subscribed to: %s", cmdTopic.c_str());
        
        // Publish initial telemetry
        StaticJsonDocument<512> telemetry;
        telemetry["ts"] = millis() / 1000;
        telemetry["fw"] = "s3-1.0.0";
        telemetry["nodes_online"] = 0;
        telemetry["wifi_rssi"] = WiFi.RSSI();
        telemetry["mmwave_event_rate"] = 0.0;
        
        String payload;
        serializeJson(telemetry, payload);
        
        String telemetryTopic = "site/site001/coord/" + coordId + "/telemetry";
        mqttClient.publish(telemetryTopic.c_str(), payload.c_str());
        
        return true;
    }
    
    Logger::error("MQTT connection failed, rc=%d", mqttClient.state());
    return false;
}

void Mqtt::handleMqttMessage(char* topic, uint8_t* payload, unsigned int length) {
    if (mqttInstance && mqttInstance->commandCallback) {
        String topicStr = String(topic);
        String payloadStr = String((char*)payload, length);
        mqttInstance->processMessage(topicStr, payloadStr);
    }
}

void Mqtt::processMessage(const String& topic, const String& payload) {
    Logger::info("MQTT message received: %s", topic.c_str());
    
    if (commandCallback) {
        commandCallback(topic, payload);
    }
}
