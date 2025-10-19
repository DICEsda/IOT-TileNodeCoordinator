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
    
    // Event callback
    void setEventCallback(std::function<void(const String& buttonId, bool pressed)> callback);

private:
    static constexpr uint32_t DEBOUNCE_MS = 50;
    
    std::function<void(const String& buttonId, bool pressed)> eventCallback;
    bool lastButtonState;      // The last confirmed stable state
    bool lastReading;          // The last raw reading
    uint32_t lastDebounceTime;
    bool activeLow = true; // whether pressed reads LOW (typical for pull-up wiring)
    
    void handleButtonChange(bool pressed);
};
