#pragma once

#include <Arduino.h>
#include "../Models.h"

class MmWave {
public:
    MmWave();
    ~MmWave();

    bool begin();
    void loop();

    // Event handling
    void setEventCallback(void (*callback)(const MmWaveEvent& event));

private:
    static constexpr uint32_t PRESENCE_DEBOUNCE_MS = 150;
    #include "../config/PinConfig.h"
    
    void (*eventCallback)(const MmWaveEvent& event);
    String currentZone;
    bool currentPresence;
    uint32_t lastEventTime;

    void processSerialData();
    void emitPresenceEvent(const String& zone, bool presence);
};
