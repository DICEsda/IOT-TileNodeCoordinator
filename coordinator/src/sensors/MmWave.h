#pragma once

#include <Arduino.h>
#include "../config/PinConfig.h"
#include "../Models.h"
#include <functional>

class MmWave {
public:
    MmWave();
    ~MmWave();

    bool begin();
    void loop();

    // Event handling
    void setEventCallback(std::function<void(const MmWaveEvent& event)> callback);

private:
    static constexpr uint32_t PRESENCE_DEBOUNCE_MS = 150;
    
    std::function<void(const MmWaveEvent& event)> eventCallback;
    String currentZone;
    bool currentPresence;
    uint32_t lastEventTime;

    void processSerialData();
    void emitPresenceEvent(const String& zone, bool presence);
};
