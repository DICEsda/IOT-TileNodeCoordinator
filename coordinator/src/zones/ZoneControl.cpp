#include "ZoneControl.h"
#include "../utils/Logger.h"
#include <Preferences.h>

ZoneControl::ZoneControl() {
}

ZoneControl::~ZoneControl() {
}

bool ZoneControl::begin() {
    loadFromStorage();
    Logger::info("Zone control initialized with %d zones", zoneToLights.size());
    return true;
}

void ZoneControl::loop() {
    // No periodic tasks needed
}

bool ZoneControl::addZone(const String& zoneId) {
    if (zoneToLights.find(zoneId) != zoneToLights.end()) {
        return false; // Zone already exists
    }
    
    zoneToLights[zoneId] = std::vector<String>();
    saveToStorage();
    Logger::info("Added new zone: %s", zoneId.c_str());
    return true;
}

bool ZoneControl::removeZone(const String& zoneId) {
    auto it = zoneToLights.find(zoneId);
    if (it == zoneToLights.end()) {
        return false;
    }
    
    // Remove zone from all lights' zone lists
    for (const String& lightId : it->second) {
        auto& zones = lightToZones[lightId];
        zones.erase(std::remove(zones.begin(), zones.end(), zoneId), zones.end());
        if (zones.empty()) {
            lightToZones.erase(lightId);
            lightStates.erase(lightId);
        }
    }
    
    zoneToLights.erase(it);
    saveToStorage();
    Logger::info("Removed zone: %s", zoneId.c_str());
    return true;
}

bool ZoneControl::addLightToZone(const String& zoneId, const String& lightId) {
    auto it = zoneToLights.find(zoneId);
    if (it == zoneToLights.end()) {
        return false;
    }
    
    // Check if light is already in zone
    auto& lights = it->second;
    if (std::find(lights.begin(), lights.end(), lightId) != lights.end()) {
        return false;
    }
    
    // Add light to zone
    lights.push_back(lightId);
    
    // Add zone to light's zone list
    lightToZones[lightId].push_back(zoneId);
    
    // Initialize light state if not present
    if (lightStates.find(lightId) == lightStates.end()) {
        lightStates[lightId] = false;
    }
    
    saveToStorage();
    Logger::info("Added light %s to zone %s", lightId.c_str(), zoneId.c_str());
    return true;
}

bool ZoneControl::removeLightFromZone(const String& zoneId, const String& lightId) {
    auto zoneIt = zoneToLights.find(zoneId);
    if (zoneIt == zoneToLights.end()) {
        return false;
    }
    
    // Remove light from zone
    auto& lights = zoneIt->second;
    lights.erase(std::remove(lights.begin(), lights.end(), lightId), lights.end());
    
    // Remove zone from light's zone list
    auto lightIt = lightToZones.find(lightId);
    if (lightIt != lightToZones.end()) {
        auto& zones = lightIt->second;
        zones.erase(std::remove(zones.begin(), zones.end(), zoneId), zones.end());
        if (zones.empty()) {
            lightToZones.erase(lightIt);
            lightStates.erase(lightId);
        }
    }
    
    saveToStorage();
    Logger::info("Removed light %s from zone %s", lightId.c_str(), zoneId.c_str());
    return true;
}

std::vector<String> ZoneControl::getLightsForZone(const String& zoneId) const {
    auto it = zoneToLights.find(zoneId);
    return it != zoneToLights.end() ? it->second : std::vector<String>();
}

std::vector<String> ZoneControl::getZonesForLight(const String& lightId) const {
    auto it = lightToZones.find(lightId);
    return it != lightToZones.end() ? it->second : std::vector<String>();
}

bool ZoneControl::isLightActive(const String& lightId) const {
    auto it = lightStates.find(lightId);
    return it != lightStates.end() ? it->second : false;
}

void ZoneControl::updateLightState(const String& lightId, bool active) {
    if (lightToZones.find(lightId) != lightToZones.end()) {
        lightStates[lightId] = active;
    }
}

void ZoneControl::loadFromStorage() {
    Preferences prefs;
    if (!prefs.begin("zones", true)) {
        Logger::warn("Failed to load zone data (NVS not initialized yet - this is normal on first boot)");
        return;
    }
    
    size_t zoneCount = prefs.getUInt("count", 0);
    for (size_t i = 0; i < zoneCount; i++) {
        String zoneKey = "z" + String(i);
        String zoneId = prefs.getString((zoneKey + "_id").c_str());
        size_t lightCount = prefs.getUInt((zoneKey + "_count").c_str(), 0);
        
        std::vector<String> lights;
        for (size_t j = 0; j < lightCount; j++) {
            String lightId = prefs.getString((zoneKey + "_l" + String(j)).c_str());
            lights.push_back(lightId);
            lightToZones[lightId].push_back(zoneId);
            lightStates[lightId] = false;
        }
        
        zoneToLights[zoneId] = lights;
    }
    
    prefs.end();
}

void ZoneControl::saveToStorage() {
    Preferences prefs;
    if (!prefs.begin("zones", false)) {
        Logger::error("Failed to save zone data");
        return;
    }
    
    prefs.clear();
    prefs.putUInt("count", zoneToLights.size());
    
    size_t zoneIndex = 0;
    for (const auto& zonePair : zoneToLights) {
        String zoneKey = "z" + String(zoneIndex);
        prefs.putString((zoneKey + "_id").c_str(), zonePair.first.c_str());
        prefs.putUInt((zoneKey + "_count").c_str(), zonePair.second.size());
        
        size_t lightIndex = 0;
        for (const String& lightId : zonePair.second) {
            prefs.putString((zoneKey + "_l" + String(lightIndex)).c_str(), lightId.c_str());
            lightIndex++;
        }
        
        zoneIndex++;
    }
    
    prefs.end();
}
