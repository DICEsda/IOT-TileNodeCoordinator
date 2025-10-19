#pragma once

#include <Arduino.h>
#include "../config/PinConfig.h"
#include <Adafruit_NeoPixel.h>

class StatusLed {
public:
    StatusLed() : strip(Pins::RgbLed::NUM_PIXELS, Pins::RgbLed::PIN, NEO_GRBW + NEO_KHZ800) {}
    ~StatusLed() {}

    bool begin() {
        strip.begin();
        strip.show(); // Initialize all pixels to 'off'
        active = false;
        return true;
    }

    // Non-blocking pulse: set color for durationMs starting now
    void pulse(uint8_t r, uint8_t g, uint8_t b, uint32_t durationMs = 1000) {
        pulseEnd = millis() + durationMs;
        targetR = r; targetG = g; targetB = b;
        active = true;
        setAll(r, g, b);
    }

    void loop() {
        if (active && millis() >= pulseEnd) {
            // Turn off all pixels
            setAll(0, 0, 0);
            active = false;
        }
    }

    void setAll(uint8_t r, uint8_t g, uint8_t b) {
        for (uint8_t i = 0; i < strip.numPixels(); ++i) {
            strip.setPixelColor(i, strip.Color(r, g, b, 0));
        }
        strip.show();
    }

private:
    Adafruit_NeoPixel strip;
    bool active = false;
    uint32_t pulseEnd = 0;
    uint8_t targetR = 0, targetG = 0, targetB = 0;
};
