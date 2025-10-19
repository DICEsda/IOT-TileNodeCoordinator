#include "MqttHandler.h"

MqttHandler::MqttHandler() : mqttClient(wifiClient), connected(false), lastReconnectAttempt(0) {
    mqttClient.setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
        this->handleMessage(topic, payload, length);
    });
}

MqttHandler::~MqttHandler() {
    mqttClient.disconnect();
}

bool MqttHandler::begin(const char* broker, uint16_t port, 
                       const char* username, const char* password) {
    mqttClient.setServer(broker, port);
    
    if (username && password) {
        return reconnect();
    }
    return true;
}

void MqttHandler::loop() {
    if (!mqttClient.connected()) {
        uint32_t now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    }
    mqttClient.loop();
}

void MqttHandler::publishNodeState(const String& nodeId, const NodeInfo& state) {
    StaticJsonDocument<JSON_CAPACITY> doc;
    // Use fields available on NodeInfo
    doc["last_duty"] = state.lastDuty;
    doc["temperature"] = state.temperature;
    doc["last_seen_ms"] = state.lastSeenMs;
    
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(buildNodeTopic(nodeId, "state").c_str(), payload.c_str());
}

void MqttHandler::publishNodeTemperature(const String& nodeId, float temperature) {
    StaticJsonDocument<64> doc;
    doc["temperature"] = temperature;
    
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(buildNodeTopic(nodeId, "temperature").c_str(), payload.c_str());
}

void MqttHandler::publishZoneState(const String& zoneId, bool presence) {
    StaticJsonDocument<64> doc;
    doc["presence"] = presence;
    
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(buildZoneTopic(zoneId, "state").c_str(), payload.c_str());
}

void MqttHandler::publishSystemStatus() {
    StaticJsonDocument<JSON_CAPACITY> doc;
    // Add system status fields
    doc["uptime"] = millis() / 1000;
    doc["free_heap"] = ESP.getFreeHeap();
    doc["wifi_rssi"] = WiFi.RSSI();
    
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(buildTopic("status").c_str(), payload.c_str());
}

void MqttHandler::onNodeCommand(CommandCallback callback) {
    nodeCommandCallback = callback;
}

void MqttHandler::onZoneCommand(CommandCallback callback) {
    zoneCommandCallback = callback;
}

void MqttHandler::onSystemCommand(CommandCallback callback) {
    systemCommandCallback = callback;
}

void MqttHandler::publishConfig(const String& nodeId, const JsonDocument& config) {
    String payload;
    serializeJson(config, payload);
    mqttClient.publish(buildNodeTopic(nodeId, "config").c_str(), payload.c_str());
}

void MqttHandler::onConfigRequest(CommandCallback callback) {
    configCallback = callback;
}

void MqttHandler::publishDiscovery() {
    StaticJsonDocument<JSON_CAPACITY> doc;
    // Add discovery information
    doc["coordinator_id"] = WiFi.macAddress();
    doc["ip"] = WiFi.localIP().toString();
    doc["version"] = "1.0.0"; // Update with actual version
    
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(buildTopic("discovery").c_str(), payload.c_str(), true);
}

String MqttHandler::buildTopic(const char* suffix) const {
    return String(BASE_TOPIC) + suffix;
}

String MqttHandler::buildNodeTopic(const String& nodeId, const char* suffix) const {
    return String(BASE_TOPIC) + "nodes/" + nodeId + "/" + suffix;
}

String MqttHandler::buildZoneTopic(const String& zoneId, const char* suffix) const {
    return String(BASE_TOPIC) + "zones/" + zoneId + "/" + suffix;
}

bool MqttHandler::reconnect() {
    if (mqttClient.connect(WiFi.macAddress().c_str())) {
        subscribe();
        connected = true;
        publishDiscovery();
        return true;
    }
    return false;
}

void MqttHandler::subscribe() {
    // Subscribe to command topics
    mqttClient.subscribe((buildTopic("nodes/+/command")).c_str());
    mqttClient.subscribe((buildTopic("zones/+/command")).c_str());
    mqttClient.subscribe((buildTopic("system/command")).c_str());
    mqttClient.subscribe((buildTopic("nodes/+/config/request")).c_str());
}

void MqttHandler::handleMessage(char* topic, uint8_t* payload, unsigned int length) {
    String topicStr = String(topic);
    String payloadStr = String((char*)payload, length);
    
    StaticJsonDocument<JSON_CAPACITY> doc;
    DeserializationError error = deserializeJson(doc, payloadStr);
    
    if (error) {
        return;
    }
    
    // Parse topic to determine message type
    if (topicStr.startsWith(buildTopic("nodes/"))) {
        String nodeId = topicStr.substring(
            strlen(BASE_TOPIC) + 6, 
            topicStr.lastIndexOf('/')
        );
        
        if (topicStr.endsWith("/command") && nodeCommandCallback) {
            nodeCommandCallback(nodeId, doc);
        } else if (topicStr.endsWith("/config/request") && configCallback) {
            configCallback(nodeId, doc);
        }
    } else if (topicStr.startsWith(buildTopic("zones/"))) {
        String zoneId = topicStr.substring(
            strlen(BASE_TOPIC) + 6, 
            topicStr.lastIndexOf('/')
        );
        
        if (topicStr.endsWith("/command") && zoneCommandCallback) {
            zoneCommandCallback(zoneId, doc);
        }
    } else if (topicStr == buildTopic("system/command") && systemCommandCallback) {
        systemCommandCallback(topicStr, doc);
    }
}
