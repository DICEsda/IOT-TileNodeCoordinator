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

    // Make these public so callback can access them
    uint32_t lastFrameMs = 0;
    uint8_t consecutiveFailures = 0;
    bool radarReady = false;

private:
    static constexpr uint32_t PRESENCE_DEBOUNCE_MS = 150; // debounce between state changes
    static constexpr uint32_t MIN_PUBLISH_INTERVAL_MS = 120; // limit MQTT event rate (~8.3 Hz)
    static constexpr uint8_t MAX_SENSOR_TARGETS = 3; // LD2450 v1.0.1 supports 3 targets (TARGET_1, TARGET_2, TARGET_3)
    static constexpr uint32_t STREAM_STALE_MS = 2500;
    static constexpr uint8_t SERIAL_LOG_DIVIDER = 3; // Log every Nth frame to serial (1=all, 3=every 3rd)
    static constexpr uint32_t RESTART_BACKOFF_MS = 1500;
    static constexpr uint8_t MAX_RESTARTS_BEFORE_OFFLINE = 4;
    static constexpr uint32_t OFFLINE_RETRY_MS = 15000;

    // LD2450 radar instance (UART streaming)
    HardwareSerial* radarSerial;
    uint32_t lastPublishMs = 0;
    uint16_t totalRestarts = 0;
    uint8_t restartAttempts = 0;
    uint32_t lastRestartMs = 0;
    uint32_t offlineSinceMs = 0;
    bool sensorSuppressed = false;
    bool offlineHintPrinted = false;
    uint32_t frameCounter = 0; // For serial logging throttle
    
    std::function<void(const MmWaveEvent& event)> eventCallback;
    String currentZone;
    bool currentPresence;
    bool currentZoneOccupied;
    uint32_t lastEventTime;

    void processRadarFrame();
    bool isTargetInZone(int16_t x_mm, int16_t y_mm) const;
    void emitPresenceEvent(const MmWaveEvent& evt);
    void buildEventFromTargets(MmWaveEvent& evt);
    void ensureStreamHealth();
    bool restartRadar(const char* reason);
};
