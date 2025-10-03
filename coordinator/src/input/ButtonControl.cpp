#include "ButtonControl.h"
#include "../utils/Logger.h"

ButtonControl::ButtonControl()
    : eventCallback(nullptr)
    , lastButtonState(true)  // Pull-up, so default is HIGH
    , lastDebounceTime(0) {
}

ButtonControl::~ButtonControl() {
}

bool ButtonControl::begin() {
    pinMode(Pins::PAIRING_BUTTON, INPUT_PULLUP);
    Logger::info("Button control initialized");
    return true;
}

void ButtonControl::loop() {
    uint32_t now = millis();
    
    // Read the button state
    bool reading = digitalRead(PAIRING_BUTTON_PIN);
    
    // If the button state changed, reset the debounce timer
    if (reading != lastButtonState) {
        lastDebounceTime = now;
    }
    
    // If enough time has passed, check if the button state has really changed
    if ((now - lastDebounceTime) > DEBOUNCE_MS) {
        if (reading != lastButtonState) {
            handleButtonChange(!reading);  // Invert because of pull-up
        }
    }
    
    lastButtonState = reading;
}

void ButtonControl::setEventCallback(void (*callback)(const String& buttonId, bool pressed)) {
    eventCallback = callback;
}

void ButtonControl::handleButtonChange(bool pressed) {
    if (eventCallback) {
        eventCallback("PAIR", pressed);
        Logger::info("Button event: pressed=%d", pressed);
    }
}
