#include "MqttHandler.h"

MqttHandler::MqttHandler() 
    : mqttClient(wifiClient)
    , connected(false)
    , lastReconnectAttempt(0)
    , siteId("site001")  // Default site ID
    , coordId("")        // Will be set to MAC address
{
    mqttClient.setCallback([this](char* topic, uint8_t* payload, unsigned int length) {
        this->handleMessage(topic, payload, length);
    });
}

MqttHandler::~MqttHandler() {
    mqttClient.disconnect();
}

bool MqttHandler::begin(const char* broker, uint16_t port, 
                       const char* username, const char* password,
                       const String& siteId, const String& coordId) {
    // Set site and coordinator IDs
    this->siteId = siteId;
    this->coordId = coordId.isEmpty() ? WiFi.macAddress() : coordId;
    
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

// PRD-compliant: site/{siteId}/node/{nodeId}/telemetry
void MqttHandler::publishNodeTelemetry(const String& nodeId, const JsonDocument& telemetry) {
    String topic = buildNodeTelemetryTopic(nodeId);
    String payload;
    serializeJson(telemetry, payload);
    mqttClient.publish(topic.c_str(), payload.c_str(), false);
}

// PRD-compliant: site/{siteId}/coord/{coordId}/telemetry
void MqttHandler::publishCoordTelemetry(const JsonDocument& telemetry) {
    String topic = buildCoordTelemetryTopic();
    String payload;
    serializeJson(telemetry, payload);
    mqttClient.publish(topic.c_str(), payload.c_str(), false);
}

// PRD-compliant: site/{siteId}/coord/{coordId}/mmwave
void MqttHandler::publishMmWaveEvent(const JsonDocument& event) {
    String topic = buildMmWaveTopic();
    String payload;
    serializeJson(event, payload);
    mqttClient.publish(topic.c_str(), payload.c_str(), false);
}

void MqttHandler::onNodeCommand(CommandCallback callback) {
    nodeCommandCallback = callback;
}

void MqttHandler::onCoordCommand(CommandCallback callback) {
    coordCommandCallback = callback;
}

void MqttHandler::publishConfig(const String& nodeId, const JsonDocument& config) {
    String payload;
    serializeJson(config, payload);
    String topic = "site/" + siteId + "/node/" + nodeId + "/config";
    mqttClient.publish(topic.c_str(), payload.c_str());
}

void MqttHandler::onConfigRequest(CommandCallback callback) {
    configCallback = callback;
}

// PRD-compliant topic builders
String MqttHandler::buildNodeTelemetryTopic(const String& nodeId) const {
    return "site/" + siteId + "/node/" + nodeId + "/telemetry";
}

String MqttHandler::buildCoordTelemetryTopic() const {
    return "site/" + siteId + "/coord/" + coordId + "/telemetry";
}

String MqttHandler::buildMmWaveTopic() const {
    return "site/" + siteId + "/coord/" + coordId + "/mmwave";
}

String MqttHandler::buildNodeCmdTopic(const String& nodeId) const {
    return "site/" + siteId + "/node/" + nodeId + "/cmd";
}

String MqttHandler::buildCoordCmdTopic() const {
    return "site/" + siteId + "/coord/" + coordId + "/cmd";
}

bool MqttHandler::reconnect() {
    String clientId = "coord-" + coordId;
    if (mqttClient.connect(clientId.c_str())) {
        subscribe();
        connected = true;
        // Publish initial telemetry on connect
        StaticJsonDocument<256> telemetry;
        telemetry["ts"] = millis() / 1000;
        telemetry["fw"] = "1.0.0";
        telemetry["coord_id"] = coordId;
        telemetry["site_id"] = siteId;
        publishCoordTelemetry(telemetry);
        return true;
    }
    return false;
}

void MqttHandler::subscribe() {
    // Subscribe to command topics (PRD-compliant)
    String coordCmdTopic = buildCoordCmdTopic();
    mqttClient.subscribe(coordCmdTopic.c_str());
    
    // Subscribe to node commands with wildcard
    String nodesCmdPattern = "site/" + siteId + "/node/+/cmd";
    mqttClient.subscribe(nodesCmdPattern.c_str());
}

void MqttHandler::handleMessage(char* topic, uint8_t* payload, unsigned int length) {
    String topicStr = String(topic);
    String payloadStr = String((char*)payload, length);
    
    StaticJsonDocument<JSON_CAPACITY> doc;
    DeserializationError error = deserializeJson(doc, payloadStr);
    
    if (error) {
        return;
    }
    
    // Parse PRD-compliant topics: site/{siteId}/node/{nodeId}/cmd or site/{siteId}/coord/{coordId}/cmd
    if (topicStr.startsWith("site/" + siteId + "/node/")) {
        // Extract node ID from topic: site/{siteId}/node/{nodeId}/cmd
        int nodeStart = topicStr.indexOf("/node/") + 6;
        int nodeEnd = topicStr.indexOf("/", nodeStart);
        if (nodeEnd == -1) nodeEnd = topicStr.length();
        String nodeId = topicStr.substring(nodeStart, nodeEnd);
        
        if (topicStr.endsWith("/cmd") && nodeCommandCallback) {
            nodeCommandCallback(nodeId, doc);
        }
    } else if (topicStr == buildCoordCmdTopic() && coordCommandCallback) {
        coordCommandCallback(topicStr, doc);
    }
}
