#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <map>
#include <functional>
#include "../Models.h"

class MqttHandler {
public:
    using CommandCallback = std::function<void(const String& topic, const JsonDocument& payload)>;

    MqttHandler();
    ~MqttHandler();

    bool begin(const char* broker, uint16_t port, 
               const char* username, const char* password,
               const String& siteId, const String& coordId);
    void loop();
    
    // Site/Coordinator Configuration
    void setSiteId(const String& siteId) { this->siteId = siteId; }
    void setCoordId(const String& coordId) { this->coordId = coordId; }
    String getSiteId() const { return siteId; }
    String getCoordId() const { return coordId; }
    
    // State Publishing (PRD-compliant)
    void publishNodeTelemetry(const String& nodeId, const JsonDocument& telemetry);
    void publishCoordTelemetry(const JsonDocument& telemetry);
    void publishMmWaveEvent(const JsonDocument& event);
    
    // Command Registration
    void onNodeCommand(CommandCallback callback);
    void onCoordCommand(CommandCallback callback);
    
    // Configuration
    void publishConfig(const String& nodeId, const JsonDocument& config);
    void onConfigRequest(CommandCallback callback);
    
private:
    // Site and Coordinator IDs
    String siteId;
    String coordId;
    
    // Constants
    static constexpr size_t JSON_CAPACITY = 1024;
    
    // MQTT Client
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    
    // Callbacks
    CommandCallback nodeCommandCallback;
    CommandCallback coordCommandCallback;
    CommandCallback configCallback;
    
    // State tracking
    bool connected;
    uint32_t lastReconnectAttempt;
    
    // Helper methods - PRD-compliant topic builders
    String buildNodeTelemetryTopic(const String& nodeId) const;
    String buildCoordTelemetryTopic() const;
    String buildMmWaveTopic() const;
    String buildNodeCmdTopic(const String& nodeId) const;
    String buildCoordCmdTopic() const;
    
    bool reconnect();
    void handleMessage(char* topic, uint8_t* payload, unsigned int length);
    void subscribe();
    
    // Message processing
    void processNodeCommand(const String& nodeId, const JsonDocument& payload);
    void processCoordCommand(const JsonDocument& payload);
};
