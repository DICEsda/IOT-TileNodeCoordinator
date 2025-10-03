#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "../config/PinConfig.h"

class TempSensor {
public:
    struct Config {
        float derateStartC;      // Temperature at which to start derating (default 70°C)
        float derateMaxC;        // Temperature at which to reach minimum duty (default 85°C)
        uint8_t derateMinDuty;   // Minimum duty cycle when fully derated (percent)
        uint32_t sampleInterval; // Sampling interval in milliseconds
    };

    TempSensor(uint8_t csPin);
    void begin(const Config& config = {70.0, 85.0, 30, 1000});
    void update();
    
    float getTemperature() const { return currentTemp; }
    bool isOverTemp() const { return currentTemp >= config.derateStartC; }
    uint8_t getDeratedDuty(uint8_t requestedDuty) const;

private:
    uint8_t csPin;
    Config config;
    float currentTemp;
    uint32_t lastSampleTime;

    float readTemperature();
    uint8_t calculateDeratedDuty(uint8_t requestedDuty) const;
};
