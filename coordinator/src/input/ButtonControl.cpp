#include "ButtonControl.h"
#include "../utils/Logger.h"

ButtonControl::ButtonControl()
    : eventCallback(nullptr)
    , lastButtonState(true)  // Pull-up, so default is HIGH
    , lastReading(true)
    , lastDebounceTime(0) {
}

ButtonControl::~ButtonControl() {
}

bool ButtonControl::begin() {
    // Always use INPUT_PULLUP for active-low button (button connects pin to GND when pressed)
    pinMode(Pins::PAIRING_BUTTON, INPUT_PULLUP);
    activeLow = true;
    
    delay(50); // Let the pin settle
    
    // Initialize state tracking
    lastReading = digitalRead(Pins::PAIRING_BUTTON);
    lastButtonState = lastReading;
    
    Logger::info("Button control initialized on pin %d (active-low with pull-up, initial: %s)", 
                 Pins::PAIRING_BUTTON, lastReading == HIGH ? "HIGH" : "LOW");
    return true;
}

void ButtonControl::loop() {
    uint32_t now = millis();
    
    // Read the button state
    bool reading = digitalRead(Pins::PAIRING_BUTTON);
    
    // If the reading changed from last time, reset the debounce timer
    if (reading != lastReading) {
        lastDebounceTime = now;
        lastReading = reading;
    }
    
    // If enough time has passed since the last change, accept the reading
    if ((now - lastDebounceTime) > DEBOUNCE_MS) {
        // If the button state has actually changed from the last stable state
        if (reading != lastButtonState) {
            lastButtonState = reading;
            bool pressed = activeLow ? !reading : reading;
            
            Logger::info("Button %s", pressed ? "PRESSED" : "RELEASED");
            handleButtonChange(pressed);
        }
    }
}

void ButtonControl::setEventCallback(std::function<void(const String& buttonId, bool pressed)> callback) {
    eventCallback = callback;
}

void ButtonControl::handleButtonChange(bool pressed) {
    if (eventCallback) {
        eventCallback("PAIR", pressed);
    }
}
