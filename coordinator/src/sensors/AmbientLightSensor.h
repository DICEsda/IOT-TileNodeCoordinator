#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_TSL2561_U.h>
#include "../config/PinConfig.h"

class AmbientLightSensor {
public:
    AmbientLightSensor();

    ~AmbientLightSensor();

    bool begin();
    float readLux();
    bool isConnected();

private:
    Adafruit_TSL2561_Unified* tsl;
    bool initialized;
};
