#include "LedController.h"
#include <math.h>

LedController::LedController(uint16_t numPixelsPerStrip)
    : strip1(numPixelsPerStrip, Pins::LED_DATA_1, NEO_GRBW + NEO_KHZ800),
      strip2(numPixelsPerStrip, Pins::LED_DATA_2, NEO_GRBW + NEO_KHZ800),
      strip3(numPixelsPerStrip, Pins::LED_DATA_3, NEO_GRBW + NEO_KHZ800),
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
    strip1.begin();
    strip2.begin();
    strip3.begin();
    strip1.show(); // Initialize all pixels to 'off'
    strip2.show();
    strip3.show();
}

void LedController::setBrightness(uint8_t brightness, uint16_t fadeMs) {
    // Apply global cap (50%)
    if (brightness > BRIGHTNESS_CAP) brightness = BRIGHTNESS_CAP;
    targetBrightness = brightness;
    if (fadeMs > 0) {
        fadeStartTime = millis();
        fadeDuration = fadeMs;
        fading = true;
    } else {
        currentBrightness = brightness;
        strip1.setBrightness(currentBrightness);
        strip2.setBrightness(currentBrightness);
        strip3.setBrightness(currentBrightness);
        strip1.show();
        strip2.show();
        strip3.show();
    }
}

void LedController::setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint16_t fadeMs) {
    targetColor = strip1.Color(r, g, b, w);
    if (fadeMs > 0) {
        fadeStartTime = millis();
        fadeDuration = fadeMs;
        fading = true;
    } else {
        currentColor = targetColor;
        for (uint16_t i = 0; i < strip1.numPixels(); i++) {
            strip1.setPixelColor(i, currentColor);
            strip2.setPixelColor(i, currentColor);
            strip3.setPixelColor(i, currentColor);
        }
        strip1.show();
        strip2.show();
        strip3.show();
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
        if (currentBrightness > BRIGHTNESS_CAP) currentBrightness = BRIGHTNESS_CAP;
        currentColor = targetColor;
        strip1.setBrightness(currentBrightness);
        strip2.setBrightness(currentBrightness);
        strip3.setBrightness(currentBrightness);
        for (uint16_t i = 0; i < strip1.numPixels(); i++) {
            strip1.setPixelColor(i, currentColor);
            strip2.setPixelColor(i, currentColor);
            strip3.setPixelColor(i, currentColor);
        }
        strip1.show();
        strip2.show();
        strip3.show();
        fading = false;
        return;
    }

    // Calculate progress (0.0 to 1.0)
    float progress = static_cast<float>(elapsed) / fadeDuration;

    // Interpolate brightness
    if (currentBrightness != targetBrightness) {
        uint8_t newBrightness = currentBrightness + 
            (targetBrightness - currentBrightness) * progress;
        if (newBrightness > BRIGHTNESS_CAP) newBrightness = BRIGHTNESS_CAP;
        strip1.setBrightness(newBrightness);
        strip2.setBrightness(newBrightness);
        strip3.setBrightness(newBrightness);
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

        uint32_t interpolatedColor = strip1.Color(r, g, b, w);
        for (uint16_t i = 0; i < strip1.numPixels(); i++) {
            strip1.setPixelColor(i, interpolatedColor);
            strip2.setPixelColor(i, interpolatedColor);
            strip3.setPixelColor(i, interpolatedColor);
        }
    }

    strip1.show();
    strip2.show();
    strip3.show();
}

