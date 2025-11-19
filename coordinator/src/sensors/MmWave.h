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
    bool isOnline() const;
    uint16_t getRestartCount() const { return totalRestarts; }

private:
    static constexpr uint32_t PRESENCE_DEBOUNCE_MS = 150; // debounce between state changes
    static constexpr uint32_t MIN_PUBLISH_INTERVAL_MS = 120; // limit MQTT event rate
    static constexpr uint8_t MAX_SENSOR_TARGETS = 4; // LD2450 supports up to 4 in latest firmware
    static constexpr uint32_t STREAM_STALE_MS = 2500;
    static constexpr uint32_t RESTART_BACKOFF_MS = 1500;
    static constexpr uint8_t MAX_RESTARTS_BEFORE_OFFLINE = 4;
    static constexpr uint32_t OFFLINE_RETRY_MS = 15000;

    // LD2450 radar instance (UART streaming)
    HardwareSerial* radarSerial;
    bool radarReady = false;
    uint32_t lastPublishMs = 0;
    uint32_t lastFrameMs = 0;
    uint8_t consecutiveFailures = 0;
    uint16_t totalRestarts = 0;
    uint8_t restartAttempts = 0;
    uint32_t lastRestartMs = 0;
    uint32_t offlineSinceMs = 0;
    bool sensorSuppressed = false;
    bool offlineHintPrinted = false;
    
    std::function<void(const MmWaveEvent& event)> eventCallback;
    String currentZone;
    bool currentPresence;
    uint32_t lastEventTime;

    void processRadarFrame();
    void emitPresenceEvent(const MmWaveEvent& evt);
    void buildEventFromTargets(MmWaveEvent& evt);
    void ensureStreamHealth();
    bool restartRadar(const char* reason);
};
