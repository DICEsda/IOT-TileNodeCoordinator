#include "Coordinator.h"
#include "../utils/Logger.h"
#include "../../shared/src/EspNowMessage.h"
#include "../comm/WifiManager.h"
#include "../sensors/AmbientLightSensor.h"
#include <algorithm>
#include <ArduinoJson.h>
#if defined(ARDUINO_ARCH_ESP32)
#include <esp32-hal.h>
#include <esp_wifi.h>
#endif

Coordinator::Coordinator()
    : espNow(nullptr)
    , mqtt(nullptr)
    , mmWave(nullptr)
    , nodes(nullptr)
    , zones(nullptr)
    , buttons(nullptr)
    , thermal(nullptr)
    , wifi(nullptr)
    , ambientLight(nullptr)
    , manualLedMode(false)
    , manualR(0)
    , manualG(0)
    , manualB(0)
    , manualLedTimeoutMs(0) {}


Coordinator::~Coordinator() {
    // Clean up in reverse order of initialization
    if (thermal) { delete thermal; thermal = nullptr; }
    if (buttons) { delete buttons; buttons = nullptr; }
    if (zones) { delete zones; zones = nullptr; }
    if (nodes) { delete nodes; nodes = nullptr; }
    if (mmWave) { delete mmWave; mmWave = nullptr; }
    if (ambientLight) { delete ambientLight; ambientLight = nullptr; }
    if (mqtt) { delete mqtt; mqtt = nullptr; }
    if (wifi) { delete wifi; wifi = nullptr; }
    if (espNow) { delete espNow; espNow = nullptr; }
}

bool Coordinator::begin() {
    // Don't call Logger::begin here - it's already called in main.cpp
    Logger::setMinLevel(Logger::INFO); // Reduce noise: default to INFO
    delay(500);
    bootStatus.clear();
    
    Logger::info("Smart Tile Coordinator starting...");
    publishLog("Smart Tile Coordinator starting...", "INFO", "setup");

    espNow = new EspNow();
    mqtt = new Mqtt();
    mmWave = new MmWave();
    nodes = new NodeRegistry();
    zones = new ZoneControl();
    buttons = new ButtonControl();
    thermal = new ThermalControl();
    wifi = new WifiManager();
    ambientLight = new AmbientLightSensor();

    Logger::info("Objects created, starting initialization...");

    // Initialize ESP-NOW first (before WiFi connects)
    Logger::info("Initializing ESP-NOW...");
    bool espNowOk = espNow->begin();
    recordBootStatus("ESP-NOW", espNowOk, espNowOk ? "Radio ready" : "init failed");
    if (!espNowOk) {
        Logger::error("Failed to initialize ESP-NOW");
        return false;
    }
    Logger::info("ESP-NOW initialized successfully");
    publishLog("ESP-NOW initialized successfully", "INFO", "setup");

    // Link EspNow to WiFi so channels sync on connection
    wifi->setEspNow(espNow);

    bool wifiReady = wifi && wifi->begin();
    WifiManager::Status wifiState;
    if (wifi) {
        wifiState = wifi->getStatus();
    }
    String wifiDetail;
    if (wifiReady && wifiState.connected) {
        wifiDetail = wifiState.ssid + " @ " + wifiState.ip.toString();
    } else if (wifiState.offlineMode) {
        wifiDetail = "Offline mode";
    } else {
        wifiDetail = "Needs setup";
    }
    recordBootStatus("Wi-Fi", wifiReady, wifiDetail);
    if (!wifiReady) {
        Logger::warn("Wi-Fi not connected at boot; continuing with offline fallback");
    }

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

        // Get current WiFi channel
        uint8_t currentChannel = 1;
        wifi_second_chan_t second = WIFI_SECOND_CHAN_NONE;
        esp_wifi_get_channel(&currentChannel, &second);
        
        JoinAcceptMessage accept;
        accept.node_id = nodeId;
        accept.light_id = nodes->getLightForNode(nodeId);
        accept.lmk = ""; // Unencrypted for now
        accept.wifi_channel = currentChannel; // Tell node which channel to use
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

    if (mqtt && wifi) {
        mqtt->setWifiManager(wifi);
    }

    Logger::info("Initializing MQTT...");
    bool mqttInitOk = mqtt->begin();
    bool mqttConnected = mqtt->isConnected();
    String brokerLabel = mqtt->getBrokerHost().isEmpty() ? String("auto") : mqtt->getBrokerHost();
    recordBootStatus("MQTT", mqttConnected, mqttConnected ? String("Connected ") + brokerLabel : String("Waiting on ") + brokerLabel);
    if (!mqttInitOk) {
        Logger::error("Failed to initialize MQTT");
        return false;
    }
    Logger::info("MQTT initialized successfully");
    mqtt->setCommandCallback([this](const String& topic, const String& payload) {
        this->handleMqttCommand(topic, payload);
    });

    Logger::info("Initializing mmWave sensor...");
    bool mmWaveOk = mmWave->begin();
    bool mmWaveOnline = mmWave->isOnline();
    recordBootStatus("mmWave", mmWaveOnline, mmWaveOnline ? "LD2450 streaming" : "will retry");
    if (!mmWaveOk) {
        Logger::warn("Failed to initialize mmWave sensor - continuing without it");
    } else if (!mmWaveOnline) {
        Logger::warn("mmWave sensor initialized but no stream detected - will retry in background");
    } else {
        Logger::info("mmWave initialized successfully");
    }

    Logger::info("Initializing node registry...");
    bool nodesOk = nodes->begin();
    recordBootStatus("Nodes", nodesOk, nodesOk ? "registry ready" : "init failed");
    if (!nodesOk) {
        Logger::error("Failed to initialize node registry");
        return false;
    }
    Logger::info("Node registry initialized successfully");
    publishLog("Node registry initialized successfully", "INFO", "setup");
    nodes->setNodeRegisteredCallback([this](const String& nodeId, const String& lightId) {
        Logger::info("Node %s paired to light %s", nodeId.c_str(), lightId.c_str());
        if (espNow) {
            espNow->disablePairingMode();
        }
        if (nodes && nodes->isPairingActive()) {
            nodes->stopPairing();
        }
        statusLed.pulse(0, 150, 0, 400);
    });

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

    bool zonesOk = zones->begin();
    recordBootStatus("Zones", zonesOk, zonesOk ? "control ready" : "init failed");
    if (!zonesOk) {
        Logger::error("Failed to initialize zone control");
        return false;
    }

    // Disable idle breathing; LEDs indicate node connections per requirements
    statusLed.setIdleBreathing(false);

    bool buttonsOk = buttons->begin();
    recordBootStatus("Button", buttonsOk, buttonsOk ? "GPIO ready" : "init failed");
    if (!buttonsOk) {
        Logger::error("Failed to initialize button control");
        return false;
    }

    bool thermalOk = thermal->begin();
    recordBootStatus("Thermal", thermalOk, thermalOk ? "monitoring" : "init failed");
    if (!thermalOk) {
        Logger::error("Failed to initialize thermal control");
        return false;
    }

    bool ambientOk = true;
    if (ambientLight && !ambientLight->begin()) {
        Logger::warn("TSL2561 ambient light sensor init failed (continuing)");
        ambientOk = false;
    }
    recordBootStatus("Ambient", ambientOk, ambientOk ? "TSL2561 ready" : "sensor offline");

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

printBootSummary();
Logger::info("Coordinator initialization complete");
Logger::info("==============================================");
Logger::info("System ready! Press BOOT button to pair nodes.");
Logger::info("Hold 4s to run wave test on paired nodes.");
logConnectedNodes();
Logger::info("==============================================");
return true;
}

