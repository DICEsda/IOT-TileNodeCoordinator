#include "Coordinator.h"
#include "../utils/Logger.h"
#include "../../shared/src/EspNowMessage.h"
#include <algorithm>

Coordinator::Coordinator()
    : espNow(nullptr)
    , mqtt(nullptr)
    , mmWave(nullptr)
    , nodes(nullptr)
    , zones(nullptr)
    , buttons(nullptr)
    , thermal(nullptr) {}


Coordinator::~Coordinator() {
    // Clean up in reverse order of initialization
    if (thermal) { delete thermal; thermal = nullptr; }
    if (buttons) { delete buttons; buttons = nullptr; }
    if (zones) { delete zones; zones = nullptr; }
    if (nodes) { delete nodes; nodes = nullptr; }
    if (mmWave) { delete mmWave; mmWave = nullptr; }
    if (mqtt) { delete mqtt; mqtt = nullptr; }
    if (espNow) { delete espNow; espNow = nullptr; }
}

bool Coordinator::begin() {
    // Don't call Logger::begin here - it's already called in main.cpp
    Logger::setMinLevel(Logger::INFO); // Reduce noise: default to INFO
    delay(500);
    
    Logger::info("Smart Tile Coordinator starting...");

    espNow = new EspNow();
    mqtt = new Mqtt();
    mmWave = new MmWave();
    nodes = new NodeRegistry();
    zones = new ZoneControl();
    buttons = new ButtonControl();
    thermal = new ThermalControl();

    Logger::info("Objects created, starting initialization...");

    // Initialize all components
    Logger::info("Initializing ESP-NOW...");
    if (!espNow->begin()) {
        Logger::error("Failed to initialize ESP-NOW");
        return false;
    }
    Logger::info("ESP-NOW initialized successfully");

    // Register message callback for regular node messages
    espNow->setMessageCallback([this](const String& nodeId, const uint8_t* data, size_t len) {
        if (data && len > 0) {
            this->handleNodeMessage(nodeId, data, len);
        }
    });

    // Register send error callback for visual feedback
    espNow->setSendErrorCallback([this](const String& nodeId) {
        // Flash red on all LEDs to indicate send failure
        statusLed.pulse(180, 0, 0, 200); // Red flash for 200ms
        Logger::warn("ESP-NOW send failed to node %s - showing red flash", nodeId.c_str());
    });
    
    // Register pairing callback to handle join requests coming from nodes
    espNow->setPairingCallback([this](const uint8_t* mac, const uint8_t* data, size_t len) {
        if (!mac || !data || len == 0) {
            Logger::warn("Invalid pairing callback parameters");
            return;
        }
        String payload((const char*)data, len);
        MessageType mt = MessageFactory::getMessageType(payload);
        if (mt != MessageType::JOIN_REQUEST) {
            Logger::warn("Pairing callback: unexpected message type");
            return;
        }

        // Format MAC string
        char macStr[18];
        snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        String nodeId(macStr);

        if (!nodes->isPairingActive()) {
            Logger::warn("Rejecting pairing from %s: pairing not active", macStr);
            return;
        }

        bool regOk = nodes->processPairingRequest(mac, nodeId);
        if (!regOk) {
            Logger::warn("Failed to register node %s during pairing", macStr);
            return;
        }

        // Add as ESP-NOW peer (unencrypted) - this now handles duplicates gracefully
        espNow->addPeer(mac);

        JoinAcceptMessage accept;
        accept.node_id = nodeId;
        accept.light_id = nodes->getLightForNode(nodeId);
        accept.lmk = ""; // Unencrypted for now
        accept.cfg.pwm_freq = 0; // Not used but set explicitly
        accept.cfg.rx_window_ms = 20;
        accept.cfg.rx_period_ms = 100;
        String json = accept.toJson();
        
        // Debug: log the message we're sending
        Logger::info("JOIN_ACCEPT message (%d bytes): %s", json.length(), json.c_str());

        // send back to node mac (will auto-add peer if missing)
        if (!espNow->sendToMac(mac, json)) {
            Logger::warn("Failed to send join_accept to %s", macStr);
        } else {
            Logger::info("Sent join_accept to %s", macStr);
        }

        // Assign LED and give brief green flash for connection feedback
        int idx = assignGroupForNode(nodeId);
        if (idx >= 0) {
            groupConnected[idx] = true; // mark active connection
            flashLedForNode(nodeId, 400); // Longer flash for pairing success
        }
        
        // Show "OK" confirmation: all LEDs green pulse for 300ms
        statusLed.pulse(0, 150, 0, 300); // Green confirmation flash
        Logger::info("Pairing successful - OK confirmation shown");
    });

    Logger::info("Initializing MQTT...");
    if (!mqtt->begin()) {
        Logger::error("Failed to initialize MQTT");
        return false;
    }
    Logger::info("MQTT initialized successfully");

    Logger::info("Initializing mmWave sensor...");
    if (!mmWave->begin()) {
        Logger::warn("Failed to initialize mmWave sensor - continuing without it");
        // Non-critical, continue without mmWave
    } else {
        Logger::info("mmWave initialized successfully");
    }

    Logger::info("Initializing node registry...");
    if (!nodes->begin()) {
        Logger::error("Failed to initialize node registry");
        return false;
    }
    Logger::info("Node registry initialized successfully");

    // Initialize onboard status LED
    statusLed.begin();
    
    // Test pattern: cycle through all pixels briefly to verify SK6812B strip is working
    Logger::info("Testing SK6812B strip (%d pixels)...", Pins::RgbLed::NUM_PIXELS);
    for (uint8_t i = 0; i < Pins::RgbLed::NUM_PIXELS; ++i) {
        statusLed.clear();
        statusLed.setPixel(i, 0, 100, 0); // green
        statusLed.show();
        delay(100);
    }
    statusLed.clear();
    
    // Initialize LED group mapping containers (4 pixels per group)
    int groupCount = Pins::RgbLed::NUM_PIXELS / 4;
    groupToNode.assign(groupCount, String());
    groupConnected.assign(groupCount, false);
    groupFlashUntilMs.assign(groupCount, 0);
    rebuildLedMappingFromRegistry();

    if (!zones->begin()) {
        Logger::error("Failed to initialize zone control");
        return false;
    }

    // Disable idle breathing; LEDs indicate node connections per requirements
    statusLed.setIdleBreathing(false);

    if (!buttons->begin()) {
        Logger::error("Failed to initialize button control");
        return false;
    }

    if (!thermal->begin()) {
        Logger::error("Failed to initialize thermal control");
        return false;
    }

    // Register event handlers
    mmWave->setEventCallback([this](const MmWaveEvent& event) {
        this->onMmWaveEvent(event);
    });

    thermal->registerThermalAlertCallback([this](const String& nodeId, const NodeThermalData& data) {
        this->onThermalEvent(nodeId, data);
    });

    buttons->setEventCallback([this](const String& buttonId, bool pressed) {
        this->onButtonEvent(buttonId, pressed);
    });
    
    buttons->setLongPressCallback([this]() {
        // Long press: flash all connected nodes on white channel while held
        this->longPressActive = true;
        this->startFlashAll();
    });
    
    buttons->setVeryLongPressCallback([this]() {
        // Very long press (10s): clear all known nodes
        Logger::info("===========================================");
        Logger::info("CLEARING ALL NODES (10s hold detected)");
        Logger::info("===========================================");
        if (nodes) {
            nodes->clearAllNodes();
        }
        if (espNow) {
            espNow->clearAllPeers(); // Clear ESP-NOW peers
        }
        // Rebuild LED mapping to clear visual indicators
        rebuildLedMappingFromRegistry();
        updateLeds();
        Logger::info("All nodes cleared. Release button to continue.");
        Logger::info("===========================================");
    });

Logger::info("Coordinator initialization complete");
Logger::info("==============================================");
Logger::info("System ready! Touch sensor to pair nodes.");
Logger::info("Hold 4s to run wave test on paired nodes.");
logConnectedNodes();
Logger::info("==============================================");
return true;
}

