#pragma once

#include <Arduino.h>
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
    
    // Event handlers
    void onMmWaveEvent(const MmWaveEvent& event);
    void onThermalEvent(const String& nodeId, const NodeThermalData& data);
    void onButtonEvent(const String& buttonId, bool pressed);
};
