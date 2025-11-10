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
    // Onboard status LED helper
    StatusLed statusLed;

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
};
