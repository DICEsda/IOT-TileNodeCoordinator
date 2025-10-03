#pragma once

#include <Arduino.h>
#include <vector>
#include <map>
#include "../Models.h"

class ZoneControl {
public:
    ZoneControl();
    ~ZoneControl();

    bool begin();
    void loop();

    // Zone configuration
    bool addZone(const String& zoneId);
    bool removeZone(const String& zoneId);
    bool addLightToZone(const String& zoneId, const String& lightId);
    bool removeLightFromZone(const String& zoneId, const String& lightId);
    
    // Zone queries
    std::vector<String> getLightsForZone(const String& zoneId) const;
    std::vector<String> getZonesForLight(const String& lightId) const;
    bool isLightActive(const String& lightId) const;
    
    // Light state tracking
    void updateLightState(const String& lightId, bool active);

private:
    std::map<String, std::vector<String>> zoneToLights;  // zoneId -> [lightId]
    std::map<String, std::vector<String>> lightToZones;  // lightId -> [zoneId]
    std::map<String, bool> lightStates;  // lightId -> active
    
    void loadFromStorage();
    void saveToStorage();
};
