#include "SmartTileCoordinator.h"

#include "managers/EspNowManager.h"
#include "managers/MqttManager.h"
#include "managers/MmWaveManager.h"
#include "managers/NodeManager.h"
#include "managers/ZoneManager.h"
#include "managers/ButtonManager.h"
#include "managers/ThermalManager.h"

SmartTileCoordinator::SmartTileCoordinator()
	: espNow(nullptr)
	, mqtt(nullptr)
	, mmwave(nullptr)
	, nodes(nullptr)
	, zones(nullptr)
	, buttons(nullptr)
	, thermal(nullptr) {}

SmartTileCoordinator::~SmartTileCoordinator() {
	delete espNow;
	delete mqtt;
	delete mmwave;
	delete nodes;
	delete zones;
	delete buttons;
	delete thermal;
}

bool SmartTileCoordinator::begin() {
	Serial.begin(115200);
	delay(500);

	espNow = new EspNowManager();
	mqtt = new MqttManager();
	mmwave = new MmWaveManager();
	nodes = new NodeManager();
	zones = new ZoneManager();
	buttons = new ButtonManager();
	thermal = new ThermalManager();

	// Minimal init paths
	espNow->begin();
	mqtt->begin();
	mmwave->begin();
	nodes->begin();
	zones->begin();
	buttons->begin();
	thermal->begin();

	return true;
}

void SmartTileCoordinator::loop() {
	espNow->loop();
	mqtt->loop();
	mmwave->loop();
	nodes->loop();
	zones->loop();
	buttons->loop();
	thermal->loop();
}




