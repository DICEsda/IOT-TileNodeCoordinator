// Minimal PowerManager interface matching current implementation
#pragma once

#include <Arduino.h>
#include <esp_sleep.h>

class PowerManager {
public:
    PowerManager() = default;
    ~PowerManager() = default;

    // Configure RX timing window/period (milliseconds)
    inline void configure(uint16_t windowMs, uint16_t periodMs) {
        rxWindowMs = windowMs;
        rxPeriodMs = periodMs;
    }

    // Enter light sleep only when idle and outside RX window
    inline void enterLightSleepIfIdle(bool isIdle) {
        if (!isIdle) return;
        if (isRxWindowActive()) return;
        esp_sleep_enable_timer_wakeup((uint64_t)rxPeriodMs * 1000ULL);
        esp_light_sleep_start();
        lastRxWindow = millis();
    }

    // Query/mark RX window
    inline bool isRxWindowActive() const {
        return (millis() - lastRxWindow) < rxWindowMs;
    }

    inline void markRxWindow() {
        lastRxWindow = millis();
    }

private:
    uint32_t lastRxWindow{0};
    uint16_t rxWindowMs{20};
    uint16_t rxPeriodMs{100};
};