void Coordinator::loop() {
    // Use guard checks to prevent null pointer crashes
    if (espNow) espNow->loop();
    if (mqtt) mqtt->loop();
    if (mmWave) mmWave->loop();
    if (nodes) nodes->loop();
    if (zones) zones->loop();
    if (buttons) buttons->loop();
    if (thermal) thermal->loop();
    
    // Update status LED pulse (pairing) if active
    statusLed.loop();

    // While flashing mode is active and button is held, tick the flash
    if (flashAllActive && buttonDown) {
        flashAllTick(millis());
    }

    // Always update per-node LEDs (show connection state)
    if (!statusLed.isPulsing()) {
        updateLeds();
    }

    // Periodically ping and check staleness
    static uint32_t lastPing = 0;
    static uint32_t lastStaleCheck = 0;
    uint32_t now = millis();
    if (now - lastPing > 2000) {
        sendHealthPings();
        lastPing = now;
    }
    if (now - lastStaleCheck > 5000) {
        checkStaleConnections();
        lastStaleCheck = now;
    }
}

void Coordinator::onMmWaveEvent(const MmWaveEvent& event) {
    // Guard against null pointers
    if (!zones || !nodes || !thermal || !espNow || !mqtt) {
        Logger::error("Cannot process mmWave event: components not initialized");
        return;
    }
    
    // Process presence detection
    auto affectedLights = zones->getLightsForZone(event.sensorId);
    for (const auto& lightId : affectedLights) {
        auto nodeId = nodes->getNodeForLight(lightId);
        if (!nodeId.isEmpty()) {
            // Check thermal status before setting brightness
            uint8_t maxBrightness = thermal->getNodeDerationLevel(nodeId);
            uint8_t targetBrightness = event.presence ? maxBrightness : 0;
            
            // Send command through ESP-NOW
            espNow->sendLightCommand(nodeId, targetBrightness);
            
            // Publish state change to MQTT
            mqtt->publishLightState(lightId, targetBrightness);
        }
    }
}

