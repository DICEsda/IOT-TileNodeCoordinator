#pragma once

#include <Arduino.h>
#include <vector>
#include <map>

struct NodeInfo {
    String nodeId;
    String lightId;
    uint8_t lastDuty;
    uint32_t lastSeenMs;
    float temperature;      // Current temperature in Celsius
    bool isDerated;         // Whether the node is currently derated
    uint8_t derationLevel; // Current deration level (100 = no deration)
};

struct ZoneMapping {
    String zoneId;
    std::vector<String> lightIds;
};

struct MmWaveEvent {
    String sensorId;
    bool presence;
    uint32_t timestampMs;
};

struct ThermalEvent {
    String nodeId;
    float temperature;
    bool isDerated;
    uint8_t derationLevel;
    uint32_t timestampMs;
};

struct SensorData {
    String nodeId;
    float temperature;
    uint32_t timestampMs;
};



