#include "Mqtt.h"
#include "../utils/Logger.h"

Mqtt::Mqtt() : mqttClient(wifiClient) {}

Mqtt::~Mqtt() {}

bool Mqtt::begin() {
    Logger::info("Mqtt shim initialized (no-op)");
    return true;
}

void Mqtt::loop() {
    // no-op shim
}

bool Mqtt::isConnected() const { return false; }

void Mqtt::publishLightState(const String& lightId, uint8_t brightness) {
    // no-op
}

void Mqtt::publishThermalEvent(const String& nodeId, const NodeThermalData& data) {
    // no-op
}

void Mqtt::publishMmWaveEvent(const MmWaveEvent& event) {
    // no-op
}

void Mqtt::publishNodeStatus(const String& nodeId, const NodeInfo& status) {
    // no-op
}

void Mqtt::setWifiCredentials(const char* ssid, const char* password) {}
void Mqtt::setBrokerConfig(const char* host, uint16_t port, const char* username, const char* password) {}
void Mqtt::setCommandCallback(void (*callback)(const String& topic, const String& payload)) {}