void Coordinator::onThermalEvent(const String& nodeId, const NodeThermalData& data) {
    // Guard against null pointers
    if (!mqtt || !nodes || !zones || !espNow) {
        Logger::error("Cannot process thermal event: components not initialized");
        return;
    }
    
    // Handle thermal alerts
    Logger::warning("Thermal alert for node %s: %.1f°C, deration: %d%%",
                   nodeId.c_str(), data.temperature, data.derationLevel);
                   
    // Publish thermal event to MQTT
    mqtt->publishThermalEvent(nodeId, data);
    
    // If node is currently active, update its brightness with deration
    auto lightId = nodes->getLightForNode(nodeId);
    if (!lightId.isEmpty() && zones->isLightActive(lightId)) {
        espNow->sendLightCommand(nodeId, data.derationLevel);
    }
}

void Coordinator::onButtonEvent(const String& buttonId, bool pressed) {
    // Guard against null pointers
    if (!nodes || !espNow) {
        Logger::error("Cannot process button event: components not initialized");
        return;
    }

    if (pressed) {
        // Begin press: track state, do not enter pairing yet
        buttonDown = true;
        buttonPressedAt = millis();
        longPressActive = false;
        return;
    }

    // On release
    buttonDown = false;
    if (longPressActive) {
        // Stop flashing and suppress pairing
        stopFlashAll();
        longPressActive = false;
        return;
    }

    // Treat as short press -> open pairing window
    Logger::info("Pairing button short press - opening pairing window");
    const uint32_t windowMs = 60000; // 60s pairing window
    nodes->startPairing(windowMs);
    espNow->enablePairingMode(windowMs);
    
    // Visual feedback: blue pulse to indicate pairing mode active
    statusLed.pulse(0, 0, 180, 500); // Blue pulse
    
    Logger::info("Pairing window open for %d ms", windowMs);
}

