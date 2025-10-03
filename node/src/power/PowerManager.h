#pragma once

#include <Arduino.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include "../config/PinConfig.h"

class PowerManager {
public:
    PowerManager();
    ~PowerManager() = default;

    void begin();
    void loop();

    // Sleep control
    void scheduleWakeup(uint32_t milliseconds);
    void enterLightSleep();
    void enterDeepSleep(uint32_t seconds = 0);
    
    // Power state management
    void setRxWindow(uint32_t interval_ms);
    void extendWakeTime(uint32_t milliseconds);
    bool canSleep() const;
    
    // Power monitoring
    float getBatteryVoltage() const;
    uint8_t getBatteryPercent() const;
    bool isLowBattery() const;
    
    // Event callbacks
    using PowerCallback = void (*)(const char* event, float value);
    void setPowerCallback(PowerCallback callback);

private:
    static constexpr uint32_t MIN_SLEEP_MS = 50;
    static constexpr float LOW_BATTERY_THRESHOLD = 3.3f;
    static constexpr uint8_t VOLTAGE_PIN = 1;   // ADC1_CH1
    
    uint32_t lastRxWindow;
    uint32_t rxInterval;
    uint32_t forcedWakeUntil;
    PowerCallback callback;
    
    void configurePowerPins();
    void setupVoltageMonitor();
    float readVoltage() const;
    void checkBatteryStatus();
    
    // Sleep helpers
    void prepareForSleep();
    void wakeupFromSleep();
};
