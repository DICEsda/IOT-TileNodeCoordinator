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

    // Non-blocking pulse: set color for durationMs starting now (affects all pixels)
    void pulse(uint8_t r, uint8_t g, uint8_t b, uint32_t durationMs = 1000) {
        pulseEnd = millis() + durationMs;
        targetR = r; targetG = g; targetB = b;
        active = true;
        setAll(r, g, b);
    }

    void loop() {
        uint32_t now = millis();
        if (active && now >= pulseEnd) {
            // End pulse
            active = false;
        }
        if (idleBreathing && !active) {
            // Warm white breathing at ~2s period, 10% brightness target
            float t = (now % 2000) / 2000.0f;
            float tri = t < 0.5f ? (t * 2.0f) : (2.0f - t * 2.0f);
            uint8_t w = 25 + (uint8_t)(tri * 30); // ~10%-20%
            setAllWarmWhite(w);
        } else if (!active) {
            // If neither breathing nor pulse, defer to per-pixel state set by caller
            // (no-op to avoid flicker)
        }
    }

    // Set all pixels to RGB (no white)
    void setAll(uint8_t r, uint8_t g, uint8_t b) {
        for (uint8_t i = 0; i < strip.numPixels(); ++i) {
            strip.setPixelColor(i, strip.Color(r, g, b, 0));
        }
        strip.show();
    }

    // Per-pixel control helpers
    void clear() {
        strip.clear();
        strip.show();
    }
    void setPixel(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0) {
        if (index >= strip.numPixels()) return;
        strip.setPixelColor(index, strip.Color(r, g, b, w));
    }
    void show() { strip.show(); }
    uint8_t numPixels() const { return strip.numPixels(); }

    void setIdleBreathing(bool enable) {
        idleBreathing = enable;
        if (!enable && !active) {
            // Clear to off when disabling idle state
            setAll(0, 0, 0);
        }
    }

    void setAllWarmWhite(uint8_t w) {
        for (uint8_t i = 0; i < strip.numPixels(); ++i) {
            strip.setPixelColor(i, strip.Color(0, 0, 0, w));
        }
        strip.show();
    }

    bool isPulsing() const { return active; }

private:
    Adafruit_NeoPixel strip;
    bool active = false;
    uint32_t pulseEnd = 0;
    uint8_t targetR = 0, targetG = 0, targetB = 0;
    bool idleBreathing = false;
};
