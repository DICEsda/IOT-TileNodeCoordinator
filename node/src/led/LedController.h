#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "../config/PinConfig.h"

class LedController {
public:
    explicit LedController(uint16_t numPixelsPerStrip = 4);
    ~LedController();

    void begin();
    void setBrightness(uint8_t brightness, uint16_t fadeMs = 0);
    void setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0, uint16_t fadeMs = 0);
    void update(); // Call this in loop to handle fading
    void clear();
    uint8_t getCurrentBrightness() const { return currentBrightness; }
    uint16_t numPixels() const { return 12; } // Total: 3 strips x 4 LEDs
    uint32_t getCurrentColor() const { return currentColor; }

    // Status patterns per PRD v0.5 (pairing/ota/error/operational)
    // Added Connected mode: green trailing animation indicating readiness
    enum class StatusMode { None, Pairing, OTA, Error, Idle, Connected, Wave };
    void setStatus(StatusMode mode);
    bool isAnimating() const { return status != StatusMode::None; }
    
    // Wave control (coordinated test)
    void startWave(uint16_t periodMs, uint32_t durationMs);

private:
    static constexpr uint8_t BRIGHTNESS_CAP = 128; // 50% cap globally for all modes
    static constexpr uint8_t NUM_STRIPS = 3; // 3 LED strips

    Adafruit_NeoPixel strip1;
    Adafruit_NeoPixel strip2;
    Adafruit_NeoPixel strip3;
    uint8_t currentBrightness;
    uint8_t targetBrightness;
    uint32_t currentColor;
    uint32_t targetColor;
    uint32_t fadeStartTime;
    uint16_t fadeDuration;
    bool fading;
    StatusMode status = StatusMode::None;
    uint32_t lastAnimMs = 0;

    void interpolateColor();
    void runStatusAnimation();
};
