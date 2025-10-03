#pragma once

#include <Arduino.h>
#include "../config/PinConfig.h"

class ButtonInput {
public:
    using PressCallback = std::function<void()>;
    using LongPressCallback = std::function<void()>;

    ButtonInput();
    void begin(uint8_t pin = Pins::BUTTON);
    void loop();

    void onPress(PressCallback cb);
    void onLongPress(LongPressCallback cb, uint32_t longPressMs = 2000);

    bool isPressed() const { return stableState; }

private:
    static constexpr uint32_t DEBOUNCE_MS = 40;

    uint8_t pin;
    bool lastRaw;
    bool stableState; // true when physically pressed
    uint32_t lastChangeMs;
    uint32_t pressStartMs;
    bool longReported;
    uint32_t longPressThresholdMs;

    PressCallback pressCb;
    LongPressCallback longPressCb;
};
