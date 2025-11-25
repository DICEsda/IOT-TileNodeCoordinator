#pragma once

#include <Arduino.h>
#include "../Logger.h"

/**
 * MqttLogger - Detailed MQTT pipeline logging for debugging and monitoring
 * 
 * Provides structured logging for:
 * - Connection lifecycle
 * - Message publishing (with payload inspection)
 * - Message reception (with topic parsing)
 * - Telemetry flow tracking
 * - Command flow tracking
 * - Error diagnostics
 */
namespace MqttLogger {

// MQTT pipeline stages for tracking message flow
enum Stage {
    CONNECT,
    DISCONNECT,
    PUBLISH,
    SUBSCRIBE,
    RECEIVE,
    PROCESS,
    FORWARD,
    ERROR_STAGE
};

// Message types for categorization
enum MessageType {
    NODE_TELEMETRY,
    COORD_TELEMETRY,
    MMWAVE_EVENT,
    NODE_COMMAND,
    COORD_COMMAND,
    SERIAL_LOG,
    UNKNOWN
};

// Statistics tracking
struct Stats {
    uint32_t messagesPublished = 0;
    uint32_t messagesReceived = 0;
    uint32_t publishErrors = 0;
    uint32_t parseErrors = 0;
    uint32_t lastPublishMs = 0;
    uint32_t lastReceiveMs = 0;
    
    // Telemetry counters
    uint32_t nodeTelemetryCount = 0;
    uint32_t coordTelemetryCount = 0;
    uint32_t mmwaveEventCount = 0;
    
