#include "Mqtt.h"
#include "../utils/Logger.h"
#include <ArduinoJson.h>

// Static instance pointer for callback
static Mqtt* mqttInstance = nullptr;

namespace {
    constexpr uint16_t DEFAULT_MQTT_PORT = 1883;

    bool waitForConsole(uint32_t timeoutMs = 0) {
        if (Serial) {
            return true;
        }
        uint32_t start = millis();
        while (!Serial) {
            if (timeoutMs > 0 && (millis() - start) > timeoutMs) {
                return false;
            }
            delay(10);
        }
        return true;
    }

    void flushSerialInput() {
        while (Serial.available()) {
            Serial.read();
        }
    }

    String promptLine(const String& prompt, bool allowEmpty, const String& defaultValue = "") {
        if (!waitForConsole()) {
            return defaultValue;
        }
        while (true) {
            if (defaultValue.length() > 0) {
                Serial.printf("%s [%s]: ", prompt.c_str(), defaultValue.c_str());
            } else {
                Serial.printf("%s: ", prompt.c_str());
            }
            Serial.flush();
            while (!Serial.available()) {
                delay(10);
            }
            String line = Serial.readStringUntil('\n');
            line.trim();
            if (line.isEmpty() && defaultValue.length() > 0) {
                line = defaultValue;
            }
            if (!line.isEmpty() || allowEmpty) {
                return line;
            }
            Serial.println("Value required. Please try again.");
        }
    }

    bool promptYesNo(const String& prompt, bool defaultYes = true) {
        String suffix = defaultYes ? "Y/n" : "y/N";
        while (true) {
            String answer = promptLine(prompt + " (" + suffix + ")", true, defaultYes ? "y" : "n");
            answer.toLowerCase();
            if (answer == "y" || answer == "yes") return true;
            if (answer == "n" || answer == "no") return false;
            Serial.println("Please answer with 'y' or 'n'.");
        }
    }

    bool isLoopbackHost(String host) {
        host.trim();
        host.toLowerCase();
        return host == "localhost" || host == "127.0.0.1" || host == "::1";
    }
}

Mqtt::Mqtt() 
    : mqttClient(wifiClient)
    , config("mqtt")
    , brokerPort(DEFAULT_MQTT_PORT)
    , wifiManager(nullptr) {
    mqttInstance = this;
}

Mqtt::~Mqtt() {
    mqttClient.disconnect();
    WiFi.disconnect();
}

bool Mqtt::begin() {
    Logger::info("Initializing MQTT client...");
    
    if (!config.begin()) {
        Logger::warn("MQTT config namespace missing; provisioning new store");
    }

    if (!ensureConfigLoaded()) {
        if (brokerHost.isEmpty()) {
            brokerHost = "192.168.1.100";
        }
        if (siteId.isEmpty()) {
            siteId = "site001";
        }
        Logger::warn("Using fallback MQTT endpoint %s:%u (update via provisioning)", brokerHost.c_str(), brokerPort);
    }

    warnIfLoopbackHost();

    if (coordId.isEmpty()) {
        coordId = WiFi.macAddress();
    }

    // Setup MQTT client
    mqttClient.setServer(brokerHost.c_str(), brokerPort);
    mqttClient.setCallback(handleMqttMessage);
    Logger::info("MQTT broker target set to %s:%u", brokerHost.c_str(), brokerPort);
    
    // Connect to MQTT broker
    if (!connectMqtt()) {
        Logger::warn("Failed initial MQTT connection (will retry)");
        // Non-fatal - will retry in loop()
    }
    
    Logger::info("MQTT initialization complete");
    return true;
}

