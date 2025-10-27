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
    if (status == mode) return; // avoid resetting animation every loop
    status = mode;
    lastAnimMs = 0; // reset animation timer
}

void LedController::startWave(uint16_t periodMs, uint32_t durationMs) {
    // Use status Wave; pack timing into fadeStartTime/fadeDuration to reuse fields
    fadeStartTime = millis();
    fadeDuration = periodMs; // store period here
    // targetBrightness holds remaining duration high byte? keep separate via currentColor as timestamp? Simplify: reuse targetBrightness as unused
    // We'll compute relative time from fadeStartTime and use fadeDuration as period
    setStatus(StatusMode::Wave);
    // Use currentBrightness as a flag if needed (not required)
    // duration is tracked via targetBrightness? Instead we will end wave after 4s externally by caller.
}

void LedController::runStatusAnimation() {
    uint32_t now = millis();
    // Throttle updates to ~120 FPS to improve smoothness while avoiding overdraw
    if (now - lastAnimMs < 8) {
        return;
    }
    switch (status) {
        case StatusMode::Pairing: {
            // Smooth blue breathing for pairing mode
            const uint32_t cycleMs = 1600;
            float phase = (float)(now % cycleMs) / (float)cycleMs; // 0..1
            float breathe = 0.5f * (1.0f + sinf(2.0f * (float)M_PI * phase));
            
            uint8_t br = (uint8_t)(40 + breathe * 140.0f); // 40..180
            uint8_t blue = (uint8_t)(80 + breathe * 175.0f); // 80..255
            
            strip.setBrightness(br);
            uint16_t N = strip.numPixels();
            for (uint16_t i = 0; i < N; ++i) {
                strip.setPixelColor(i, strip.Color(0, 0, blue, 0));
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
            // Idle = off (avoid confusion when coordinator is off)
            strip.clear();
            strip.setBrightness(0);
            strip.show();
            break;
        }
        case StatusMode::Connected: {
            // Professional green trailing wave for 4-LED SK6812B (idle connected)
            uint16_t N = strip.numPixels();
            if (N == 0) break;
            
            uint32_t cycleMs = 2000;
            float phase = (float)(now % cycleMs) / (float)cycleMs; // 0..1
            float pos = phase * (float)N; // float position 0..N
            
            strip.clear();
            strip.setBrightness(140);
            for (uint16_t i = 0; i < N; ++i) {
                float dist = pos - (float)i;
                if (dist < 0) dist += N;
                uint8_t g = 0;
                if (dist < 0.5f) g = 255;
                else if (dist < 1.5f) g = (uint8_t)(120.0f * (1.0f - (dist - 0.5f)));
                else if (dist < 2.5f) g = (uint8_t)(40.0f * (1.0f - (dist - 1.5f)));
                if (g > 0) strip.setPixelColor(i, strip.Color(0, g, 0, 0));
            }
            strip.show();
            break;
        }
        case StatusMode::Wave: {
            // Coordinated test wave: sequential brightness wave across 4 LEDs
            uint16_t N = strip.numPixels();
            if (N == 0) break;
            uint32_t period = fadeDuration > 0 ? fadeDuration : 1200;
            float phase = (float)((now - fadeStartTime) % period) / (float)period; // 0..1
            float pos = phase * (float)N;
            strip.clear();
            strip.setBrightness(200);
            for (uint16_t i = 0; i < N; ++i) {
                float dist = fabsf(pos - (float)i);
                if (dist > N/2) dist = N - dist; // wrap minimal distance
                uint8_t w = 0;
                if (dist < 0.5f) w = 255;
                else if (dist < 1.5f) w = (uint8_t)(150.0f * (1.0f - (dist - 0.5f)));
                else if (dist < 2.5f) w = (uint8_t)(60.0f * (1.0f - (dist - 1.5f)));
                if (w > 0) strip.setPixelColor(i, strip.Color(0, 0, 0, w)); // use white channel for clarity
            }
            strip.show();
            break;
        }
        case StatusMode::None:
        default:
            break;
    }
    lastAnimMs = now;
}
