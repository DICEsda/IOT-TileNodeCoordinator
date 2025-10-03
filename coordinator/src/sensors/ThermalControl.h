#pragma once

#include <Arduino.h>
#include <map>
#include "../Models.h"

struct NodeThermalData {
    float temperature;
    uint32_t lastUpdateTime;
    bool isDerated;
    uint8_t derationLevel; // 0-100% where 100 means no deration
    float derateStartTemp;
    float derateMaxTemp;
};

class ThermalControl {
public:
    ThermalControl();
    ~ThermalControl();

    bool begin();
    void loop();

    // Node temperature management
    void updateNodeTemperature(const String& nodeId, float temperature);
    bool isNodeDerated(const String& nodeId) const;
    uint8_t getNodeDerationLevel(const String& nodeId) const;
    NodeThermalData getNodeThermalData(const String& nodeId) const;
    
    // Configuration
    void setNodeThermalLimits(const String& nodeId, float derateStartTemp, float derateMaxTemp);
    void setGlobalThermalLimits(float derateStartTemp, float derateMaxTemp);
    
    // Alert handling
    void registerThermalAlertCallback(void (*callback)(const String&, const NodeThermalData&));

private:
    std::map<String, NodeThermalData> nodeTemperatures;
    float globalDerateStartTemp;
    float globalDerateMaxTemp;
    void (*thermalAlertCallback)(const String&, const NodeThermalData&);

    void checkThermalAlert(const String& nodeId, const NodeThermalData& data);
    uint8_t calculateDerationLevel(float temp, float startTemp, float maxTemp);
};
