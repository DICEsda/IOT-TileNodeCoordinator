// Minimal, safe stub implementation so coordinator builds.
#include "ThermalManager.h"

ThermalManager::ThermalManager()
    : globalDerateStartTemp(70.0f)
    , globalDerateMaxTemp(85.0f)
    , thermalAlertCallback(nullptr) {}

ThermalManager::~ThermalManager() {}

bool ThermalManager::begin() { return true; }

void ThermalManager::loop() {
    // no-op in stub
}

void ThermalManager::updateNodeTemperature(const String& nodeId, float temperature) {
    auto& data = nodeTemperatures[nodeId];
    data.temperature = temperature;
    data.lastUpdateTime = millis();
    data.derationLevel = calculateDerationLevel(temperature, data.derateStartTemp > 0 ? data.derateStartTemp : globalDerateStartTemp, data.derateMaxTemp > 0 ? data.derateMaxTemp : globalDerateMaxTemp);
    data.isDerated = data.derationLevel < 100;
    checkThermalAlert(nodeId, data);
}

bool ThermalManager::isNodeDerated(const String& nodeId) const {
    auto it = nodeTemperatures.find(nodeId);
    return it != nodeTemperatures.end() && it->second.isDerated;
}

uint8_t ThermalManager::getNodeDerationLevel(const String& nodeId) const {
    auto it = nodeTemperatures.find(nodeId);
    return it != nodeTemperatures.end() ? it->second.derationLevel : 100;
}

NodeThermalData ThermalManager::getNodeThermalData(const String& nodeId) const {
    auto it = nodeTemperatures.find(nodeId);
    return it != nodeTemperatures.end() ? it->second : NodeThermalData();
}

void ThermalManager::setNodeThermalLimits(const String& nodeId, float derateStartTemp, float derateMaxTemp) {
    auto& data = nodeTemperatures[nodeId];
    data.derateStartTemp = derateStartTemp;
    data.derateMaxTemp = derateMaxTemp;
}

void ThermalManager::setGlobalThermalLimits(float derateStartTemp, float derateMaxTemp) {
    globalDerateStartTemp = derateStartTemp;
    globalDerateMaxTemp = derateMaxTemp;
}

void ThermalManager::registerThermalAlertCallback(void (*callback)(const String&, const NodeThermalData&)) {
    thermalAlertCallback = callback;
}

void ThermalManager::checkThermalAlert(const String& nodeId, const NodeThermalData& data) {
    if (data.isDerated && thermalAlertCallback) {
        thermalAlertCallback(nodeId, data);
    }
}

uint8_t ThermalManager::calculateDerationLevel(float temp, float startTemp, float maxTemp) {
    if (temp < startTemp) return 100;
    if (temp >= maxTemp) return 30;
    float tempRange = maxTemp - startTemp;
    float progress = (temp - startTemp) / tempRange;
    return (uint8_t) (100 - (70.0f * progress));
}




