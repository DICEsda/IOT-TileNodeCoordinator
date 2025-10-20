#include "Coordinator.h"
#include "../utils/Logger.h"
#include "../../shared/src/EspNowMessage.h"

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
    Logger::setMinLevel(Logger::DEBUG); // Temporarily DEBUG to see RX activity
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

        // Add as ESP-NOW peer via ESPNowW (unencrypted)
        if (!espNow->addPeer(mac)) {
            Logger::error("Failed to add ESP-NOW peer for %s", macStr);
        }

        JoinAcceptMessage accept;
        accept.node_id = nodeId;
        accept.light_id = nodes->getLightForNode(nodeId);
        accept.lmk = ""; // Using unencrypted ESP-NOW in ESPNowW path
        String json = accept.toJson();

        // send back to node mac
        if (!espNow->sendToMac(mac, json)) {
            Logger::warn("Failed to send join_accept to %s", macStr);
        } else {
            Logger::info("Sent join_accept to %s", macStr);
        }
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

    // Hook node registered callback with enhanced LED feedback
    nodes->setNodeRegisteredCallback([this](const String& nodeId, const String& lightId) {
        // Triple flash bright green when a node successfully registers!
        Logger::info("✓ NODE PAIRED! Node: %s -> Light: %s", nodeId.c_str(), lightId.c_str());
        
        // Flash green 3 times quickly
        for (int i = 0; i < 3; i++) {
            statusLed.setAll(0, 255, 0);  // Bright green
            delay(150);
            statusLed.setAll(0, 0, 0);     // Off
            delay(150);
        }
        
        // Then return to pairing mode LED (pulsing blue) if still pairing
        if (nodes->isPairingActive()) {
            statusLed.pulse(0, 100, 255, 60000); // Back to blue pulse
        } else {
            statusLed.setAll(10, 10, 10); // Idle state
        }
    });

    if (!zones->begin()) {
        Logger::error("Failed to initialize zone control");
        return false;
    }

    // Do NOT auto-open pairing window on boot; pairing must be manual via button
    statusLed.setIdleBreathing(true); // idle warm white breathing

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

    Logger::info("Coordinator initialization complete");
    Logger::info("==============================================");
    Logger::info("System ready! Press button to pair new nodes.");
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
    
    // Update status LED (non-blocking)
    statusLed.loop();
    
    // Maintain LED state based on pairing
    if (nodes && !nodes->isPairingActive()) {
        statusLed.setIdleBreathing(true);
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
    
    // Short press opens a pairing window; release does nothing special
    if (pressed) {
        Logger::info("Pairing button pressed - opening pairing window");
        const uint32_t windowMs = 60000; // 60s pairing window
        nodes->startPairing(windowMs);
        espNow->enablePairingMode(windowMs);
        statusLed.setIdleBreathing(false);
        statusLed.pulse(0, 100, 255, windowMs); // Blue pulse while pairing
        Logger::info("Pairing window open for %d ms", windowMs);
    }
}

void Coordinator::handleNodeMessage(const String& nodeId, const uint8_t* data, size_t len) {
    Logger::info("Received message from node %s: %d bytes", nodeId.c_str(), len);
    
    // Parse the JSON payload
    String payload((const char*)data, len);
    Logger::debug("Payload: %s", payload.c_str());
    
    // TODO: Add message handling logic here based on message type
    // For now, just log the received message
}
