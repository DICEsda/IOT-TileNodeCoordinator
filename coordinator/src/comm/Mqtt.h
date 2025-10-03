#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <map>
#include "../Models.h"

class Mqtt {
public:
    Mqtt();
    ~Mqtt();

    bool begin();
    void loop();
    bool isConnected() const;

    // Publishing methods
    void publishLightState(const String& lightId, uint8_t brightness);
    void publishThermalEvent(const String& nodeId, const NodeThermalData& data);
    void publishMmWaveEvent(const MmWaveEvent& event);
    void publishNodeStatus(const String& nodeId, const NodeInfo& status);
    
    // Configuration
    void setWifiCredentials(const char* ssid, const char* password);
    void setBrokerConfig(const char* host, uint16_t port, const char* username, const char* password);
    
    // Subscription handling
    void setCommandCallback(void (*callback)(const String& topic, const String& payload));

private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    
    // Configuration
    String wifiSsid;
    String wifiPassword;
    String brokerHost;
    uint16_t brokerPort;
    String brokerUsername;
    String brokerPassword;
    
    void (*commandCallback)(const String& topic, const String& payload);
    
    bool connectWifi();
    bool connectMqtt();
    static void handleMqttMessage(char* topic, uint8_t* payload, unsigned int length);
    void processMessage(const String& topic, const String& payload);
};
