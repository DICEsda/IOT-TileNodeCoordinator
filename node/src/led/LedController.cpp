#include "LedController.h"

LedController::LedController(uint16_t numPixels)
        : strip(numPixels, Pins::LED_DATA, NEO_GRB + NEO_KHZ800),
            currentBrightness(0),
            targetBrightness(0),
            currentColor(0),
            targetColor(0),
            fadeStartTime(0),
            fadeDuration(0),
            fading(false) {
}

LedController::~LedController() {
    clear();
}

void LedController::begin() {
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
}

void LedController::setBrightness(uint8_t brightness, uint16_t fadeMs) {
    targetBrightness = brightness;
    if (fadeMs > 0) {
        fadeStartTime = millis();
        fadeDuration = fadeMs;
        fading = true;
    } else {
        currentBrightness = brightness;
        strip.setBrightness(brightness);
        strip.show();
    }
}

void LedController::setColor(uint8_t r, uint8_t g, uint8_t b, uint16_t fadeMs) {
    targetColor = strip.Color(r, g, b);
    if (fadeMs > 0) {
        fadeStartTime = millis();
        fadeDuration = fadeMs;
        fading = true;
    } else {
        currentColor = targetColor;
        for (uint16_t i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, currentColor);
        }
        strip.show();
    }
}

void LedController::update() {
    if (!fading) return;

    uint32_t currentTime = millis();
    uint32_t elapsed = currentTime - fadeStartTime;

    if (elapsed >= fadeDuration) {
        // Fading complete
        currentBrightness = targetBrightness;
        currentColor = targetColor;
        strip.setBrightness(currentBrightness);
        for (uint16_t i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, currentColor);
        }
        strip.show();
        fading = false;
        return;
    }

    // Calculate progress (0.0 to 1.0)
    float progress = static_cast<float>(elapsed) / fadeDuration;

    // Interpolate brightness
    if (currentBrightness != targetBrightness) {
        uint8_t newBrightness = currentBrightness + 
            (targetBrightness - currentBrightness) * progress;
        strip.setBrightness(newBrightness);
    }

    // Interpolate color
    if (currentColor != targetColor) {
        uint8_t r1 = (currentColor >> 16) & 0xFF;
        uint8_t g1 = (currentColor >> 8) & 0xFF;
        uint8_t b1 = currentColor & 0xFF;
        
        uint8_t r2 = (targetColor >> 16) & 0xFF;
        uint8_t g2 = (targetColor >> 8) & 0xFF;
        uint8_t b2 = targetColor & 0xFF;
        
        uint8_t r = r1 + (r2 - r1) * progress;
        uint8_t g = g1 + (g2 - g1) * progress;
        uint8_t b = b1 + (b2 - b1) * progress;
        
        uint32_t interpolatedColor = strip.Color(r, g, b);
        for (uint16_t i = 0; i < strip.numPixels(); i++) {
            strip.setPixelColor(i, interpolatedColor);
        }
    }

    strip.show();
}

void LedController::clear() {
    strip.clear();
    strip.show();
}
