#include "ButtonControl.h"
#include "../utils/Logger.h"

ButtonControl::ButtonControl()
    : eventCallback(nullptr)
    , longPressCallback(nullptr)
    , veryLongPressCallback(nullptr)
    , lastButtonState(true)  // Pull-up, so default is HIGH
    , lastReading(true)
    , lastDebounceTime(0)
    , pressStartTime(0)
    , longPressTriggered(false)
    , veryLongPressTriggered(false) {
}

ButtonControl::~ButtonControl() {
}

bool ButtonControl::begin() {
    // Physical push button with internal pull-up; pressed = LOW (active-low)
    pinMode(Pins::PAIRING_BUTTON, INPUT_PULLUP);
    activeLow = true;

    delay(50); // Let the pin settle

    // Initialize state tracking
    lastReading = digitalRead(Pins::PAIRING_BUTTON);
    lastButtonState = lastReading;

    Logger::info("Button initialized on pin %d (INPUT_PULLUP, pressed=LOW, initial=%s)",
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
                veryLongPressTriggered = false;
            }
            
            Logger::info("Button %s", pressed ? "PRESSED" : "RELEASED");
            handleButtonChange(pressed);
        }
    }
    
    // Check for long press (4s hold) and very long press (10s hold)
    bool pressed = activeLow ? !lastButtonState : lastButtonState;
    if (pressed && pressStartTime > 0) {
        // Check for very long press (10s) first
        if (!veryLongPressTriggered && (now - pressStartTime) >= VERY_LONG_PRESS_MS) {
            veryLongPressTriggered = true;
            Logger::info("Button VERY LONG PRESS (10s) - CLEARING ALL NODES");
            if (veryLongPressCallback) {
                veryLongPressCallback();
            }
        }
        // Check for regular long press (4s)
        else if (!longPressTriggered && (now - pressStartTime) >= LONG_PRESS_MS) {
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

void ButtonControl::setVeryLongPressCallback(std::function<void()> callback) {
    veryLongPressCallback = callback;
}

void ButtonControl::handleButtonChange(bool pressed) {
    if (eventCallback) {
        eventCallback("PAIR", pressed);
    }
}
