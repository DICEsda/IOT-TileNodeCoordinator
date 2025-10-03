#include <esp_sleep.h>
#include "PowerManager.h"

PowerManager::PowerManager()
	: lastRxWindow(0)
	, rxWindowMs(20)
	, rxPeriodMs(100) {}

void PowerManager::configure(uint16_t windowMs, uint16_t periodMs) {
	rxWindowMs = windowMs;
	rxPeriodMs = periodMs;
}

void PowerManager::enterLightSleepIfIdle(bool isIdle) {
	if (!isIdle) return;
	if (isRxWindowActive()) return;
	esp_sleep_enable_timer_wakeup((uint64_t)rxPeriodMs * 1000ULL);
	esp_light_sleep_start();
	lastRxWindow = millis();
}

bool PowerManager::isRxWindowActive() const {
	return (millis() - lastRxWindow) < rxWindowMs;
}

void PowerManager::markRxWindow() {
	lastRxWindow = millis();
}


