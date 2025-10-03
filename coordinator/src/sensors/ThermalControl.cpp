#include "ThermalControl.h"
#include "../utils/Logger.h"

ThermalControl::ThermalControl()
    : globalDerateStartTemp(70.0f)
    , globalDerateMaxTemp(85.0f)
    , thermalAlertCallback(nullptr) {
}

ThermalControl::~ThermalControl() {
}

bool ThermalControl::begin() {
    Logger::info("Thermal control initialized with global limits: %.1f°C - %.1f°C",
                globalDerateStartTemp, globalDerateMaxTemp);
    return true;
}

void ThermalControl::loop() {
    uint32_t currentTime = millis();
    
    // Check all nodes for stale data or thermal issues
    for (auto& pair : nodeTemperatures) {
        const String& nodeId = pair.first;
        NodeThermalData& data = pair.second;
        
        // Check for stale data (no updates in 60 seconds)
        if (currentTime - data.lastUpdateTime > 60000) {
            Logger::warning("Node %s temperature data is stale", nodeId.c_str());
        }
        
        // Check thermal status
        checkThermalAlert(nodeId, data);
    }
}

void ThermalControl::updateNodeTemperature(const String& nodeId, float temperature) {
    auto& data = nodeTemperatures[nodeId];
    data.temperature = temperature;
    data.lastUpdateTime = millis();
    
    // Calculate deration level
    float startTemp = data.derateStartTemp > 0 ? data.derateStartTemp : globalDerateStartTemp;
    float maxTemp = data.derateMaxTemp > 0 ? data.derateMaxTemp : globalDerateMaxTemp;
    
    data.derationLevel = calculateDerationLevel(temperature, startTemp, maxTemp);
    data.isDerated = data.derationLevel < 100;
    
    // Check for thermal alerts
    checkThermalAlert(nodeId, data);
    
    Logger::info("Node %s temperature: %.1f°C, deration: %d%%", 
                 nodeId.c_str(), temperature, data.derationLevel);
}

bool ThermalControl::isNodeDerated(const String& nodeId) const {
    auto it = nodeTemperatures.find(nodeId);
    return it != nodeTemperatures.end() && it->second.isDerated;
}

uint8_t ThermalControl::getNodeDerationLevel(const String& nodeId) const {
    auto it = nodeTemperatures.find(nodeId);
    return it != nodeTemperatures.end() ? it->second.derationLevel : 100;
}

NodeThermalData ThermalControl::getNodeThermalData(const String& nodeId) const {
    auto it = nodeTemperatures.find(nodeId);
    return it != nodeTemperatures.end() ? it->second : NodeThermalData();
}

void ThermalControl::setNodeThermalLimits(const String& nodeId, float derateStartTemp, float derateMaxTemp) {
    auto& data = nodeTemperatures[nodeId];
    data.derateStartTemp = derateStartTemp;
    data.derateMaxTemp = derateMaxTemp;
    
    Logger::info("Set thermal limits for node %s: %.1f°C - %.1f°C",
                nodeId.c_str(), derateStartTemp, derateMaxTemp);
    
    // Recalculate deration with new limits if we have temperature data
    if (data.lastUpdateTime > 0) {
        updateNodeTemperature(nodeId, data.temperature);
    }
}

void ThermalControl::setGlobalThermalLimits(float derateStartTemp, float derateMaxTemp) {
    globalDerateStartTemp = derateStartTemp;
    globalDerateMaxTemp = derateMaxTemp;
    
    Logger::info("Updated global thermal limits: %.1f°C - %.1f°C",
                derateStartTemp, derateMaxTemp);
    
    // Recalculate deration for all nodes using global limits
    for (const auto& pair : nodeTemperatures) {
        if (pair.second.lastUpdateTime > 0) {
            updateNodeTemperature(pair.first, pair.second.temperature);
        }
    }
}

void ThermalControl::registerThermalAlertCallback(void (*callback)(const String&, const NodeThermalData&)) {
    thermalAlertCallback = callback;
}

void ThermalControl::checkThermalAlert(const String& nodeId, const NodeThermalData& data) {
    if (data.isDerated && thermalAlertCallback) {
        thermalAlertCallback(nodeId, data);
    }
}

uint8_t ThermalControl::calculateDerationLevel(float temp, float startTemp, float maxTemp) {
    if (temp < startTemp) {
        return 100; // No deration
    }
    if (temp >= maxTemp) {
        return 30;  // Maximum deration (30% of original brightness)
    }
    
    // Linear interpolation between full duty and minimum duty
    float tempRange = maxTemp - startTemp;
    float tempProgress = (temp - startTemp) / tempRange;
    float dutyRange = 70.0f; // 100 - 30
    
    return 100 - (dutyRange * tempProgress);
}
