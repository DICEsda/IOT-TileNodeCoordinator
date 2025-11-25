#pragma once

#include <Arduino.h>

/**
 * EspNowLogger - Detailed ESP-NOW communication logging for nodes
 * 
 * Provides structured logging for:
 * - Message transmission (to coordinator)
 * - Message reception (from coordinator)
 * - Join/pairing flow
 * - Command reception
 * - Telemetry submission
 * - Error diagnostics
 * - Link health tracking
 */
namespace EspNowLogger {

// Message types for categorization
enum MessageType {
    JOIN_REQUEST,
    JOIN_ACCEPT,
    SET_LIGHT,
    NODE_STATUS,
    ACK,
    ERROR_MSG,
    UNKNOWN
};

// Statistics tracking
struct Stats {
    uint32_t messagesSent = 0;
    uint32_t messagesReceived = 0;
    uint32_t sendErrors = 0;
    uint32_t parseErrors = 0;
    uint32_t lastSendMs = 0;
    uint32_t lastReceiveMs = 0;
    
    // Message type counters
    uint32_t joinRequestsSent = 0;
    uint32_t joinAcceptsReceived = 0;
    uint32_t statusMessagesSent = 0;
    uint32_t lightCommandsReceived = 0;
    uint32_t acksReceived = 0;
    