void Coordinator::loop() {
    if (wifi) {
        wifi->loop();
    }
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

    refreshCoordinatorSensors();
    printSerialTelemetry();
}

void Coordinator::onMmWaveEvent(const MmWaveEvent& event) {
    lastMmWaveEvent = event;
    haveMmWaveSample = true;
    // Guard against null pointers
    if (!zones || !nodes || !thermal || !espNow || !mqtt) {
        Logger::error("Cannot process mmWave event: components not initialized");
        return;
    }
    
    // Publish raw mmWave frame to MQTT for backend/frontend consumption
    mqtt->publishMmWaveEvent(event);

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

    const uint32_t windowMs = 60000; // 60s pairing window
    startPairingWindow(windowMs, "button");
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
            updateNodeTelemetryCache(nodeId, *statusMsg);
            
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
    
    // Check for manual LED override timeout
    if (manualLedMode && manualLedTimeoutMs > 0 && now > manualLedTimeoutMs) {
        manualLedMode = false;
        Logger::info("Manual LED override timed out");
    }

    int groupCount = Pins::RgbLed::NUM_PIXELS / 4;
    for (int g = 0; g < groupCount; ++g) {
        uint8_t r=0,gc=0,b=0;
        
        if (manualLedMode) {
            // Manual override active
            r = manualR;
            gc = manualG;
            b = manualB;
        } else if (groupFlashUntilMs[g] > now) {
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

void Coordinator::handleMqttCommand(const String& topic, const String& payload) {
    StaticJsonDocument<256> doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        Logger::warn("Failed to parse MQTT command (%s)", err.c_str());
        return;
    }

    String cmd = doc["cmd"] | "";
    cmd.toLowerCase();
    if (cmd == "pair" || cmd == "pairing.start" || cmd == "enter_pairing_mode") {
        uint32_t windowMs = doc["duration_ms"] | 60000;
        startPairingWindow(windowMs, "mqtt");
    } else if (cmd == "pairing.stop") {
        if (nodes) nodes->stopPairing();
        if (espNow) espNow->disablePairingMode();
        Logger::info("Pairing window closed via MQTT command");
        Serial.println("Pairing window closed via MQTT command");
    } else if (cmd == "led.set") {
        manualR = doc["r"] | 0;
        manualG = doc["g"] | 0;
        manualB = doc["b"] | 0;
        uint32_t duration = doc["duration_ms"] | 0;
        
        manualLedMode = true;
        if (duration > 0) {
            manualLedTimeoutMs = millis() + duration;
        } else {
            manualLedTimeoutMs = 0; // Indefinite
        }
        Logger::info("Manual LED override: RGB(%d,%d,%d)", manualR, manualG, manualB);
        updateLeds();
    } else if (cmd == "led.reset") {
        manualLedMode = false;
        Logger::info("Manual LED override cleared");
        updateLeds();
    }
}

void Coordinator::startPairingWindow(uint32_t durationMs, const char* reason) {
    if (!nodes || !espNow) {
        return;
    }
    nodes->startPairing(durationMs);
    espNow->enablePairingMode(durationMs);
    const char* origin = reason ? reason : "manual";
    Logger::info("Pairing window (%s) open for %u ms", origin, durationMs);
    Serial.printf("PAIRING MODE (%s) OPEN for %lu ms\n", origin, (unsigned long)durationMs);
    String msg = "Pairing window (" + String(origin) + ") open for " + String(durationMs) + " ms";
    publishLog(msg, "INFO", "pairing");
    statusLed.pulse(0, 0, 180, 500);
}

void Coordinator::updateNodeTelemetryCache(const String& nodeId, const NodeStatusMessage& statusMsg) {
    NodeTelemetrySnapshot snapshot;
    snapshot.avgR = statusMsg.avg_r;
    snapshot.avgG = statusMsg.avg_g;
    snapshot.avgB = statusMsg.avg_b;
    snapshot.avgW = statusMsg.avg_w;
    snapshot.temperatureC = statusMsg.temperature;
    snapshot.buttonPressed = statusMsg.button_pressed;
    snapshot.lastUpdateMs = millis();
    nodeTelemetry[nodeId] = snapshot;

    if (mqtt) {
        mqtt->publishNodeStatus(statusMsg);
    }
}

void Coordinator::refreshCoordinatorSensors() {
    uint32_t now = millis();
    if (now - lastSensorSampleMs < 2000) {
        return;
    }
    lastSensorSampleMs = now;

    if (ambientLight) {
        coordinatorSensors.lightLux = ambientLight->readLux();
    }
    
    // Read ESP32 internal temperature sensor (if available)
    #ifdef SOC_TEMP_SENSOR_SUPPORTED
    coordinatorSensors.tempC = temperatureRead();
    #else
    coordinatorSensors.tempC = 0.0f;
    #endif
    
    coordinatorSensors.timestampMs = now;
    bool mmWaveOnline = mmWave && mmWave->isOnline();
    coordinatorSensors.mmWaveOnline = mmWaveOnline;
    if (haveMmWaveSample && mmWaveOnline) {
        coordinatorSensors.mmWavePresence = lastMmWaveEvent.presence;
        coordinatorSensors.mmWaveConfidence = lastMmWaveEvent.confidence;
    } else if (!mmWaveOnline) {
        coordinatorSensors.mmWavePresence = false;
        coordinatorSensors.mmWaveConfidence = 0.0f;
    }

    if (wifi) {
        WifiManager::Status wifiStatus = wifi->getStatus();
        coordinatorSensors.wifiConnected = wifiStatus.connected && !wifiStatus.offlineMode;
        coordinatorSensors.wifiRssi = wifiStatus.connected ? wifiStatus.rssi : -127;
    } else {
        coordinatorSensors.wifiConnected = WiFi.status() == WL_CONNECTED;
        coordinatorSensors.wifiRssi = coordinatorSensors.wifiConnected ? WiFi.RSSI() : -127;
    }

    if (mqtt) {
        mqtt->publishCoordinatorTelemetry(coordinatorSensors);
    }
}

void Coordinator::printSerialTelemetry() {
    uint32_t now = millis();
    if (now - lastSerialPrintMs < 3000) {
        return;
    }
    lastSerialPrintMs = now;

    WifiManager::Status wifiStatus;
    if (wifi) {
        wifiStatus = wifi->getStatus();
    } else {
        wifiStatus.connected = (WiFi.status() == WL_CONNECTED);
        wifiStatus.ssid = wifiStatus.connected ? WiFi.SSID() : "";
        wifiStatus.rssi = wifiStatus.connected ? WiFi.RSSI() : -127;
        wifiStatus.offlineMode = false;
    }

    bool mqttConnected = mqtt && mqtt->isConnected();
    String brokerHost = mqtt ? mqtt->getBrokerHost() : String("n/a");
    uint16_t brokerPort = mqtt ? mqtt->getBrokerPort() : 0;
    const char* pairingState = (nodes && nodes->isPairingActive()) ? "OPEN" : "IDLE";
    const char* mmStatus;
    if (!coordinatorSensors.mmWaveOnline) {
        mmStatus = "OFFLINE";
    } else {
        mmStatus = coordinatorSensors.mmWavePresence ? "PRESENT" : "CLEAR";
    }
    uint16_t mmRestarts = mmWave ? mmWave->getRestartCount() : 0;

    size_t activeNodes = 0;
    for (const auto& entry : nodeTelemetry) {
        if (now - entry.second.lastUpdateMs <= 30000) {
            activeNodes++;
        }
    }

    Serial.println();
    Serial.println("========== Coordinator Snapshot ==========");
    Serial.printf("Sensors   | Lux %5.1f\n",
                  coordinatorSensors.lightLux);
    Serial.printf("mmWave    | %-8s  conf=%.2f restarts=%u\n",
                  mmStatus,
                  coordinatorSensors.mmWaveConfidence,
                  static_cast<unsigned>(mmRestarts));
    if (!coordinatorSensors.mmWaveOnline) {
        Serial.println("           | sensor offline - verify LD2450 wiring (RX=GPIO44, TX=GPIO43, 3V3, GND)");
    }
    Serial.printf("Wi-Fi     | %-10s ssid=%s rssi=%d dBm offline=%s\n",
                  wifiStatus.connected ? "CONNECTED" : "DISCONNECTED",
                  wifiStatus.ssid.c_str(),
                  wifiStatus.rssi,
                  wifiStatus.offlineMode ? "true" : "false");
    Serial.printf("MQTT      | %-10s %s:%u\n",
                  mqttConnected ? "CONNECTED" : "RETRYING",
                  brokerHost.c_str(),
                  brokerPort);
    Serial.printf("Pairing   | %s\n", pairingState);
    if (activeNodes == 0) {
        Serial.println("Nodes     | none paired (mmWave + ambient-only mode)");
    } else {
        Serial.printf("Nodes     | %u active\n", static_cast<unsigned>(activeNodes));
        for (const auto& entry : nodeTelemetry) {
            const auto& data = entry.second;
            uint32_t age = now - data.lastUpdateMs;
            if (age > 30000) {
                continue;
            }
            Serial.printf("           - %s -> RGBW(%d,%d,%d,%d) temp=%.1f C btn=%s age=%lus\n",
                          entry.first.c_str(),
                          data.avgR,
                          data.avgG,
                          data.avgB,
                          data.avgW,
                          data.temperatureC,
                          data.buttonPressed ? "DOWN" : "up",
                          static_cast<unsigned long>(age / 1000));
        }
    }
    Serial.println("==========================================");
}

void Coordinator::recordBootStatus(const char* name, bool ok, const String& detail) {
    BootStatusEntry entry;
    entry.name = name ? name : "Subsystem";
    entry.ok = ok;
    entry.detail = detail;
    bootStatus.push_back(entry);
}

void Coordinator::publishLog(const String& message, const String& level, const String& tag) {
    if (mqtt && mqtt->isConnected()) {
        mqtt->publishSerialLog(message, level, tag);
    }
}

void Coordinator::printBootSummary() {
    if (bootStatus.empty()) {
        return;
    }
    Serial.println();
    Serial.println("┌────────────┬──────────────────────────────┐");
    Serial.println("│ Subsystem  │ Status                       │");
    Serial.println("├────────────┼──────────────────────────────┤");
    for (const auto& entry : bootStatus) {
        String detail = entry.detail;
        if (detail.isEmpty()) {
            detail = entry.ok ? "OK" : "See logs";
        }
        if (detail.length() > 28) {
            detail = detail.substring(0, 25) + "...";
        }
        String status = String(entry.ok ? "✓ " : "! ") + detail;
        Serial.printf("│ %-10s │ %-30s │\n", entry.name.c_str(), status.c_str());
    }
    Serial.println("└────────────┴──────────────────────────────┘");
    Serial.println();
}

