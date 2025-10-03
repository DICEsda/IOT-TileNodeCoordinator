#include "ButtonInput.h"

ButtonInput::ButtonInput()
	: pin(Pins::BUTTON), lastRaw(false), stableState(false),
	  lastChangeMs(0), pressStartMs(0), longReported(false),
	  longPressThresholdMs(2000) {}

void ButtonInput::begin(uint8_t pinIn) {
	pin = pinIn;
	pinMode(pin, INPUT_PULLUP);
	lastRaw = !digitalRead(pin);
	stableState = lastRaw;
	lastChangeMs = millis();
}

void ButtonInput::onPress(PressCallback cb) { pressCb = cb; }

void ButtonInput::onLongPress(LongPressCallback cb, uint32_t longPressMs) {
	longPressCb = cb;
	longPressThresholdMs = longPressMs;
}

void ButtonInput::loop() {
	bool raw = !digitalRead(pin); // active low
	uint32_t now = millis();

	if (raw != lastRaw) {
		lastChangeMs = now;
		lastRaw = raw;
	}

	// Debounce
	if ((now - lastChangeMs) > DEBOUNCE_MS) {
		if (stableState != raw) {
			stableState = raw;
			if (stableState) {
				pressStartMs = now;
				longReported = false;
				if (pressCb) pressCb();
			} else {
				// release
				pressStartMs = 0;
				longReported = false;
			}
		}
	}

	// Long press detection while still pressed
	if (stableState && !longReported && longPressCb && pressStartMs) {
		if ((now - pressStartMs) >= longPressThresholdMs) {
			longReported = true;
			longPressCb();
		}
	}
}


