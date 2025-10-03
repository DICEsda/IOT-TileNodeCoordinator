#include "Coordinator.h"
#include "../utils/Logger.h"

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

    espNow = new EspNow();
    mqtt = new Mqtt();
    mmWave = new MmWave();
    nodes = new NodeRegistry();
    zones = new ZoneControl();
    buttons = new ButtonControl();
    thermal = new ThermalControl();

    // Initialize all components
    if (!espNow->begin()) {
        Logger::error("Failed to initialize ESP-NOW");
        return false;
    }

    if (!mqtt->begin()) {
        Logger::error("Failed to initialize MQTT");
        return false;
    }

    if (!mmWave->begin()) {
        Logger::error("Failed to initialize mmWave sensor");
        return false;
    }

    if (!nodes->begin()) {
        Logger::error("Failed to initialize node registry");
        return false;
    }

    if (!zones->begin()) {
        Logger::error("Failed to initialize zone control");
        return false;
    }

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
        nodes->startPairing();
        espNow->enablePairingMode();
    }
}
