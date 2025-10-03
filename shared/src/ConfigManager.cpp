#include "ConfigManager.h"

ConfigManager::ConfigManager(const String& ns) : namespace_name(ns), initialized(false) {
}

ConfigManager::~ConfigManager() {
    if (initialized) {
        end();
    }
}

bool ConfigManager::begin() {
    if (initialized) {
        return true;
    }
    
    if (!preferences.begin(namespace_name.c_str(), false)) {
        Serial.printf("Failed to initialize preferences namespace: %s\n", namespace_name.c_str());
        return false;
    }
    
    initialized = true;
    return true;
}

void ConfigManager::end() {
    if (initialized) {
        preferences.end();
        initialized = false;
    }
}

String ConfigManager::getString(const String& key, const String& defaultValue) {
    if (!initialized) return defaultValue;
    
    String value = preferences.getString(key.c_str(), defaultValue.c_str());
    return value;
}

bool ConfigManager::setString(const String& key, const String& value) {
    if (!initialized) return false;
    
    return preferences.putString(key.c_str(), value);
}

int ConfigManager::getInt(const String& key, int defaultValue) {
    if (!initialized) return defaultValue;
    
    return preferences.getInt(key.c_str(), defaultValue);
}

bool ConfigManager::setInt(const String& key, int value) {
    if (!initialized) return false;
    
    return preferences.putInt(key.c_str(), value);
}

float ConfigManager::getFloat(const String& key, float defaultValue) {
    if (!initialized) return defaultValue;
    
    return preferences.getFloat(key.c_str(), defaultValue);
}

bool ConfigManager::setFloat(const String& key, float value) {
    if (!initialized) return false;
    
    return preferences.putFloat(key.c_str(), value);
}

bool ConfigManager::getBool(const String& key, bool defaultValue) {
    if (!initialized) return defaultValue;
    
    return preferences.getBool(key.c_str(), defaultValue);
}

bool ConfigManager::setBool(const String& key, bool value) {
    if (!initialized) return false;
    
    return preferences.putBool(key.c_str(), value);
}

JsonObject ConfigManager::getJson(const String& key) {
    static DynamicJsonDocument doc(1024);
    doc.clear();
    
    if (!initialized) return doc.to<JsonObject>();
    
    String jsonStr = preferences.getString(key.c_str(), "{}");
    DeserializationError error = deserializeJson(doc, jsonStr);
    
    if (error) {
        Serial.printf("Failed to parse JSON for key %s: %s\n", key.c_str(), error.c_str());
        return doc.to<JsonObject>();
    }
    
    return doc.as<JsonObject>();
}

bool ConfigManager::setJson(const String& key, const JsonObject& obj) {
    if (!initialized) return false;
    
    String jsonStr;
    serializeJson(obj, jsonStr);
    
    return preferences.putString(key.c_str(), jsonStr);
}

bool ConfigManager::exists(const String& key) {
    if (!initialized) return false;
    
    return preferences.isKey(key.c_str());
}

bool ConfigManager::remove(const String& key) {
    if (!initialized) return false;
    
    return preferences.remove(key.c_str());
}

void ConfigManager::clear() {
    if (!initialized) return;
    
    preferences.clear();
}

bool ConfigManager::factoryReset() {
    if (!initialized) return false;
    
    preferences.clear();
    loadDefaults();
    return true;
}

bool ConfigManager::validateConfig() {
    if (!initialized) return false;
    
    // Basic validation - check if required keys exist
    // This can be extended based on specific requirements
    
    return true;
}

void ConfigManager::loadDefaults() {
    if (!initialized) return;
    
    // Load coordinator defaults
    if (!exists(ConfigKeys::PRESENCE_DEBOUNCE_MS)) {
        setInt(ConfigKeys::PRESENCE_DEBOUNCE_MS, Defaults::PRESENCE_DEBOUNCE_MS);
    }
    if (!exists(ConfigKeys::OCCUPANCY_HOLD_MS)) {
        setInt(ConfigKeys::OCCUPANCY_HOLD_MS, Defaults::OCCUPANCY_HOLD_MS);
    }
    if (!exists(ConfigKeys::FADE_IN_MS)) {
        setInt(ConfigKeys::FADE_IN_MS, Defaults::FADE_IN_MS);
    }
    if (!exists(ConfigKeys::FADE_OUT_MS)) {
        setInt(ConfigKeys::FADE_OUT_MS, Defaults::FADE_OUT_MS);
    }
    if (!exists(ConfigKeys::PAIRING_WINDOW_S)) {
        setInt(ConfigKeys::PAIRING_WINDOW_S, Defaults::PAIRING_WINDOW_S);
    }
    
    // Load node defaults
    if (!exists(ConfigKeys::PWM_FREQ_HZ)) {
        setInt(ConfigKeys::PWM_FREQ_HZ, Defaults::PWM_FREQ_HZ);
    }
    if (!exists(ConfigKeys::PWM_RESOLUTION_BITS)) {
        setInt(ConfigKeys::PWM_RESOLUTION_BITS, Defaults::PWM_RESOLUTION_BITS);
    }
    if (!exists(ConfigKeys::TELEMETRY_INTERVAL_S)) {
        setInt(ConfigKeys::TELEMETRY_INTERVAL_S, Defaults::TELEMETRY_INTERVAL_S);
    }
    if (!exists(ConfigKeys::RX_WINDOW_MS)) {
        setInt(ConfigKeys::RX_WINDOW_MS, Defaults::RX_WINDOW_MS);
    }
    if (!exists(ConfigKeys::RX_PERIOD_MS)) {
        setInt(ConfigKeys::RX_PERIOD_MS, Defaults::RX_PERIOD_MS);
    }
    if (!exists(ConfigKeys::DERATE_START_C)) {
        setFloat(ConfigKeys::DERATE_START_C, Defaults::DERATE_START_C);
    }
    if (!exists(ConfigKeys::DERATE_MIN_DUTY_PCT)) {
        setInt(ConfigKeys::DERATE_MIN_DUTY_PCT, Defaults::DERATE_MIN_DUTY_PCT);
    }
    if (!exists(ConfigKeys::RETRY_COUNT)) {
        setInt(ConfigKeys::RETRY_COUNT, Defaults::RETRY_COUNT);
    }
    if (!exists(ConfigKeys::CMD_TTL_MS)) {
        setInt(ConfigKeys::CMD_TTL_MS, Defaults::CMD_TTL_MS);
    }
}