void LedController::clear() {
    strip1.clear();
    strip2.clear();
    strip3.clear();
    strip1.show();
    strip2.show();
    strip3.show();
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

// Helper to apply same color/brightness to all strips
void applyToAllStrips(Adafruit_NeoPixel& s1, Adafruit_NeoPixel& s2, Adafruit_NeoPixel& s3, 
                      uint8_t brightness, uint32_t color) {
    s1.setBrightness(brightness);
    s2.setBrightness(brightness);
    s3.setBrightness(brightness);
    for (uint16_t i = 0; i < s1.numPixels(); i++) {
        s1.setPixelColor(i, color);
        s2.setPixelColor(i, color);
        s3.setPixelColor(i, color);
    }
    s1.show();
    s2.show();
    s3.show();
}

void LedController::runStatusAnimation() {
    uint32_t now = millis();
    // Throttle updates to ~120 FPS to improve smoothness while avoiding overdraw
    if (now - lastAnimMs < 8) {
        return;
    }
    switch (status) {
        case StatusMode::Pairing: {
            // Steady, predictable blue breathing for pairing mode (slower, calmer)
            const uint32_t cycleMs = 2000; // Slower 2-second cycle
            float phase = (float)(now % cycleMs) / (float)cycleMs; // 0..1
            // Use smoother sine wave for calmer breathing
            float breathe = 0.5f * (1.0f + sinf(2.0f * (float)M_PI * phase));
            
            // More stable brightness range: 50..120 instead of 40..180
            uint8_t br = (uint8_t)(50 + breathe * 70.0f); // 50..120
            // Consistent blue color (don't vary color, only brightness)
            uint8_t blue = 200; // Fixed blue intensity
            
            // Apply cap
            if (br > BRIGHTNESS_CAP) br = BRIGHTNESS_CAP;
            applyToAllStrips(strip1, strip2, strip3, br, strip1.Color(0, 0, blue, 0));
            break;
        }
        case StatusMode::OTA: {
            // Chasing blue dot
            uint16_t N = strip1.numPixels();
            uint16_t idx = (now / 120) % (N ? N : 1);
            strip1.clear();
            strip2.clear();
            strip3.clear();
            for (uint16_t i = 0; i < N; ++i) {
                uint8_t dim = (i == idx) ? 100 : 10;
                uint32_t color = strip1.Color(0, 0, dim, 0);
                strip1.setPixelColor(i, color);
                strip2.setPixelColor(i, color);
                strip3.setPixelColor(i, color);
            }
            strip1.setBrightness(60);
            strip2.setBrightness(60);
            strip3.setBrightness(60);
            strip1.show();
            strip2.show();
            strip3.show();
            break;
        }
        case StatusMode::Error: {
            // Double red flash every 1.2s
            uint16_t phase = now % 1200;
            bool on = (phase < 120) || (phase > 240 && phase < 360);
            uint8_t br = on ? (uint8_t)min<uint16_t>(76, BRIGHTNESS_CAP) : 0;
            uint32_t c = strip1.Color(180, 10, 10, 0);
            applyToAllStrips(strip1, strip2, strip3, br, c);
            break;
        }
        case StatusMode::Idle: {
            // Idle = off (avoid confusion when coordinator is off)
            strip1.clear();
            strip2.clear();
            strip3.clear();
            strip1.setBrightness(0);
            strip2.setBrightness(0);
            strip3.setBrightness(0);
            strip1.show();
            strip2.show();
            strip3.show();
            break;
        }
        case StatusMode::Connected: {
            // Solid green when connected (no animation)
            uint16_t N = strip1.numPixels();
            if (N == 0) break;
            
            // Solid dim green at 30% brightness
            uint8_t br = min<uint16_t>(80, BRIGHTNESS_CAP);
            uint32_t color = strip1.Color(0, 180, 0, 0); // Green
            applyToAllStrips(strip1, strip2, strip3, br, color);
            break;
        }
        case StatusMode::Wave: {
            // Coordinated test wave: sequential brightness wave across 4 LEDs
            uint16_t N = strip1.numPixels();
            if (N == 0) break;
            uint32_t period = fadeDuration > 0 ? fadeDuration : 1200;
            float phase = (float)((now - fadeStartTime) % period) / (float)period; // 0..1
            float pos = phase * (float)N;
            strip1.clear();
            strip2.clear();
            strip3.clear();
            uint8_t br = min<uint16_t>(200, BRIGHTNESS_CAP);
            strip1.setBrightness(br);
            strip2.setBrightness(br);
            strip3.setBrightness(br);
            for (uint16_t i = 0; i < N; ++i) {
                float dist = fabsf(pos - (float)i);
                if (dist > N/2) dist = N - dist; // wrap minimal distance
                uint8_t w = 0;
                if (dist < 0.5f) w = 255;
                else if (dist < 1.5f) w = (uint8_t)(150.0f * (1.0f - (dist - 0.5f)));
                else if (dist < 2.5f) w = (uint8_t)(60.0f * (1.0f - (dist - 1.5f)));
                uint32_t color = strip1.Color(0, 0, 0, w);
                if (w > 0) {
                    strip1.setPixelColor(i, color);
                    strip2.setPixelColor(i, color);
                    strip3.setPixelColor(i, color);
                }
            }
            strip1.show();
            strip2.show();
            strip3.show();
            break;
        }
        case StatusMode::None:
        default:
            break;
    }
    lastAnimMs = now;
}
