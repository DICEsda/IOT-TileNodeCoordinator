#pragma once

#include <Arduino.h>
#include "../config/PinConfig.h"

class AmbientLightSensor {
public:
    AmbientLightSensor();

    bool begin(uint8_t analogPin = Pins::External::AMBIENT_LIGHT_ADC);
    float readLux();
    uint16_t readRaw();

private:
    uint8_t pin;
    bool initialized;
};
