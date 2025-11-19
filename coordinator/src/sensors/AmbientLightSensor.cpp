#include "AmbientLightSensor.h"
#include "../utils/Logger.h"

#if defined(ARDUINO_ARCH_ESP32)
#include <esp32-hal-adc.h>
#endif

AmbientLightSensor::AmbientLightSensor()
    : pin(Pins::External::AMBIENT_LIGHT_ADC)
    , initialized(false) {
}

bool AmbientLightSensor::begin(uint8_t analogPin) {
    pin = analogPin;
#if defined(ARDUINO_ARCH_ESP32)
    pinMode(pin, INPUT);
    int8_t channel = digitalPinToAnalogChannel(pin);
    if (channel < 0) {
        Logger::warn("Ambient light sensor pin %d has no ADC channel", pin);
        initialized = false;
        return false;
    }
    Logger::info("Ambient light sensor using ADC channel %d", channel);
    analogReadResolution(12);
    analogSetPinAttenuation(pin, ADC_11db);
#endif
    initialized = true;
    return true;
}

uint16_t AmbientLightSensor::readRaw() {
    if (!initialized) {
        return 0;
    }
    return analogRead(pin);
}

float AmbientLightSensor::readLux() {
    if (!initialized) {
        return 0.0f;
    }
    uint16_t raw = analogRead(pin);
    float voltage = (raw / 4095.0f) * 3.3f;
    // Simple approximation assuming 1V ≈ 300 lux with the default photoresistor divider
    return voltage * 300.0f;
}
