#pragma once

#include <Arduino.h>
#include <functional>
#include "../config/PinConfig.h"

class ButtonControl {
public:
    ButtonControl();
    ~ButtonControl();

    bool begin();
    void loop();
    
    // Event callbacks
    void setEventCallback(std::function<void(const String& buttonId, bool pressed)> callback);
    void setLongPressCallback(std::function<void()> callback); // triggered after 4s hold
    void setVeryLongPressCallback(std::function<void()> callback); // triggered after 10s hold

private:
    static constexpr uint32_t DEBOUNCE_MS = 50;
    static constexpr uint32_t LONG_PRESS_MS = 4000;
    static constexpr uint32_t VERY_LONG_PRESS_MS = 10000;
    
    std::function<void(const String& buttonId, bool pressed)> eventCallback;
    std::function<void()> longPressCallback;
    std::function<void()> veryLongPressCallback;
    bool lastButtonState;      // The last confirmed stable state
    bool lastReading;          // The last raw reading
    uint32_t lastDebounceTime;
    uint32_t pressStartTime;
    bool longPressTriggered;
    bool veryLongPressTriggered;
    bool activeLow = true; // whether pressed reads LOW (typical for pull-up wiring)
    
    void handleButtonChange(bool pressed);
};