void Mqtt::loop() {
    bool wifiReady = wifiManager ? wifiManager->isConnected() : (WiFi.status() == WL_CONNECTED);

    if (!wifiReady) {
        if (mqttClient.connected()) {
            mqttClient.disconnect();
            Logger::warn("MQTT disconnected (Wi-Fi unavailable)");
        }
        return;
    }

    if (!mqttClient.connected()) {
        static uint32_t lastReconnect = 0;
        uint32_t now = millis();
        if (now - lastReconnect > 5000) {
            lastReconnect = now;
            connectMqtt();
        }
    }

    mqttClient.loop();
}

bool Mqtt::isConnected() {
    return mqttClient.connected();
}

void Mqtt::publishLightState(const String& lightId, uint8_t brightness) {
    if (!mqttClient.connected()) return;
    
    StaticJsonDocument<256> doc;
    doc["ts"] = millis() / 1000;
    doc["light_id"] = lightId;
    doc["brightness"] = brightness;
    
    String payload;
    serializeJson(doc, payload);
    
    String topic = nodeTelemetryTopic(lightId);
    mqttClient.publish(topic.c_str(), payload.c_str());
}

// PRD-compliant: site/{siteId}/node/{nodeId}/telemetry
void Mqtt::publishThermalEvent(const String& nodeId, const NodeThermalData& data) {
    if (!mqttClient.connected()) return;
    
    StaticJsonDocument<512> doc;
    doc["ts"] = millis() / 1000;
    doc["node_id"] = nodeId;
    doc["temp_c"] = data.temperature;
    doc["is_derated"] = data.isDerated;
    doc["deration_level"] = data.derationLevel;
    
    String payload;
    serializeJson(doc, payload);
    
    String topic = nodeTelemetryTopic(nodeId);
    mqttClient.publish(topic.c_str(), payload.c_str());
    
    Logger::info("Published thermal event for node %s", nodeId.c_str());
}

// PRD-compliant: site/{siteId}/coord/{coordId}/mmwave
void Mqtt::publishMmWaveEvent(const MmWaveEvent& event) {
    if (!mqttClient.connected()) return;
    // Build rich target payload (backward compatible: includes legacy "events" array)
    StaticJsonDocument<1024> doc;
    doc["ts"] = event.timestampMs / 1000;
    String coord = coordId.length() ? coordId : WiFi.macAddress();
    doc["site_id"] = siteId;
    doc["coord_id"] = coord;
    doc["sensor_id"] = event.sensorId;
    doc["presence"] = event.presence;
    doc["confidence"] = event.confidence;
    // Legacy simplified presence format
    JsonArray legacy = doc.createNestedArray("events");
    {
        JsonObject evt = legacy.createNestedObject();
        int zone = 0;
        // try parse numeric sensor id as zone
        for (size_t i = 0; i < event.sensorId.length(); ++i) {
            if (!isDigit(event.sensorId[i])) { zone = 0; break; }
        }
        if (zone == 0) {
            zone = 1; // default single-zone
        }
        evt["zone"] = zone;
        evt["presence"] = event.presence;
        evt["confidence"] = event.confidence;
    }
    JsonArray targets = doc.createNestedArray("targets");
    for (const auto &t : event.targets) {
        if (!t.valid) continue; // only publish valid targets
        JsonObject o = targets.createNestedObject();
        o["id"] = t.id;
        o["distance_mm"] = t.distance_mm;
        JsonObject pos = o.createNestedObject("position_mm");
        pos["x"] = t.x_mm;
        pos["y"] = t.y_mm;
        pos["z"] = 0;
        JsonObject vel = o.createNestedObject("velocity_m_s");
        vel["x"] = t.vx_m_s;
        vel["y"] = t.vy_m_s;
        vel["z"] = 0.0f;
        o["speed_cm_s"] = t.speed_cm_s;
        o["resolution_mm"] = t.resolution_mm;
    }
    String payload;
    serializeJson(doc, payload);
    String topic = coordinatorMmwaveTopic();
    mqttClient.publish(topic.c_str(), payload.c_str());
    Logger::info("Published mmWave frame (%d targets)", targets.size());
}

