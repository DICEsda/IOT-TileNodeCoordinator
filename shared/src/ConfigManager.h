#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>

namespace ConfigKeys {
    // Coordinator
    static const char* const PRESENCE_DEBOUNCE_MS = "presence_debounce_ms";
    static const char* const OCCUPANCY_HOLD_MS   = "occupancy_hold_ms";
    static const char* const FADE_IN_MS          = "fade_in_ms";
    static const char* const FADE_OUT_MS         = "fade_out_ms";
    static const char* const PAIRING_WINDOW_S    = "pairing_window_s";

    // Node
    static const char* const NODE_ID             = "node_id";
    static const char* const LIGHT_ID            = "light_id";
    static const char* const LMK                 = "lmk"; // ESP-NOW LMK key
    static const char* const PWM_FREQ_HZ         = "pwm_freq_hz";
    static const char* const PWM_RESOLUTION_BITS = "pwm_res_bits";
    static const char* const TELEMETRY_INTERVAL_S= "telemetry_s";
    static const char* const RX_WINDOW_MS        = "rx_window_ms";
    static const char* const RX_PERIOD_MS        = "rx_period_ms";
    static const char* const DERATE_START_C      = "derate_start_c";
    static const char* const DERATE_MIN_DUTY_PCT = "derate_min_duty_pct";
    static const char* const RETRY_COUNT         = "retry_count";
    static const char* const CMD_TTL_MS          = "cmd_ttl_ms";
}

namespace Defaults {
    // Coordinator defaults (from PRD)
    static constexpr int PRESENCE_DEBOUNCE_MS = 150;
    static constexpr int OCCUPANCY_HOLD_MS   = 5000;
    static constexpr int FADE_IN_MS          = 150;
    static constexpr int FADE_OUT_MS         = 1000;
    static constexpr int PAIRING_WINDOW_S    = 120;

    // Node defaults (from PRD)
    static constexpr int PWM_FREQ_HZ         = 1000;
    static constexpr int PWM_RESOLUTION_BITS = 12;
    static constexpr int TELEMETRY_INTERVAL_S= 1;
    static constexpr int RX_WINDOW_MS        = 20;
    static constexpr int RX_PERIOD_MS        = 100;
    static constexpr float DERATE_START_C    = 70.0f;
    static constexpr int DERATE_MIN_DUTY_PCT = 30;
    static constexpr int RETRY_COUNT         = 3;
    static constexpr int CMD_TTL_MS          = 1500;
}

class ConfigManager {
public:
    explicit ConfigManager(const String& ns);
    ~ConfigManager();

    bool begin();
    void end();

    String getString(const String& key, const String& defaultValue = "");
    bool setString(const String& key, const String& value);

    int getInt(const String& key, int defaultValue = 0);
    bool setInt(const String& key, int value);

    float getFloat(const String& key, float defaultValue = 0.0f);
    bool setFloat(const String& key, float value);

    bool getBool(const String& key, bool defaultValue = false);
    bool setBool(const String& key, bool value);

    JsonObject getJson(const String& key);
    bool setJson(const String& key, const JsonObject& obj);

    bool exists(const String& key);
    bool remove(const String& key);
    void clear();

    bool factoryReset();
    bool validateConfig();
    void loadDefaults();

private:
    Preferences preferences;
    String namespace_name;
    bool initialized;
};