    // Link health
    uint32_t lastLinkActivityMs = 0;
    bool isPaired = false;
};

// Global statistics instance
inline Stats& getStats() {
    static Stats stats;
    return stats;
}

// Helper to get message type from JSON
inline MessageType getMessageType(const String& json) {
    if (json.indexOf("\"msg\":\"join_request\"") >= 0) return JOIN_REQUEST;
    if (json.indexOf("\"msg\":\"join_accept\"") >= 0) return JOIN_ACCEPT;
    if (json.indexOf("\"msg\":\"set_light\"") >= 0) return SET_LIGHT;
    if (json.indexOf("\"msg\":\"node_status\"") >= 0) return NODE_STATUS;
    if (json.indexOf("\"msg\":\"ack\"") >= 0) return ACK;
    if (json.indexOf("\"msg\":\"error\"") >= 0) return ERROR_MSG;
    return UNKNOWN;
}

// Helper to get message type name
inline const char* getMessageTypeName(MessageType type) {
    switch (type) {
        case JOIN_REQUEST: return "JoinRequest";
        case JOIN_ACCEPT: return "JoinAccept";
        case SET_LIGHT: return "SetLight";
        case NODE_STATUS: return "NodeStatus";
        case ACK: return "Ack";
        case ERROR_MSG: return "Error";
        case UNKNOWN: return "Unknown";
        default: return "Invalid";
    }
}

// Helper to format MAC address
inline String formatMac(const uint8_t* mac) {
    if (!mac) return "NULL";
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

// Log message send
inline void logSend(const uint8_t* destMac, const String& json, bool success, const char* detail = nullptr) {
    Stats& stats = getStats();
    MessageType type = getMessageType(json);
    
    if (success) {
        stats.messagesSent++;
        stats.lastSendMs = millis();
        stats.lastLinkActivityMs = millis();
        
        // Update type-specific counters
        if (type == JOIN_REQUEST) stats.joinRequestsSent++;
        else if (type == NODE_STATUS) stats.statusMessagesSent++;
        
        // Truncate long payloads
        String displayJson = json;
        if (displayJson.length() > 80) {
            displayJson = displayJson.substring(0, 77) + "...";
        }
        
        Serial.printf("[ESP→] %s | dest=%s | size=%d\n", 
                      getMessageTypeName(type), formatMac(destMac).c_str(), json.length());
        Serial.printf("[ESP→]   %s\n", displayJson.c_str());
        
        if (detail) {
            Serial.printf("[ESP→]   %s\n", detail);
        }
    } else {
        stats.sendErrors++;
        Serial.printf("[ESP→] ✗ Send failed | %s | dest=%s | size=%d\n",
                      getMessageTypeName(type), formatMac(destMac).c_str(), json.length());
        if (detail) {
            Serial.printf("[ESP→]   error: %s\n", detail);
        }
    }
}

// Log message receive
inline void logReceive(const uint8_t* srcMac, const uint8_t* data, uint16_t len, const char* detail = nullptr) {
    Stats& stats = getStats();
    stats.messagesReceived++;
    stats.lastReceiveMs = millis();
    stats.lastLinkActivityMs = millis();
    
    // Try to parse as string for message type detection
    String json;
    if (len > 0 && len < 512) {
        json = String((char*)data, len);
    }
    
    MessageType type = getMessageType(json);
    
    // Update type-specific counters
    if (type == JOIN_ACCEPT) {
        stats.joinAcceptsReceived++;
        stats.isPaired = true;
    } else if (type == SET_LIGHT) {
        stats.lightCommandsReceived++;
    } else if (type == ACK) {
        stats.acksReceived++;
    }
    
    Serial.printf("[ESP←] %s | src=%s | size=%d\n",
                  getMessageTypeName(type), formatMac(srcMac).c_str(), len);
    
    if (!json.isEmpty() && json.length() <= 100) {
        Serial.printf("[ESP←]   %s\n", json.c_str());
    } else if (json.length() > 100) {
        Serial.printf("[ESP←]   %s...\n", json.substring(0, 97).c_str());
    }
    
    if (detail) {
        Serial.printf("[ESP←]   %s\n", detail);
    }
}

// Log pairing status
inline void logPairing(bool entered, const char* reason = nullptr) {
    if (entered) {
        Serial.println("[ESP🔗] ▶ Pairing mode STARTED");
        if (reason) {
            Serial.printf("[ESP🔗]   reason: %s\n", reason);
        }
    } else {
        Serial.println("[ESP🔗] ■ Pairing mode STOPPED");
        if (reason) {
            Serial.printf("[ESP🔗]   reason: %s\n", reason);
        }
    }
}

// Log pairing success
inline void logPairSuccess(const String& nodeId, const String& lightId, const uint8_t* coordMac) {
    Stats& stats = getStats();
    stats.isPaired = true;
    
    Serial.println("[ESP🔗] ✓ PAIRED successfully!");
    Serial.printf("[ESP🔗]   node_id:  %s\n", nodeId.c_str());
    Serial.printf("[ESP🔗]   light_id: %s\n", lightId.c_str());
    Serial.printf("[ESP🔗]   coord:    %s\n", formatMac(coordMac).c_str());
}

// Log pairing failure
inline void logPairFailure(const char* reason) {
    Serial.printf("[ESP🔗] ✗ Pairing failed: %s\n", reason);
}

// Log link health
inline void logLinkStatus(bool alive, uint32_t lastActivityMs) {
    Stats& stats = getStats();
    uint32_t age = millis() - lastActivityMs;
    
    if (alive) {
        Serial.printf("[ESP💓] Link ALIVE | last activity %u ms ago\n", age);
    } else {
        Serial.printf("[ESP💓] Link DEAD | no activity for %u ms\n", age);
    }
}

// Log command processing
inline void logCommandProcess(const String& command, bool success, const char* detail = nullptr) {
    if (success) {
        Serial.printf("[ESP⚙] ✓ Command: %s\n", command.c_str());
        if (detail) {
            Serial.printf("[ESP⚙]   %s\n", detail);
        }
    } else {
        Serial.printf("[ESP⚙] ✗ Command failed: %s\n", command.c_str());
        if (detail) {
            Serial.printf("[ESP⚙]   error: %s\n", detail);
        }
    }
}

// Log LED control
inline void logLedControl(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t brightness) {
    Serial.printf("[ESP💡] LED set: R=%d G=%d B=%d W=%d brightness=%d\n", r, g, b, w, brightness);
}

// Log temperature reading
inline void logTemperature(float tempC, bool valid) {
    if (valid) {
        Serial.printf("[ESP🌡] Temperature: %.2f°C\n", tempC);
    } else {
        Serial.println("[ESP🌡] ✗ Temperature read failed");
    }
}

// Log telemetry submission
inline void logTelemetrySubmit(float tempC, uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint16_t vbatMv) {
    Serial.printf("[ESP📊] Telemetry: temp=%.1f°C RGBW=(%d,%d,%d,%d) vbat=%dmV\n",
                  tempC, r, g, b, w, vbatMv);
}

// Log parse errors
inline void logParseError(const char* what, const char* reason) {
    Stats& stats = getStats();
    stats.parseErrors++;
    Serial.printf("[ESP⚙] ✗ Parse error: %s | %s\n", what, reason);
}

// Print statistics summary
inline void printStats() {
    Stats& stats = getStats();
    uint32_t now = millis();
    
    Serial.println("========== ESP-NOW Statistics ==========");
    Serial.printf("Paired:                 %s\n", stats.isPaired ? "YES" : "NO");
    Serial.printf("Messages Sent:          %u\n", stats.messagesSent);
    Serial.printf("  - Join Requests:      %u\n", stats.joinRequestsSent);
    Serial.printf("  - Status Messages:    %u\n", stats.statusMessagesSent);
    Serial.printf("Messages Received:      %u\n", stats.messagesReceived);
    Serial.printf("  - Join Accepts:       %u\n", stats.joinAcceptsReceived);
    Serial.printf("  - Light Commands:     %u\n", stats.lightCommandsReceived);
    Serial.printf("  - Acks:               %u\n", stats.acksReceived);
    Serial.printf("Send Errors:            %u\n", stats.sendErrors);
    Serial.printf("Parse Errors:           %u\n", stats.parseErrors);
    
    if (stats.lastSendMs > 0) {
        Serial.printf("Last Send:              %u ms ago\n", now - stats.lastSendMs);
    }
    if (stats.lastReceiveMs > 0) {
        Serial.printf("Last Receive:           %u ms ago\n", now - stats.lastReceiveMs);
    }
    if (stats.lastLinkActivityMs > 0) {
        Serial.printf("Last Link Activity:     %u ms ago\n", now - stats.lastLinkActivityMs);
    }
    Serial.println("========================================");
}

// Reset statistics
inline void resetStats() {
    Stats& stats = getStats();
    bool wasPaired = stats.isPaired;
    stats = Stats();
    stats.isPaired = wasPaired; // Keep pairing state
    Serial.println("[ESP] Statistics reset");
}

// Log heartbeat
inline void logHeartbeat(bool paired, bool linkAlive, uint32_t intervalMs = 30000) {
    static uint32_t lastHeartbeat = 0;
    uint32_t now = millis();
    
    if (now - lastHeartbeat >= intervalMs) {
        lastHeartbeat = now;
        Stats& stats = getStats();
        
        const char* status = paired ? (linkAlive ? "Connected" : "Paired/Idle") : "Unpaired";
        Serial.printf("[ESP💓] %s | sent=%u recv=%u errors=%u\n",
                      status, stats.messagesSent, stats.messagesReceived,
                      stats.sendErrors + stats.parseErrors);
    }
}

// Log encryption status
inline void logEncryption(bool enabled, const char* detail = nullptr) {
    if (enabled) {
        Serial.println("[ESP🔒] Encryption ENABLED");
    } else {
        Serial.println("[ESP🔒] Encryption DISABLED");
    }
    if (detail) {
        Serial.printf("[ESP🔒]   %s\n", detail);
    }
}

// Log power state
inline void logPowerState(const char* state, const char* detail = nullptr) {
    Serial.printf("[ESP⚡] Power: %s\n", state);
    if (detail) {
        Serial.printf("[ESP⚡]   %s\n", detail);
    }
}

// Log latency measurement
inline void logLatency(const char* operation, uint32_t startMs) {
    uint32_t latencyMs = millis() - startMs;
    
    if (latencyMs > 500) {
        Serial.printf("[ESP⏱] ⚠ High latency: %s took %u ms\n", operation, latencyMs);
    } else {
        Serial.printf("[ESP⏱] Latency: %s took %u ms\n", operation, latencyMs);
    }
}

// Log retry attempt
inline void logRetry(const char* operation, uint8_t attempt, uint8_t maxAttempts) {
    Serial.printf("[ESP🔄] Retry %d/%d: %s\n", attempt, maxAttempts, operation);
}

// Update link activity (called when any communication happens)
inline void updateLinkActivity() {
    getStats().lastLinkActivityMs = millis();
}

} // namespace EspNowLogger