    // Command counters
    uint32_t nodeCommandCount = 0;
    uint32_t coordCommandCount = 0;
};

// Global statistics instance
inline Stats& getStats() {
    static Stats stats;
    return stats;
}

// Helper to get message type from topic
inline MessageType getMessageType(const String& topic) {
    if (topic.indexOf("/node/") >= 0 && topic.endsWith("/telemetry")) {
        return NODE_TELEMETRY;
    } else if (topic.indexOf("/coord/") >= 0 && topic.endsWith("/telemetry")) {
        return COORD_TELEMETRY;
    } else if (topic.indexOf("/mmwave") >= 0) {
        return MMWAVE_EVENT;
    } else if (topic.indexOf("/node/") >= 0 && topic.endsWith("/cmd")) {
        return NODE_COMMAND;
    } else if (topic.indexOf("/coord/") >= 0 && topic.endsWith("/cmd")) {
        return COORD_COMMAND;
    } else if (topic.indexOf("/serial") >= 0) {
        return SERIAL_LOG;
    }
    return UNKNOWN;
}

// Helper to get message type name
inline const char* getMessageTypeName(MessageType type) {
    switch (type) {
        case NODE_TELEMETRY: return "NodeTelemetry";
        case COORD_TELEMETRY: return "CoordTelemetry";
        case MMWAVE_EVENT: return "MmWaveEvent";
        case NODE_COMMAND: return "NodeCommand";
        case COORD_COMMAND: return "CoordCommand";
        case SERIAL_LOG: return "SerialLog";
        case UNKNOWN: return "Unknown";
        default: return "Invalid";
    }
}

// Extract IDs from topic (site, coord, node)
struct TopicIds {
    String siteId;
    String coordId;
    String nodeId;
    bool valid = false;
};

inline TopicIds parseTopicIds(const String& topic) {
    TopicIds ids;
    
    // Topic format: site/{siteId}/coord/{coordId}/... or site/{siteId}/node/{nodeId}/...
    int sitePos = topic.indexOf("site/");
    if (sitePos < 0) return ids;
    
    int siteStart = sitePos + 5;
    int siteEnd = topic.indexOf('/', siteStart);
    if (siteEnd < 0) return ids;
    
    ids.siteId = topic.substring(siteStart, siteEnd);
    
    // Check for coordinator or node
    int coordPos = topic.indexOf("/coord/", siteEnd);
    int nodePos = topic.indexOf("/node/", siteEnd);
    
    if (coordPos >= 0) {
        int coordStart = coordPos + 7;
        int coordEnd = topic.indexOf('/', coordStart);
        if (coordEnd < 0) coordEnd = topic.length();
        ids.coordId = topic.substring(coordStart, coordEnd);
        ids.valid = true;
    } else if (nodePos >= 0) {
        int nodeStart = nodePos + 6;
        int nodeEnd = topic.indexOf('/', nodeStart);
        if (nodeEnd < 0) nodeEnd = topic.length();
        ids.nodeId = topic.substring(nodeStart, nodeEnd);
        ids.valid = true;
    }
    
    return ids;
}

// Log connection event
inline void logConnect(const String& broker, uint16_t port, const String& clientId, bool success) {
    Stats& stats = getStats();
    
    if (success) {
        Logger::info("[MQTT] ✓ Connected to broker: %s:%d as '%s'", 
                     broker.c_str(), port, clientId.c_str());
    } else {
        Logger::error("[MQTT] ✗ Connection failed: %s:%d as '%s'", 
                      broker.c_str(), port, clientId.c_str());
    }
}

// Log disconnect event
inline void logDisconnect(int reason) {
    Logger::warn("[MQTT] ✗ Disconnected (reason: %d)", reason);
}

// Log subscription
inline void logSubscribe(const String& topic, bool success) {
    if (success) {
        Logger::info("[MQTT] ✓ Subscribed to: %s", topic.c_str());
    } else {
        Logger::error("[MQTT] ✗ Subscribe failed: %s", topic.c_str());
    }
}

// Log publish with detailed information
inline void logPublish(const String& topic, const String& payload, bool success, uint16_t payloadLen = 0) {
    Stats& stats = getStats();
    MessageType type = getMessageType(topic);
    TopicIds ids = parseTopicIds(topic);
    
    if (success) {
        stats.messagesPublished++;
        stats.lastPublishMs = millis();
        
        // Update type-specific counters
        switch (type) {
            case NODE_TELEMETRY:
                stats.nodeTelemetryCount++;
                break;
            case COORD_TELEMETRY:
                stats.coordTelemetryCount++;
                break;
            case MMWAVE_EVENT:
                stats.mmwaveEventCount++;
                break;
            default:
                break;
        }
        
        // Truncate long payloads for display
        String displayPayload = payload;
        if (displayPayload.length() > 100) {
            displayPayload = displayPayload.substring(0, 97) + "...";
        }
        
        Logger::info("[MQTT→] %s | topic=%s | size=%d bytes", 
                     getMessageTypeName(type), topic.c_str(), 
                     payloadLen > 0 ? payloadLen : payload.length());
        Logger::debug("[MQTT→] payload: %s", displayPayload.c_str());
        
        // Log extracted IDs at debug level
        if (ids.valid) {
            if (!ids.nodeId.isEmpty()) {
                Logger::debug("[MQTT→] site=%s node=%s", ids.siteId.c_str(), ids.nodeId.c_str());
            } else if (!ids.coordId.isEmpty()) {
                Logger::debug("[MQTT→] site=%s coord=%s", ids.siteId.c_str(), ids.coordId.c_str());
            }
        }
    } else {
        stats.publishErrors++;
        Logger::error("[MQTT→] ✗ Publish failed | topic=%s | size=%d bytes", 
                      topic.c_str(), payloadLen > 0 ? payloadLen : payload.length());
    }
}

// Log received message
inline void logReceive(const String& topic, const uint8_t* payload, uint16_t length) {
    Stats& stats = getStats();
    stats.messagesReceived++;
    stats.lastReceiveMs = millis();
    
    MessageType type = getMessageType(topic);
    TopicIds ids = parseTopicIds(topic);
    
    // Update command counters
    if (type == NODE_COMMAND) {
        stats.nodeCommandCount++;
    } else if (type == COORD_COMMAND) {
        stats.coordCommandCount++;
    }
    
    Logger::info("[MQTT←] %s | topic=%s | size=%d bytes", 
                 getMessageTypeName(type), topic.c_str(), length);
    
    // Log payload at debug level (truncated)
    if (Logger::getMinLevel() <= Logger::DEBUG) {
        String payloadStr;
        if (length > 0 && length < 512) {
            payloadStr = String((char*)payload, length);
            if (payloadStr.length() > 100) {
                payloadStr = payloadStr.substring(0, 97) + "...";
            }
            Logger::debug("[MQTT←] payload: %s", payloadStr.c_str());
        }
    }
    
    // Log extracted IDs at debug level
    if (ids.valid) {
        if (!ids.nodeId.isEmpty()) {
            Logger::debug("[MQTT←] site=%s node=%s", ids.siteId.c_str(), ids.nodeId.c_str());
        } else if (!ids.coordId.isEmpty()) {
            Logger::debug("[MQTT←] site=%s coord=%s", ids.siteId.c_str(), ids.coordId.c_str());
        }
    }
}

// Log message processing
inline void logProcess(const String& topic, const String& action, bool success, const String& detail = "") {
    if (success) {
        if (detail.isEmpty()) {
            Logger::info("[MQTT⚙] %s | topic=%s", action.c_str(), topic.c_str());
        } else {
            Logger::info("[MQTT⚙] %s | topic=%s | %s", action.c_str(), topic.c_str(), detail.c_str());
        }
    } else {
        Logger::error("[MQTT⚙] ✗ %s failed | topic=%s | %s", 
                      action.c_str(), topic.c_str(), detail.c_str());
    }
}

// Log ESP-NOW forwarding
inline void logForward(const String& nodeId, const String& msgType, bool success, const String& detail = "") {
    if (success) {
        Logger::info("[MQTT→ESP] Forwarded %s to node=%s | %s", 
                     msgType.c_str(), nodeId.c_str(), detail.c_str());
    } else {
        Logger::error("[MQTT→ESP] ✗ Forward failed | node=%s | %s | %s",
                      nodeId.c_str(), msgType.c_str(), detail.c_str());
    }
}

// Log parsing errors
inline void logParseError(const String& topic, const String& reason) {
    Stats& stats = getStats();
    stats.parseErrors++;
    Logger::error("[MQTT⚙] ✗ Parse error | topic=%s | reason=%s", topic.c_str(), reason.c_str());
}

// Log QoS and retention info
inline void logQoS(const String& topic, uint8_t qos, bool retained) {
    Logger::debug("[MQTT] QoS=%d retained=%d | topic=%s", qos, retained, topic.c_str());
}

// Print statistics summary
inline void printStats() {
    Stats& stats = getStats();
    uint32_t now = millis();
    
    Logger::info("========== MQTT Statistics ==========");
    Logger::info("Messages Published:     %u", stats.messagesPublished);
    Logger::info("  - Node Telemetry:     %u", stats.nodeTelemetryCount);
    Logger::info("  - Coord Telemetry:    %u", stats.coordTelemetryCount);
    Logger::info("  - MmWave Events:      %u", stats.mmwaveEventCount);
    Logger::info("Messages Received:      %u", stats.messagesReceived);
    Logger::info("  - Node Commands:      %u", stats.nodeCommandCount);
    Logger::info("  - Coord Commands:     %u", stats.coordCommandCount);
    Logger::info("Publish Errors:         %u", stats.publishErrors);
    Logger::info("Parse Errors:           %u", stats.parseErrors);
    
    if (stats.lastPublishMs > 0) {
        Logger::info("Last Publish:           %u ms ago", now - stats.lastPublishMs);
    }
    if (stats.lastReceiveMs > 0) {
        Logger::info("Last Receive:           %u ms ago", now - stats.lastReceiveMs);
    }
    Logger::info("====================================");
}

// Reset statistics
inline void resetStats() {
    getStats() = Stats();
    Logger::info("[MQTT] Statistics reset");
}

// Log heartbeat (called periodically to show MQTT is alive)
inline void logHeartbeat(bool connected, uint32_t intervalMs = 60000) {
    static uint32_t lastHeartbeat = 0;
    uint32_t now = millis();
    
    if (now - lastHeartbeat >= intervalMs) {
        lastHeartbeat = now;
        Stats& stats = getStats();
        
        if (connected) {
            Logger::info("[MQTT💓] Alive | pub=%u recv=%u errors=%u", 
                         stats.messagesPublished, stats.messagesReceived, 
                         stats.publishErrors + stats.parseErrors);
        } else {
            Logger::warn("[MQTT💓] Disconnected | reconnect needed");
        }
    }
}

// Log buffer/queue status
inline void logBufferStatus(uint16_t queueSize, uint16_t maxQueue, uint16_t droppedMessages = 0) {
    if (queueSize > maxQueue * 0.8) {
        Logger::warn("[MQTT] Queue high: %u/%u (dropped=%u)", queueSize, maxQueue, droppedMessages);
    } else {
        Logger::debug("[MQTT] Queue: %u/%u", queueSize, maxQueue);
    }
}

// Log message timing (for latency tracking)
inline void logLatency(const String& messageType, uint32_t startMs) {
    uint32_t latencyMs = millis() - startMs;
    
    if (latencyMs > 1000) {
        Logger::warn("[MQTT⏱] High latency: %s took %u ms", messageType.c_str(), latencyMs);
    } else {
        Logger::debug("[MQTT⏱] Latency: %s took %u ms", messageType.c_str(), latencyMs);
    }
}

} // namespace MqttLogger