void Coordinator::handleNodeMessage(const String& nodeId, const uint8_t* data, size_t len) {
    // Parse the JSON payload to detect message type
    String payload((const char*)data, len);
    MessageType mt = MessageFactory::getMessageType(payload);

    // Re-accept known node JOIN_REQUEST even if pairing window is closed
    if (mt == MessageType::JOIN_REQUEST && nodes) {
        // nodeId is the MAC string
        String existingLight = nodes->getLightForNode(nodeId);
        if (existingLight.length() > 0 || nodes->getNodeStatus(nodeId).nodeId.length() > 0) {
            // Known node: respond with join_accept
            uint8_t mac[6];
            if (EspNow::macStringToBytes(nodeId, mac)) {
                espNow->addPeer(mac);
            }
            JoinAcceptMessage accept;
            accept.node_id = nodeId;
            accept.light_id = nodes->getLightForNode(nodeId);
            accept.lmk = "";
            accept.cfg.rx_window_ms = 20;
            accept.cfg.rx_period_ms = 100;
            String json = accept.toJson();
            // Parse MAC for send
            uint8_t mac2[6];
            if (EspNow::macStringToBytes(nodeId, mac2)) {
                if (!espNow->sendToMac(mac2, json)) {
                    Logger::warn("Failed to send re-join_accept to %s", nodeId.c_str());
                } else {
                    Logger::info("Re-accepted known node %s", nodeId.c_str());
                }
            }
        }
    }

    // Update last-seen for any message first
    if (nodes) {
        nodes->updateNodeStatus(nodeId, 0);
    }
    
    // Ensure we have a group assigned for this node
    int idx = getGroupIndexForNode(nodeId);
    if (idx < 0) {
        idx = assignGroupForNode(nodeId);
        Logger::info("Assigned group %d to node %s", idx + 1, nodeId.c_str());
    }
    if (idx >= 0) {
        if (!groupConnected[idx]) {
            Logger::info("[Node %d] %s CONNECTED", idx + 1, nodeId.c_str());
        }
        groupConnected[idx] = true; // mark as active connection
        flashLedForNode(nodeId, 150); // brief activity flash on each message
    }

    // Improved logging per node index with MAC
    Logger::info("[Node %d] %s %s | %d bytes", 
                 idx >= 0 ? idx + 1 : 0,
                 nodeId.c_str(),
                 mt == MessageType::NODE_STATUS ? "STATUS" : "MESSAGE", 
                 (int)len);

    // Mark last seen on status and log sensor data
    if (mt == MessageType::NODE_STATUS && nodes) {
        nodes->updateNodeStatus(nodeId, 0);
        
        // Parse and log sensor data from telemetry
        EspNowMessage* msg = MessageFactory::createMessage(payload);
        if (msg && msg->type == MessageType::NODE_STATUS) {
            NodeStatusMessage* statusMsg = static_cast<NodeStatusMessage*>(msg);
            
            // Log temperature if available
            if (statusMsg->temperature > -50.0f && statusMsg->temperature < 150.0f) {
                Logger::info("  [Node %d] Temperature: %.2f°C", 
                             idx >= 0 ? idx + 1 : 0, 
                             statusMsg->temperature);
            }
            
            // Log button state
            Logger::info("  [Node %d] Button: %s, RGBW: (%d,%d,%d,%d)", 
                         idx >= 0 ? idx + 1 : 0,
                         statusMsg->button_pressed ? "PRESSED" : "Released",
                         statusMsg->avg_r, statusMsg->avg_g, statusMsg->avg_b, statusMsg->avg_w);
            
            delete msg;
        }
    }
}

