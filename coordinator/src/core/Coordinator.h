#pragma once

#include <Arduino.h>
#include <map>
#include <vector>
#include "../comm/EspNow.h"
#include "../comm/Mqtt.h"
#include "../sensors/MmWave.h"
#include "../nodes/NodeRegistry.h"
#include "../zones/ZoneControl.h"
#include "../input/ButtonControl.h"
#include "../sensors/ThermalControl.h"
#include "../utils/StatusLed.h"

class WifiManager;
class AmbientLightSensor;
struct NodeStatusMessage;

class Coordinator {
public:
    Coordinator();
    ~Coordinator();

    bool begin();
    void loop();

private:
    EspNow* espNow;
    Mqtt* mqtt;
    MmWave* mmWave;
    NodeRegistry* nodes;
    ZoneControl* zones;
    ButtonControl* buttons;
    ThermalControl* thermal;
    WifiManager* wifi;
    AmbientLightSensor* ambientLight;
    // Onboard status LED helper
    StatusLed statusLed;
    struct BootStatusEntry {
        String name;
        bool ok;
        String detail;
    };
    std::vector<BootStatusEntry> bootStatus;

    struct NodeTelemetrySnapshot {
        uint8_t avgR = 0;
        uint8_t avgG = 0;
        uint8_t avgB = 0;
        uint8_t avgW = 0;
        float temperatureC = 0.0f;
        bool buttonPressed = false;
        uint32_t lastUpdateMs = 0;
    };
    std::map<String, NodeTelemetrySnapshot> nodeTelemetry;
    CoordinatorSensorSnapshot coordinatorSensors;
    MmWaveEvent lastMmWaveEvent;
    bool haveMmWaveSample = false;
    uint32_t lastSensorSampleMs = 0;
    uint32_t lastSerialPrintMs = 0;

    // Per-node LED group mapping (4 pixels per group)
    std::map<String, int> nodeToGroup;         // nodeId -> group index (0..groups-1)
    std::vector<String> groupToNode;           // size = NUM_PIXELS/4
    std::vector<bool> groupConnected;          // true if connected
    std::vector<uint32_t> groupFlashUntilMs;   // activity flash until ts

    // Helpers for LED mapping and updates
    void rebuildLedMappingFromRegistry();
    int getGroupIndexForNode(const String& nodeId);
    int assignGroupForNode(const String& nodeId);
    void updateLeds();
    void flashLedForNode(const String& nodeId, uint32_t durationMs);
    void logConnectedNodes();
    void checkStaleConnections();
    void sendHealthPings();

    // Button/flash state
    bool buttonDown = false;
    bool longPressActive = false;
    bool flashAllActive = false;
    bool flashOn = false;
    uint32_t lastFlashTick = 0;
    uint32_t buttonPressedAt = 0;
    void startFlashAll();
    void stopFlashAll();
    void flashAllTick(uint32_t now);
    
    // Event handlers
    void onMmWaveEvent(const MmWaveEvent& event);
    void onThermalEvent(const String& nodeId, const NodeThermalData& data);
    void onButtonEvent(const String& buttonId, bool pressed);
    void handleNodeMessage(const String& nodeId, const uint8_t* data, size_t len);
    void triggerNodeWaveTest();
    void handleMqttCommand(const String& topic, const String& payload);
    void startPairingWindow(uint32_t durationMs, const char* reason);
    void updateNodeTelemetryCache(const String& nodeId, const NodeStatusMessage& statusMsg);
    void refreshCoordinatorSensors();
    void printSerialTelemetry();
    float readInternalTemperatureC() const;
    void recordBootStatus(const char* name, bool ok, const String& detail);
    void printBootSummary();
};
