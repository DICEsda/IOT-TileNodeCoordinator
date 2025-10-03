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

private:
	EspNowManager* espNow;
	MqttManager* mqtt;
	MmWaveManager* mmwave;
	NodeManager* nodes;
	ZoneManager* zones;
	ButtonManager* buttons;
	ThermalManager* thermal;
};




