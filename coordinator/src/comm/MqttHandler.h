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
               const char* username, const char* password);
    void loop();
    
    // State Publishing
    void publishNodeState(const String& nodeId, const NodeInfo& state);
    void publishNodeTemperature(const String& nodeId, float temperature);
    void publishZoneState(const String& zoneId, bool presence);
    void publishSystemStatus();
    
    // Command Registration
    void onNodeCommand(CommandCallback callback);
    void onZoneCommand(CommandCallback callback);
    void onSystemCommand(CommandCallback callback);
    
    // Configuration
    void publishConfig(const String& nodeId, const JsonDocument& config);
    void onConfigRequest(CommandCallback callback);
    
    // Discovery
    void publishDiscovery();
    
private:
    // Constants
    static constexpr const char* BASE_TOPIC = "smart_tile/";
    static constexpr size_t JSON_CAPACITY = 1024;
    
    // MQTT Client
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    
    // Callbacks
    CommandCallback nodeCommandCallback;
    CommandCallback zoneCommandCallback;
    CommandCallback systemCommandCallback;
    CommandCallback configCallback;
    
    // State tracking
    bool connected;
    uint32_t lastReconnectAttempt;
    
    // Helper methods
    String buildTopic(const char* suffix) const;
    String buildNodeTopic(const String& nodeId, const char* suffix) const;
    String buildZoneTopic(const String& zoneId, const char* suffix) const;
    
    bool reconnect();
    void handleMessage(char* topic, uint8_t* payload, unsigned int length);
    void subscribe();
    
    // Message processing
    void processNodeCommand(const String& nodeId, const JsonDocument& payload);
    void processZoneCommand(const String& zoneId, const JsonDocument& payload);
    void processSystemCommand(const JsonDocument& payload);
};
