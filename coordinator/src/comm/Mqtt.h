#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <map>
#include <functional>
#include "../Models.h"
#include "../sensors/ThermalControl.h" // for NodeThermalData
#include "WifiManager.h"
#include "../../shared/src/EspNowMessage.h"
#include "../../shared/src/ConfigManager.h"

class Mqtt {
public:
    Mqtt();
    ~Mqtt();

    bool begin();
    void loop();
    bool isConnected();

    // Publishing methods
    void publishLightState(const String& lightId, uint8_t brightness);
    void publishThermalEvent(const String& nodeId, const NodeThermalData& data);
    void publishMmWaveEvent(const MmWaveEvent& event);
    void publishNodeStatus(const NodeStatusMessage& status);
    void publishCoordinatorTelemetry(const CoordinatorSensorSnapshot& snapshot);
    
    // Configuration
    void setBrokerConfig(const char* host, uint16_t port, const char* username, const char* password);
    void setWifiManager(WifiManager* manager);
    
    // Subscription handling
    void setCommandCallback(std::function<void(const String& topic, const String& payload)> callback);

    // Read-only broker info for telemetry/log formatting
    String getBrokerHost() const { return brokerHost; }
    uint16_t getBrokerPort() const { return brokerPort; }
    String getSiteId() const { return siteId; }
    String getCoordinatorId() const { return coordId; }

private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    ConfigManager config;
    
    // Configuration
    String brokerHost;
    uint16_t brokerPort;
    String brokerUsername;
    String brokerPassword;
    String siteId;
    String coordId;
         bool configLoaded = false;
         bool discoveryAttempted = false;
    
    WifiManager* wifiManager;
    std::function<void(const String& topic, const String& payload)> commandCallback;
    int8_t lastFailureState = 0;
    uint32_t lastDiagPrintMs = 0;
    bool loopbackHintPrinted = false;
    
    bool connectMqtt();
    bool ensureConfigLoaded();
    bool loadConfigFromStore();
    bool runProvisioningWizard();
    void persistConfig();
    static void handleMqttMessage(char* topic, uint8_t* payload, unsigned int length);
    void processMessage(const String& topic, const String& payload);
         bool autoDiscoverBroker();
         bool tryBrokerCandidate(const IPAddress& candidate);
    void logConnectionFailureDetail(int8_t state);
    const char* describeMqttState(int8_t state) const;
    void warnIfLoopbackHost();
    void runReachabilityProbe();

    String nodeTelemetryTopic(const String& nodeId) const;
    String coordinatorTelemetryTopic() const;
    String coordinatorCmdTopic() const;
    String coordinatorMmwaveTopic() const;
};
