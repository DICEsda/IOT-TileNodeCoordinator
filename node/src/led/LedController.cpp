#include "LedController.h"
#include <math.h>

LedController::LedController(uint16_t numPixels)
    : strip(numPixels, Pins::LED_DATA, NEO_GRBW + NEO_KHZ800),
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

void LedController::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint16_t fadeMs) {
    targetColor = strip.Color(r, g, b, w);
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
    if (status != StatusMode::None) {
        runStatusAnimation();
        // status animation manages show()
    }
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

    // Interpolate color (RGBA packed)
    if (currentColor != targetColor) {
        uint8_t r1 = (currentColor >> 16) & 0xFF;
        uint8_t g1 = (currentColor >> 8) & 0xFF;
        uint8_t b1 = currentColor & 0xFF;
        uint8_t w1 = (currentColor >> 24) & 0xFF;

        uint8_t r2 = (targetColor >> 16) & 0xFF;
        uint8_t g2 = (targetColor >> 8) & 0xFF;
        uint8_t b2 = targetColor & 0xFF;
        uint8_t w2 = (targetColor >> 24) & 0xFF;

        uint8_t r = r1 + (int)((r2 - r1) * progress);
        uint8_t g = g1 + (int)((g2 - g1) * progress);
        uint8_t b = b1 + (int)((b2 - b1) * progress);
        uint8_t w = w1 + (int)((w2 - w1) * progress);

        uint32_t interpolatedColor = strip.Color(r, g, b, w);
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

void LedController::setStatus(StatusMode mode) {
    status = mode;
    lastAnimMs = 0; // reset animation timer
}

void LedController::runStatusAnimation() {
    uint32_t now = millis();
    switch (status) {
        case StatusMode::Pairing: {
            // Synchronized "heartbeat" double-pulse in blue with a longer delay
            // Two quick pulses (lub-dub) then rest. Total cycle ~1600ms.
            const uint32_t cycleMs = 1600;
            uint32_t t = now % cycleMs;

            // Gaussian-like pulses for smooth rise/fall
            auto pulse = [](float x, float mu, float sigma) -> float {
                float d = (x - mu) / sigma;
                return expf(-0.5f * d * d); // 0..1
            };

            // Centers and widths (ms) for the two pulses
            float a1 = pulse((float)t, 110.0f, 55.0f);   // first beat around 110ms
            float a2 = pulse((float)t, 310.0f, 55.0f);   // second beat ~200ms later
            float a = fminf(1.0f, a1 + a2);

            // Map amplitude to brightness and blue intensity
            uint8_t br = (uint8_t)(26 + a * 120.0f);     // brightness 26..146
            int blue = (int)(70 + a * 185.0f);           // blue 70..255
            if (blue > 255) blue = 255;

            strip.setBrightness(br);
            uint16_t N = strip.numPixels();
            for (uint16_t i = 0; i < N; ++i) {
                strip.setPixelColor(i, strip.Color(0, 0, (uint8_t)blue, 0));
            }
            strip.show();
            break;
        }
        case StatusMode::OTA: {
            // Chasing blue dot
            uint16_t N = strip.numPixels();
            uint16_t idx = (now / 120) % (N ? N : 1);
            strip.clear();
            for (uint16_t i = 0; i < N; ++i) {
                uint8_t dim = (i == idx) ? 100 : 10;
                strip.setPixelColor(i, strip.Color(0, 0, dim, 0));
            }
            strip.setBrightness(60); // <= 30%
            strip.show();
            break;
        }
        case StatusMode::Error: {
            // Double red flash every 1.2s
            uint16_t phase = now % 1200;
            bool on = (phase < 120) || (phase > 240 && phase < 360);
            strip.setBrightness(on ? 76 : 0);
            uint32_t c = strip.Color(180, 10, 10, 0);
            for (uint16_t i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, c);
            strip.show();
            break;
        }
        case StatusMode::Idle: {
            // Subtle warm white breathing on W channel (~3s period)
            float breathe = 0.5f * (1.0f + sinf((2.0f * (float)M_PI) * (now % 3000) / 3000.0f));
            uint8_t w = 24 + (uint8_t)(breathe * 32); // 24..56
            strip.setBrightness(28 + (uint8_t)(breathe * 20)); // 28..48
            for (uint16_t i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0, 0, 0, w));
            strip.show();
            break;
        }
        case StatusMode::None:
        default:
            break;
    }
}
