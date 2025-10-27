#include "ButtonControl.h"
#include "../utils/Logger.h"

ButtonControl::ButtonControl()
    : eventCallback(nullptr)
    , longPressCallback(nullptr)
    , lastButtonState(true)  // Pull-up, so default is HIGH
    , lastReading(true)
    , lastDebounceTime(0)
    , pressStartTime(0)
    , longPressTriggered(false) {
}

ButtonControl::~ButtonControl() {
}

bool ButtonControl::begin() {
    // TTP223 capacitive touch sensor outputs HIGH when touched (active-high)
    pinMode(Pins::PAIRING_BUTTON, INPUT);
    activeLow = false; // TTP223 is active-high
    
    delay(50); // Let the pin settle
    
    // Initialize state tracking
    lastReading = digitalRead(Pins::PAIRING_BUTTON);
    lastButtonState = lastReading;
    
    Logger::info("TTP223 touch sensor initialized on pin %d (active-high, initial: %s)", 
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
            
            if (pressed) {
                pressStartTime = now;
                longPressTriggered = false;
            }
            
            Logger::info("Touch %s", pressed ? "DETECTED" : "RELEASED");
            handleButtonChange(pressed);
        }
    }
    
    // Check for long press (4s hold)
    bool pressed = activeLow ? !lastButtonState : lastButtonState;
    if (pressed && !longPressTriggered && pressStartTime > 0) {
        if ((now - pressStartTime) >= LONG_PRESS_MS) {
            longPressTriggered = true;
            Logger::info("Touch LONG PRESS (4s)");
            if (longPressCallback) {
                longPressCallback();
            }
        }
    }
}

void ButtonControl::setEventCallback(std::function<void(const String& buttonId, bool pressed)> callback) {
    eventCallback = callback;
}

void ButtonControl::setLongPressCallback(std::function<void()> callback) {
    longPressCallback = callback;
}

void ButtonControl::handleButtonChange(bool pressed) {
    if (eventCallback) {
        eventCallback("PAIR", pressed);
    }
}