void Mqtt::publishNodeStatus(const NodeStatusMessage& status) {
    if (!mqttClient.connected()) return;
    
    StaticJsonDocument<1024> doc;
    doc["ts"] = millis() / 1000;
    doc["node_id"] = status.node_id;
    doc["light_id"] = status.light_id;
    doc["avg_r"] = status.avg_r;
    doc["avg_g"] = status.avg_g;
    doc["avg_b"] = status.avg_b;
    doc["avg_w"] = status.avg_w;
    doc["status_mode"] = status.status_mode;
    doc["temp_c"] = status.temperature;
    doc["button_pressed"] = status.button_pressed;
    doc["vbat_mv"] = status.vbat_mv;
    doc["fw"] = status.fw;
    
    String payload;
    serializeJson(doc, payload);
    
    String topic = nodeTelemetryTopic(status.node_id);
    mqttClient.publish(topic.c_str(), payload.c_str());
}

void Mqtt::setBrokerConfig(const char* host, uint16_t port, const char* username, const char* password) {
    brokerHost = host;
    brokerPort = port;
    brokerUsername = username;
    brokerPassword = password;
    loopbackHintPrinted = false;
    warnIfLoopbackHost();
    persistConfig();
}

void Mqtt::setWifiManager(WifiManager* manager) {
    wifiManager = manager;
}

void Mqtt::setCommandCallback(std::function<void(const String& topic, const String& payload)> callback) {
    commandCallback = callback;
}

bool Mqtt::connectMqtt() {
    if (mqttClient.connected()) {
        return true;
    }
    bool wifiReady = wifiManager ? wifiManager->ensureConnected() : (WiFi.status() == WL_CONNECTED);
    if (!wifiReady) {
        Logger::warn("MQTT connect skipped - Wi-Fi unavailable");
        return false;
    }
    
    warnIfLoopbackHost();
    Logger::info("Connecting to MQTT broker: %s:%d", brokerHost.c_str(), brokerPort);
    
    if (coordId.isEmpty()) {
        coordId = WiFi.macAddress();
    }
    String clientId = "coord-" + coordId;
    bool connected = false;
    
    if (brokerUsername.length() > 0 && brokerPassword.length() > 0) {
        connected = mqttClient.connect(clientId.c_str(), brokerUsername.c_str(), brokerPassword.c_str());
    } else {
        connected = mqttClient.connect(clientId.c_str());
    }
    
    if (connected) {
        Logger::info("MQTT connected!");
        
        // Subscribe to coordinator commands (PRD-compliant)
        String cmdTopic = coordinatorCmdTopic();
        mqttClient.subscribe(cmdTopic.c_str());
        
        Logger::info("Subscribed to: %s", cmdTopic.c_str());
        
        // Publish initial telemetry
        CoordinatorSensorSnapshot snapshot;
        snapshot.timestampMs = millis();
        snapshot.wifiConnected = true;
        snapshot.wifiRssi = WiFi.RSSI();
        publishCoordinatorTelemetry(snapshot);
        
        return true;
    }
    
    int8_t state = mqttClient.state();
    logConnectionFailureDetail(state);
    if (!discoveryAttempted && autoDiscoverBroker()) {
        Logger::info("Retrying MQTT connection using %s:%u", brokerHost.c_str(), brokerPort);
        mqttClient.setServer(brokerHost.c_str(), brokerPort);
        return connectMqtt();
    }
    return false;
}

bool Mqtt::ensureConfigLoaded() {
    configLoaded = loadConfigFromStore();
    if (configLoaded) {
        return true;
    }

    if (!discoveryAttempted && autoDiscoverBroker()) {
        Logger::info("Discovered MQTT broker at %s", brokerHost.c_str());
        if (siteId.isEmpty()) {
            siteId = "site001";
        }
        persistConfig();
        configLoaded = true;
        return true;
    }

    Serial.println();
    Serial.println("===========================================");
    Serial.println("MQTT broker settings not found in NVS.");
    Serial.println("The coordinator needs the Docker host IP to reach MQTT.");
    Serial.println("===========================================");
    if (!promptYesNo("Configure MQTT broker now?", true)) {
        Logger::warn("MQTT provisioning skipped by operator");
        return false;
    }

    if (!runProvisioningWizard()) {
        Logger::error("MQTT provisioning failed (using defaults)");
        return false;
    }

    configLoaded = loadConfigFromStore();
    return configLoaded;
}

