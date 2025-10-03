#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "../config/PinConfig.h"

class LedController {
public:
    explicit LedController(uint16_t numPixels = 1);
    ~LedController();

    void begin();
    void setBrightness(uint8_t brightness, uint16_t fadeMs = 0);
    void setColor(uint8_t r, uint8_t g, uint8_t b, uint16_t fadeMs = 0);
    void update(); // Call this in loop to handle fading
    void clear();
    uint8_t getCurrentBrightness() const { return currentBrightness; }
    uint16_t numPixels() const { return strip.numPixels(); }

private:
    Adafruit_NeoPixel strip;
    uint8_t currentBrightness;
    uint8_t targetBrightness;
    uint32_t currentColor;
    uint32_t targetColor;
    uint32_t fadeStartTime;
    uint16_t fadeDuration;
    bool fading;

    void interpolateColor();
};
