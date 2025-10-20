#pragma once

#include <Arduino.h>

class EspNowManager;
class MqttManager;
class MmWaveManager;
class NodeManager;
class ZoneManager;
class ButtonManager;
class ThermalManager;

class SmartTileCoordinator {
public:
	SmartTileCoordinator();
	~SmartTileCoordinator();
	bool begin();
	void loop();
	void setup();

private:
	EspNowManager* espNow;
	MqttManager* mqtt;
	MmWaveManager* mmwave;
	NodeManager* nodes;
	ZoneManager* zones;
	ButtonManager* buttons;
	ThermalManager* thermal;
};




