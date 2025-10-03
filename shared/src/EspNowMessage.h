#ifndef ESP_NOW_MESSAGE_H
#define ESP_NOW_MESSAGE_H

#include <Arduino.h>
#include <ArduinoJson.h>

// Message types with clear intent
enum class MessageType {
    JOIN_REQUEST,        // Node requesting to join network
    JOIN_ACCEPT,         // Coordinator accepting node
    SET_LIGHT,          // Light control command
    NODE_STATUS,        // Node reporting status (health, sensors)
    THERMAL_ALERT,      // Temperature threshold exceeded
    ERROR,              // Error condition
    ACK,                // Command acknowledgment
    KEEP_ALIVE          // Connection maintenance
};

// Base message structure
struct EspNowMessage {
    MessageType type;
    String msg_id;      // Unique message identifier
    uint32_t timestamp; // Message timestamp
    
    virtual ~EspNowMessage() = default;
    virtual String toJson() const = 0;
    virtual bool fromJson(const String& json) = 0;
    
protected:
    // Helper for JSON serialization
    void serializeBase(JsonDocument& doc) const {
        doc["type"] = static_cast<uint8_t>(type);
        doc["msg_id"] = msg_id;
        doc["timestamp"] = timestamp;
    }
    
    // Helper for JSON deserialization
    bool deserializeBase(const JsonDocument& doc) {
        type = static_cast<MessageType>(doc["type"].as<uint8_t>());
        msg_id = doc["msg_id"].as<String>();
        timestamp = doc["timestamp"].as<uint32_t>();
        return true;
    }
};

// Join request message with capability reporting
struct JoinRequestMessage : public EspNowMessage {
    String mac;            // Node MAC address
    String fw_version;     // Firmware version
    String hw_version;     // Hardware version
    
    struct Capabilities {
        bool rgb_led;      // SK6812B support
        bool temp_sensor;  // Temperature sensor
        bool deep_sleep;   // Deep sleep support
        uint8_t led_count; // Number of LEDs
    } caps;
    
    String token;          // Security token
    
    JoinRequestMessage();
    String toJson() const override;
    bool fromJson(const String& json) override;
};

// Join accept message with configuration
struct JoinAcceptMessage : public EspNowMessage {
    String node_id;        // Assigned node ID
    String light_id;       // Assigned light ID
    String encryption_key; // Link encryption key
    
    struct Config {
        uint16_t keep_alive_interval;    // ms
        uint16_t status_report_interval; // ms
        float temp_derate_start;         // °C
        float temp_derate_max;           // °C
        uint8_t min_brightness;          // %
    } config;
        int rx_window_ms;
        int rx_period_ms;
    } cfg;
    
    JoinAcceptMessage();
    String toJson() const override;
    bool fromJson(const String& json) override;
};

// Set light command message
struct SetLightMessage : public EspNowMessage {
    String light_id;
    uint8_t value;
    uint16_t fade_ms;
    String reason;
    uint16_t ttl_ms;
    
    SetLightMessage();
    String toJson() const override;
    bool fromJson(const String& json) override;
};

// Node status message
struct NodeStatusMessage : public EspNowMessage {
    String node_id;
    String light_id;
    float temp_c;
    uint8_t duty;
    String fw;
    uint16_t vbat_mv;
    
    NodeStatusMessage();
    String toJson() const override;
    bool fromJson(const String& json) override;
};

// Error message
struct ErrorMessage : public EspNowMessage {
    String node_id;
    String code;
    String info;
    
    ErrorMessage();
    String toJson() const override;
    bool fromJson(const String& json) override;
};

// Acknowledgment message
struct AckMessage : public EspNowMessage {
    AckMessage();
    String toJson() const override;
    bool fromJson(const String& json) override;
};

// Message factory
class MessageFactory {
public:
    static EspNowMessage* createMessage(const String& json);
    static MessageType getMessageType(const String& json);
};

#endif // ESP_NOW_MESSAGE_H