bool Mqtt::loadConfigFromStore() {
    brokerHost = config.getString("broker_host", "");
    brokerHost.trim();
    brokerPort = static_cast<uint16_t>(config.getInt("broker_port", DEFAULT_MQTT_PORT));
    if (brokerPort == 0) {
        brokerPort = DEFAULT_MQTT_PORT;
    }
    brokerUsername = config.getString("broker_user", "user1");
    brokerPassword = config.getString("broker_pass", "user1");
    siteId = config.getString("site_id", "site001");
    siteId.trim();
    coordId = config.getString("coord_id", "");
    coordId.trim();

    bool ready = !brokerHost.isEmpty() && !siteId.isEmpty();
    return ready;
}

bool Mqtt::runProvisioningWizard() {
    if (!waitForConsole(2000)) {
        Logger::warn("Serial console not available for MQTT provisioning");
        return false;
    }

    flushSerialInput();
    Serial.println();
    Serial.println("=== MQTT Broker Setup ===");
    Serial.println("Enter the IP of the machine running docker-compose (ex. 10.0.0.42).");
    Serial.println("Do NOT enter 'localhost' because the coordinator is on Wi-Fi.");

    String host;
    while (true) {
        host = promptLine("Broker host/IP", false, brokerHost.isEmpty() ? "192.168.1.100" : brokerHost);
        if (isLoopbackHost(host)) {
            Serial.println("Loopback addresses won't work. Please enter the LAN IP of the Docker host.");
            continue;
        }
        break;
    }

    String portStr = promptLine("Port", true, String(brokerPort == 0 ? DEFAULT_MQTT_PORT : brokerPort));
    uint16_t portCandidate = portStr.isEmpty() ? DEFAULT_MQTT_PORT : static_cast<uint16_t>(portStr.toInt());
    if (portCandidate == 0) {
        portCandidate = DEFAULT_MQTT_PORT;
    }

    String user = promptLine("Username (leave empty for anonymous)", true, brokerUsername);
    String pass = promptLine("Password", true, brokerPassword);

    String site = promptLine("Site ID", false, siteId.isEmpty() ? "site001" : siteId);
    String coord = promptLine("Coordinator ID (blank = use MAC)", true, coordId);

    brokerHost = host;
    brokerPort = portCandidate;
    brokerUsername = user;
    brokerPassword = pass;
    siteId = site;
    coordId = coord;

    loopbackHintPrinted = false;
    warnIfLoopbackHost();

    persistConfig();
    Serial.println("MQTT settings saved to NVS.");
    Serial.println();
    return true;
}

void Mqtt::persistConfig() {
    config.setString("broker_host", brokerHost);
    config.setInt("broker_port", brokerPort);
    config.setString("broker_user", brokerUsername);
    config.setString("broker_pass", brokerPassword);
    config.setString("site_id", siteId);
    if (!coordId.isEmpty()) {
        config.setString("coord_id", coordId);
    } else {
        config.remove("coord_id");
    }
}