// ===== LED mapping helpers =====
void Coordinator::rebuildLedMappingFromRegistry() {
    nodeToGroup.clear();
    std::fill(groupToNode.begin(), groupToNode.end(), String());
    std::fill(groupConnected.begin(), groupConnected.end(), false);
    std::fill(groupFlashUntilMs.begin(), groupFlashUntilMs.end(), 0);

    if (!nodes) return;
    // Assign deterministically by sorted nodeId up to available groups
    auto list = nodes->getAllNodes();
    std::sort(list.begin(), list.end(), [](const NodeInfo& a, const NodeInfo& b){ return a.nodeId < b.nodeId; });
    int maxGroups = Pins::RgbLed::NUM_PIXELS / 4;
    int idx = 0;
    uint32_t now = millis();
    for (const auto& n : list) {
        if (idx >= maxGroups) break;
        nodeToGroup[n.nodeId] = idx;
        groupToNode[idx] = n.nodeId;
        // Mark as connected if recently seen (within last 6 seconds)
        groupConnected[idx] = (n.lastSeenMs > 0 && (now - n.lastSeenMs) <= 6000U);
        idx++;
    }
}

int Coordinator::getGroupIndexForNode(const String& nodeId) {
    auto it = nodeToGroup.find(nodeId);
    if (it != nodeToGroup.end()) return it->second;
    return -1;
}

int Coordinator::assignGroupForNode(const String& nodeId) {
    // Already assigned?
    int cur = getGroupIndexForNode(nodeId);
    if (cur >= 0) return cur;
    // Find first free group slot
    int groupCount = Pins::RgbLed::NUM_PIXELS / 4;
    for (int i = 0; i < groupCount; ++i) {
        if (groupToNode[i].length() == 0) {
            groupToNode[i] = nodeId;
            nodeToGroup[nodeId] = i;
            return i;
        }
    }
    Logger::warn("No free LED group available for node %s", nodeId.c_str());
    return -1;
}

void Coordinator::flashLedForNode(const String& nodeId, uint32_t durationMs) {
    int idx = getGroupIndexForNode(nodeId);
    if (idx < 0) return;
    groupFlashUntilMs[idx] = millis() + durationMs;
}

void Coordinator::updateLeds() {
    uint32_t now = millis();
    int groupCount = Pins::RgbLed::NUM_PIXELS / 4;
    for (int g = 0; g < groupCount; ++g) {
        uint8_t r=0,gc=0,b=0;
        if (groupFlashUntilMs[g] > now) {
            // Bright green flash (activity) at 50%
            r=0; gc=128; b=0;
        } else if (groupToNode[g].length() > 0) {
            if (groupConnected[g]) {
                // Solid green (dim) at 50%
                r=0; gc=45; b=0;
            } else {
                // Red for disconnected/stale node at 50%
                r=90; gc=0; b=0;
            }
        } else {
            r=0; gc=0; b=0;
        }
        // Paint the 4-pixel group
        int base = g * 4;
        for (int k=0;k<4;k++) {
            statusLed.setPixel(base+k, r, gc, b);
        }
    }
    statusLed.show();
}

void Coordinator::logConnectedNodes() {
    if (!nodes) return;
    auto allNodes = nodes->getAllNodes();
    if (allNodes.empty()) {
        Logger::info("Connected nodes: 0");
        return;
    }

    std::sort(allNodes.begin(), allNodes.end(), [](const NodeInfo& a, const NodeInfo& b) {
        return a.nodeId < b.nodeId;
    });

    Logger::info("Connected nodes: %d", allNodes.size());
    for (const auto& node : allNodes) {
        int idx = getGroupIndexForNode(node.nodeId);
        bool alive = (idx >= 0) ? groupConnected[idx] : false;
        Logger::info("  [Node %d] %s -> %s [%s]",
                     idx >= 0 ? idx + 1 : 0,
                     node.nodeId.c_str(),
                     node.lightId.c_str(),
                     alive ? "ONLINE" : "OFFLINE");
    }
}

void Coordinator::checkStaleConnections() {
    if (!nodes) return;
    auto allNodes = nodes->getAllNodes();
    uint32_t now = millis();

    for (const auto& node : allNodes) {
        int idx = getGroupIndexForNode(node.nodeId);
        if (idx >= 0 && groupConnected[idx]) {
            // Mark disconnected if last seen > 6s
            if (node.lastSeenMs > 0 && (now - node.lastSeenMs) > 6000U) {
                groupConnected[idx] = false;
                Logger::warn("[Node %d] DISCONNECTED (timeout)", idx + 1);
            }
        }
    }
}

