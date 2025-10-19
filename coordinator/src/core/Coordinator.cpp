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
    delete espNow;
    delete mqtt;
    delete mmWave;
    delete nodes;
    delete zones;
    delete buttons;
    delete thermal;
}

bool Coordinator::begin() {
    Logger::begin(115200);
    delay(500);
    
    Logger::info("=== Smart Tile Coordinator Starting ===");
    Logger::info("Initializing components...");

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

    // Register pairing callback to handle join requests coming from nodes
    espNow->setPairingCallback([this](const uint8_t* mac, const uint8_t* data, size_t len) {
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

        uint8_t lmk[16];
        esp_fill_random(lmk, sizeof(lmk));

        // Configure the esp-now peer on coordinator
        esp_now_del_peer(mac);
        esp_now_peer_info_t peer{};
        memcpy(peer.peer_addr, mac, 6);
        peer.channel = 0;
        peer.encrypt = true;
        memcpy(peer.lmk, lmk, sizeof(lmk));
        if (esp_now_add_peer(&peer) != ESP_OK) {
            Logger::error("Failed to add esp-now peer for %s", macStr);
        } else {
            Logger::info("Added esp-now peer for %s", macStr);
        }

        esp_err_t sres = esp_now_set_pmk(lmk);
        if (sres != ESP_OK) {
            Logger::warn("esp_now_set_pmk returned %d", sres);
        }

        JoinAcceptMessage accept;
        accept.node_id = nodeId;
        accept.light_id = nodes->getLightForNode(nodeId);
        char hexbuf[33];
        for (int i = 0; i < 16; ++i) snprintf(&hexbuf[i*2], 3, "%02X", lmk[i]);
        accept.lmk = String(hexbuf);
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

    // Open a short pairing window on boot so users can quickly pair tiles
    constexpr uint32_t BOOT_PAIRING_MS = 10000;
    nodes->startPairing(BOOT_PAIRING_MS);
    espNow->enablePairingMode();
    // Slow pulsing cyan/blue for the boot pairing window
    statusLed.pulse(0, 150, 255, BOOT_PAIRING_MS);
    Logger::info("Boot pairing window open for 10 seconds...");

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
    espNow->loop();
    mqtt->loop();
    mmWave->loop();
    nodes->loop();
    zones->loop();
    buttons->loop();
    thermal->loop();
    // Update status LED (non-blocking)
    statusLed.loop();
}

void Coordinator::onMmWaveEvent(const MmWaveEvent& event) {
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
    // Handle button events (pairing, etc)
    if (pressed) {
        Logger::info("Button pressed - ENTERING pairing mode");
        
        // Enter pairing mode (stays active while button is held)
        nodes->startPairing(60000); // 60 second timeout as safety
        espNow->enablePairingMode();
        
        // Pulsing blue LED indicates pairing mode is active and waiting for nodes
        statusLed.pulse(0, 100, 255, 60000); // Blue pulsing
        
        Logger::info("Pairing mode ACTIVE - waiting for nodes to pair...");
    } else {
        Logger::info("Button released - EXITING pairing mode");
        
        // Stop pairing when button is released
        nodes->stopPairing();
        espNow->disablePairingMode();
        
        // Turn LED off or show idle state (dim white)
        statusLed.setAll(10, 10, 10); // Dim white for idle
        
        Logger::info("Pairing mode CLOSED");
    }
}