bool Mqtt::autoDiscoverBroker() {
    discoveryAttempted = true;
    bool wifiReady = wifiManager ? wifiManager->ensureConnected() : (WiFi.status() == WL_CONNECTED);
    if (!wifiReady) {
        Logger::warn("MQTT autodiscovery skipped - Wi-Fi unavailable");
        return false;
    }

    IPAddress local = WiFi.localIP();
    IPAddress subnet = WiFi.subnetMask();
    IPAddress gateway = WiFi.gatewayIP();

    if ((uint32_t)local == 0 || (uint32_t)subnet == 0) {
        Logger::warn("MQTT autodiscovery aborted - invalid IP context");
        return false;
    }

    if (gateway && tryBrokerCandidate(gateway)) {
        brokerHost = gateway.toString();
        persistConfig();
        return true;
    }

    uint32_t ip = (uint32_t)local;
    uint32_t mask = (uint32_t)subnet;
    uint32_t network = ip & mask;
    uint32_t broadcast = network | ~mask;
    if (broadcast <= network + 1) {
        return false;
    }

    uint32_t hostCount = broadcast - network - 1;
    const uint32_t maxScan = 256; // limit scan time
    if (hostCount > maxScan) {
        hostCount = maxScan;
    }

    Logger::info("Scanning %u LAN hosts for MQTT (port %u)...", hostCount, brokerPort);
    uint32_t start = network + 1;
    uint32_t localIndex = (ip > start) ? (ip - start) : 0;

    for (uint32_t i = 0; i < hostCount; ++i) {
        uint32_t offset = (localIndex + i) % hostCount;
        IPAddress candidate(start + offset);
        if (candidate == local || candidate == gateway) {
            continue;
        }
        if (tryBrokerCandidate(candidate)) {
            brokerHost = candidate.toString();
            persistConfig();
            Logger::info("Auto-discovered MQTT broker at %s", brokerHost.c_str());
            return true;
        }
    }

    Logger::warn("No MQTT broker responded on the local subnet");
    return false;
}

bool Mqtt::tryBrokerCandidate(const IPAddress& candidate) {
    WiFiClient probe;
    constexpr uint32_t timeoutMs = 250;
    if (!probe.connect(candidate, brokerPort, timeoutMs)) {
        return false;
    }
    probe.stop();
    return true;
}

void Mqtt::logConnectionFailureDetail(int8_t state) {
    const char* description = describeMqttState(state);
    Logger::error("MQTT connection failed, rc=%d (%s)", state, description);
    warnIfLoopbackHost();

    uint32_t now = millis();
    if (state == lastFailureState && (now - lastDiagPrintMs) < 30000) {
        return;
    }
    lastFailureState = state;
    lastDiagPrintMs = now;

    switch (state) {
        case MQTT_CONNECT_FAILED:
        case MQTT_CONNECTION_TIMEOUT:
        case MQTT_CONNECTION_LOST:
            runReachabilityProbe();
            break;
        case MQTT_CONNECT_BAD_CREDENTIALS:
        case MQTT_CONNECT_UNAUTHORIZED:
            Logger::warn("MQTT broker rejected credentials. Update ConfigManager 'mqtt' user/pass or adjust mosquitto ACLs.");
            break;
        case MQTT_CONNECT_BAD_CLIENT_ID:
            Logger::warn("MQTT broker rejected coordinator ID. Set a unique Coordinator ID during provisioning.");
            break;
        case MQTT_CONNECT_UNAVAILABLE:
            Logger::warn("MQTT broker reported itself unavailable. Ensure the Mosquitto container is running and listening on 0.0.0.0:%u.", brokerPort);
            break;
        default:
            break;
    }
}

const char* Mqtt::describeMqttState(int8_t state) const {
    switch (state) {
        case MQTT_CONNECTION_TIMEOUT: return "connection timeout";
        case MQTT_CONNECTION_LOST: return "connection lost";
        case MQTT_CONNECT_FAILED: return "TCP connection failed";
        case MQTT_DISCONNECTED: return "disconnected";
        case MQTT_CONNECTED: return "connected";
        case MQTT_CONNECT_BAD_PROTOCOL: return "bad protocol";
        case MQTT_CONNECT_BAD_CLIENT_ID: return "client ID rejected";
        case MQTT_CONNECT_UNAVAILABLE: return "server unavailable";
        case MQTT_CONNECT_BAD_CREDENTIALS: return "bad credentials";
        case MQTT_CONNECT_UNAUTHORIZED: return "unauthorized";
        default: return "unknown";
    }
}

