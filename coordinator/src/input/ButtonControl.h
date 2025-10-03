#pragma once

#include <Arduino.h>

class ButtonControl {
public:
    ButtonControl();
    ~ButtonControl();

    bool begin();
    void loop();
    
    // Event callback
    void setEventCallback(void (*callback)(const String& buttonId, bool pressed));

private:
    #include "../config/PinConfig.h"
    static constexpr uint32_t DEBOUNCE_MS = 50;
    
    void (*eventCallback)(const String& buttonId, bool pressed);
    bool lastButtonState;
    uint32_t lastDebounceTime;
    
    void handleButtonChange(bool pressed);
};
