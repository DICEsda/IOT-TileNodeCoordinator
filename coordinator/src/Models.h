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
    float confidence; // 0.0–1.0 heuristic confidence (fraction of valid targets)
    struct MmWaveTarget {
        uint8_t id;           // 1-based target id
        bool valid;           // target slot valid
        int16_t x_mm;         // X position (mm)
        int16_t y_mm;         // Y position (mm)
        uint16_t distance_mm; // radial distance (mm)
        int16_t speed_cm_s;   // radial speed (cm/s)
        int16_t resolution_mm;// sensor reported resolution granularity
        float vx_m_s;         // estimated X velocity (m/s)
        float vy_m_s;         // estimated Y velocity (m/s)
    };
    std::vector<MmWaveTarget> targets; // up to sensor-specific max (LD2450: 3–4)
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

struct CoordinatorSensorSnapshot {
    float lightLux = 0.0f;
    float tempC = 0.0f;
    bool mmWavePresence = false;
    float mmWaveConfidence = 0.0f;
    bool mmWaveOnline = false;
    int16_t wifiRssi = -127;
    bool wifiConnected = false;
    uint32_t timestampMs = 0;
};