void Mqtt::warnIfLoopbackHost() {
    if (!brokerHost.length()) {
        return;
    }
    if (isLoopbackHost(brokerHost)) {
        if (!loopbackHintPrinted) {
            Logger::warn("MQTT host %s is a loopback address. Use the LAN IP of the Docker host (ex. 192.168.x.x).", brokerHost.c_str());
            loopbackHintPrinted = true;
        }
        return;
    }
    loopbackHintPrinted = false;
}

void Mqtt::runReachabilityProbe() {
    if (brokerHost.isEmpty()) {
        return;
    }

    Logger::info("Probing TCP reachability to %s:%u...", brokerHost.c_str(), brokerPort);
    WiFiClient probe;
    probe.setTimeout(1000);
    if (!probe.connect(brokerHost.c_str(), brokerPort)) {
        Logger::error("Unable to open TCP socket to %s:%u. Ensure docker-compose exposes Mosquitto on 0.0.0.0:%u and Windows firewall permits inbound connections.",
                      brokerHost.c_str(), brokerPort, brokerPort);
        if (wifiManager) {
            WifiManager::Status status = wifiManager->getStatus();
            String ip = status.ip.toString();
            Logger::info("Wi-Fi context: %s (offline=%s) ip=%s", status.connected ? "connected" : "disconnected",
                         status.offlineMode ? "true" : "false",
                         ip.c_str());
        }
        return;
    }

    Logger::info("TCP port responded but MQTT handshake still failed. Confirm mosquitto.conf allows the configured credentials or enable anonymous access for testing.");
    probe.stop();
}

void Mqtt::handleMqttMessage(char* topic, uint8_t* payload, unsigned int length) {
    if (mqttInstance && mqttInstance->commandCallback) {
        String topicStr = String(topic);
        String payloadStr = String((char*)payload, length);
        mqttInstance->processMessage(topicStr, payloadStr);
    }
}

void Mqtt::processMessage(const String& topic, const String& payload) {
    Logger::info("MQTT message received: %s", topic.c_str());
    
    if (commandCallback) {
        commandCallback(topic, payload);
    }
}

void Mqtt::publishCoordinatorTelemetry(const CoordinatorSensorSnapshot& snapshot) {
    if (!mqttClient.connected()) return;
    StaticJsonDocument<256> doc;
    uint32_t ts = snapshot.timestampMs ? snapshot.timestampMs : millis();
    doc["ts"] = ts / 1000;
    doc["light_lux"] = snapshot.lightLux;
    doc["temp_c"] = snapshot.temperatureC;
    doc["mmwave_presence"] = snapshot.mmWavePresence;
    doc["mmwave_confidence"] = snapshot.mmWaveConfidence;
    doc["mmwave_online"] = snapshot.mmWaveOnline;
    doc["wifi_rssi"] = snapshot.wifiConnected ? snapshot.wifiRssi : -127;
    doc["wifi_connected"] = snapshot.wifiConnected;
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(coordinatorTelemetryTopic().c_str(), payload.c_str());
}

String Mqtt::nodeTelemetryTopic(const String& nodeId) const {
    return "site/" + siteId + "/node/" + nodeId + "/telemetry";
}

String Mqtt::coordinatorTelemetryTopic() const {
    String id = coordId.length() ? coordId : WiFi.macAddress();
    return "site/" + siteId + "/coord/" + id + "/telemetry";
}

String Mqtt::coordinatorCmdTopic() const {
    String id = coordId.length() ? coordId : WiFi.macAddress();
    return "site/" + siteId + "/coord/" + id + "/cmd";
}

String Mqtt::coordinatorMmwaveTopic() const {
    String id = coordId.length() ? coordId : WiFi.macAddress();
    return "site/" + siteId + "/coord/" + id + "/mmwave";
}