void Coordinator::sendHealthPings() {
    if (!espNow) return;
    int groupCount = Pins::RgbLed::NUM_PIXELS / 4;
    for (int g = 0; g < groupCount; ++g) {
        // Only ping nodes that currently appear connected
        if (groupToNode[g].length() == 0 || !groupConnected[g]) continue;
        uint8_t mac[6];
        if (!EspNow::macStringToBytes(groupToNode[g], mac)) continue;
        const String ping = "{\"msg\":\"ping\"}";
        espNow->sendToMac(mac, ping);
    }
}

void Coordinator::startFlashAll() {
    // Build list of connected nodes
    if (!nodes || !espNow) return;
    auto all = nodes->getAllNodes();
    bool any = false;
    for (const auto& n : all) {
        int gi = getGroupIndexForNode(n.nodeId);
        if (gi >= 0 && groupConnected[gi]) { any = true; break; }
    }
    if (!any) {
        Logger::info("No connected nodes - flash-all suppressed");
        flashAllActive = false;
        return;
    }
    flashAllActive = true;
    flashOn = false; // will toggle to ON on first tick
    lastFlashTick = 0;
    Logger::info("Flash-all: ACTIVE (hold button to keep flashing)");
    // trigger first tick immediately for instant feedback
    flashAllTick(millis());
}

void Coordinator::stopFlashAll() {
    flashAllActive = false;
    flashOn = false;
    lastFlashTick = 0;
    Logger::info("Flash-all: STOPPED");
}

void Coordinator::flashAllTick(uint32_t now) {
    const uint32_t intervalMs = 350; // toggle cadence
    if (now - lastFlashTick < intervalMs) return;
    lastFlashTick = now;
    flashOn = !flashOn;

    // Send white on/off to all connected nodes with short TTL and override_status
    auto all = nodes->getAllNodes();
    for (const auto& n : all) {
        int gi = getGroupIndexForNode(n.nodeId);
        if (gi < 0 || !groupConnected[gi]) continue;
        uint8_t level = flashOn ? 128 : 0; // 50% brightness
        // quick fade for nicer blink
        espNow->sendLightCommand(n.nodeId, level, 60 /*fadeMs*/, true /*override*/, 500 /*ttl*/);
    }
}

void Coordinator::triggerNodeWaveTest() {
    if (!nodes || !espNow) {
        Logger::warn("Cannot run node wave test: components not initialized");
        return;
    }

    // Build list of currently connected nodes only
    auto allNodes = nodes->getAllNodes();
    std::vector<NodeInfo> connected;
    connected.reserve(allNodes.size());
    for (const auto& n : allNodes) {
        int gi = getGroupIndexForNode(n.nodeId);
        if (gi >= 0 && groupConnected[gi]) connected.push_back(n);
    }
    if (connected.empty()) {
        Logger::info("No connected nodes - wave test skipped");
        return;
    }

    Logger::info("Starting wave on %d connected node(s)...", connected.size());

    // Deterministic order and synchronized start time across nodes
    std::sort(connected.begin(), connected.end(), [](const NodeInfo& a, const NodeInfo& b) {
        return a.nodeId < b.nodeId;
    });

    const uint32_t now = millis();
    const uint32_t startAt = now + 300; // 300ms in the future to allow delivery jitter
    const uint16_t periodMs = 1200;
    const uint16_t durationMs = 4000;

    for (const auto& node : connected) {
        uint8_t mac[6];
        if (!EspNow::macStringToBytes(node.nodeId, mac)) continue;
        // Include start_at to coordinate across nodes
        String wave = String("{\"msg\":\"wave\",\"period_ms\":") + String(periodMs) +
                      ",\"duration_ms\":" + String(durationMs) +
                      ",\"start_at\":" + String(startAt) + "}";
        espNow->sendToMac(mac, wave);
    }

    Logger::info("Wave command sent");
}
