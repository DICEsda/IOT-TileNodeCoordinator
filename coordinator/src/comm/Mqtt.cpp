#include "Mqtt.h"
#include "MqttLogger.h"
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
        coordId = "coord001";  // Default to coord001 for frontend compatibility
        Logger::info("No coordinator ID set, using default: coord001");
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
            MqttLogger::logDisconnect(-1); // WiFi lost
        }
        return;
    }

    if (!mqttClient.connected()) {
        static uint32_t lastReconnect = 0;
        static uint32_t failedAttempts = 0;
        uint32_t now = millis();
        if (now - lastReconnect > 5000) {
            lastReconnect = now;
            if (!connectMqtt()) {
                failedAttempts++;
                // After 6 failed attempts (30 seconds), try rediscovery
                if (failedAttempts >= 6) {
                    Logger::info("Multiple MQTT failures - attempting rediscovery");
                    discoveryAttempted = false; // Reset flag to allow rediscovery
                    failedAttempts = 0;
                }
            } else {
                failedAttempts = 0; // Reset on success
            }
        }
    }

    mqttClient.loop();
    
    // Periodic heartbeat logging (every 60 seconds)
    MqttLogger::logHeartbeat(mqttClient.connected(), 60000);
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
    if (!mqttClient.connected()) {
        MqttLogger::logPublish("node_telemetry", "", false, 0);
        return;
    }
    
    uint32_t startMs = millis();
    
    StaticJsonDocument<1024> doc;
    doc["ts"] = startMs / 1000;
    doc["node_id"] = status.node_id.c_str();
    doc["light_id"] = status.light_id.c_str();
    doc["avg_r"] = status.avg_r;
    doc["avg_g"] = status.avg_g;
    doc["avg_b"] = status.avg_b;
    doc["avg_w"] = status.avg_w;
    doc["status_mode"] = status.status_mode.length() > 0 ? status.status_mode.c_str() : "idle";
    doc["temp_c"] = status.temperature;
    doc["button_pressed"] = status.button_pressed;
    doc["vbat_mv"] = status.vbat_mv;
    doc["fw"] = status.fw.length() > 0 ? status.fw.c_str() : "";
    
    String payload;
    serializeJson(doc, payload);
    
    String topic = nodeTelemetryTopic(status.node_id);
    bool success = mqttClient.publish(topic.c_str(), payload.c_str());
    
    // Detailed logging
    MqttLogger::logPublish(topic, payload, success, payload.length());
    MqttLogger::logLatency("NodeStatus", startMs);
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
    
    // Check if stored broker IP is on same subnet, if not trigger rediscovery
    if (!brokerHost.isEmpty() && configLoaded) {
        IPAddress brokerIP;
        if (brokerIP.fromString(brokerHost)) {
            IPAddress local = WiFi.localIP();
            IPAddress mask = WiFi.subnetMask();
            uint32_t localNet = (uint32_t)local & (uint32_t)mask;
            uint32_t brokerNet = (uint32_t)brokerIP & (uint32_t)mask;
            if (localNet != brokerNet) {
                Logger::warn("Broker %s not on current subnet - triggering rediscovery", brokerHost.c_str());
                discoveryAttempted = false;
            }
        }
    }
    
    warnIfLoopbackHost();
    
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
    
    // Log connection result with detailed info
    MqttLogger::logConnect(brokerHost, brokerPort, clientId, connected);
    
    if (connected) {
        // Subscribe to coordinator commands (PRD-compliant)
        String cmdTopic = coordinatorCmdTopic();
        bool subSuccess = mqttClient.subscribe(cmdTopic.c_str());
        MqttLogger::logSubscribe(cmdTopic, subSuccess);
        
        // Subscribe to node commands (wildcard) for light control forwarding
        String nodeCmd = "site/" + siteId + "/node/+/cmd";
        bool nodeSubSuccess = mqttClient.subscribe(nodeCmd.c_str());
        MqttLogger::logSubscribe(nodeCmd, nodeSubSuccess);
        
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

    // Priority 1: Try gateway (often the mobile hotspot host running Docker)
    if (gateway && (uint32_t)gateway != 0) {
        Logger::info("Trying gateway %s as MQTT broker...", gateway.toString().c_str());
        if (tryBrokerCandidate(gateway)) {
            brokerHost = gateway.toString();
            persistConfig();
            Logger::info("MQTT broker found at gateway: %s", brokerHost.c_str());
            return true;
        }
    }

    // Priority 2: Scan local subnet (limit to reasonable range)
    // We iterate the last octet (1-254) to avoid endianness issues with uint32_t arithmetic on ESP32
    const uint32_t hostCount = 254;

    Logger::info("Scanning %u nearby hosts for MQTT (this may take 15-30s)...", hostCount);
    
    for (uint32_t i = 1; i <= hostCount; ++i) {
        IPAddress candidate = local;
        candidate[3] = i; // Iterate the last octet
        
        if (candidate == local || candidate == gateway) {
            continue;
        }
        // Provide visual feedback every 20 hosts
        if (i % 20 == 0) {
             Logger::debug("Scanning... %s", candidate.toString().c_str());
        }
        
        if (tryBrokerCandidate(candidate)) {
            brokerHost = candidate.toString();
            persistConfig();
            Logger::info("Auto-discovered MQTT broker at %s", brokerHost.c_str());
            return true;
        }
    }

    Logger::warn("No MQTT broker found on network");
    return false;
}

bool Mqtt::tryBrokerCandidate(const IPAddress& candidate) {
    WiFiClient probe;
    constexpr uint32_t timeoutMs = 100; // Reduced from 250ms for faster full-subnet scan
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
    
    // Check WiFi status first (guard against null pointer)
    if (wifiManager) {
        WifiManager::Status status = wifiManager->getStatus();
        if (!status.connected) {
            Logger::error("Cannot probe: WiFi not connected");
            return;
        }
    }
    
    if (!probe.connect(brokerHost.c_str(), brokerPort)) {
        Logger::error("Unable to open TCP socket to %s:%u. Ensure docker-compose exposes Mosquitto on 0.0.0.0:%u and Windows firewall permits inbound connections.",
                      brokerHost.c_str(), brokerPort, brokerPort);
        if (wifiManager) {
            WifiManager::Status status = wifiManager->getStatus();
            String ip = status.ip.toString();
            Logger::info("Wi-Fi context: SSID=%s ip=%s", status.ssid.c_str(), ip.c_str());
        }
        probe.stop();
        return;
    }

    Logger::info("TCP port responded but MQTT handshake still failed. Confirm mosquitto.conf allows the configured credentials or enable anonymous access for testing.");
    probe.stop();
}

void Mqtt::handleMqttMessage(char* topic, uint8_t* payload, unsigned int length) {
    // DEBUG: Print ALL MQTT messages to serial
    Serial.printf("\n[MQTT_RX] Topic: %s\n", topic);
    Serial.printf("[MQTT_RX] Length: %d bytes\n", length);
    Serial.printf("[MQTT_RX] Payload: %.*s\n", length, (char*)payload);
    
    // Log incoming message with detailed info
    MqttLogger::logReceive(String(topic), payload, length);
    
    if (mqttInstance && mqttInstance->commandCallback) {
        String topicStr = String(topic);
        String payloadStr = String((char*)payload, length);
        mqttInstance->processMessage(topicStr, payloadStr);
    }
}

void Mqtt::processMessage(const String& topic, const String& payload) {
    uint32_t startMs = millis();
    
    if (commandCallback) {
        commandCallback(topic, payload);
        MqttLogger::logProcess(topic, "Command processed", true);
    } else {
        MqttLogger::logProcess(topic, "No callback", false, "callback not registered");
    }
    
    MqttLogger::logLatency("ProcessMessage", startMs);
}

void Mqtt::publishCoordinatorTelemetry(const CoordinatorSensorSnapshot& snapshot) {
    if (!mqttClient.connected()) return;
    StaticJsonDocument<256> doc;
    uint32_t ts = snapshot.timestampMs ? snapshot.timestampMs : millis();
    doc["ts"] = ts / 1000;
    doc["site_id"] = siteId;
    doc["coord_id"] = coordId.length() ? coordId : WiFi.macAddress();
    doc["light_lux"] = snapshot.lightLux;
    doc["temp_c"] = snapshot.tempC;
    doc["mmwave_presence"] = snapshot.mmWavePresence;
    doc["mmwave_confidence"] = snapshot.mmWaveConfidence;
    doc["mmwave_online"] = snapshot.mmWaveOnline;
    doc["wifi_rssi"] = snapshot.wifiConnected ? snapshot.wifiRssi : -127;
    doc["wifi_connected"] = snapshot.wifiConnected;
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(coordinatorTelemetryTopic().c_str(), payload.c_str());
}

void Mqtt::publishSerialLog(const String& message, const String& level, const String& tag) {
    if (!mqttClient.connected()) return;
    StaticJsonDocument<512> doc;
    doc["ts"] = millis() / 1000;
    doc["message"] = message;
    doc["level"] = level;
    if (tag.length() > 0) {
        doc["tag"] = tag;
    }
    String payload;
    serializeJson(doc, payload);
    mqttClient.publish(coordinatorSerialTopic().c_str(), payload.c_str());
}

String Mqtt::nodeTelemetryTopic(const String& nodeId) const {
    return "site/" + siteId + "/node/" + nodeId + "/telemetry";
}

String Mqtt::coordinatorTelemetryTopic() const {
    String id = coordId.length() ? coordId : WiFi.macAddress();
    return "site/" + siteId + "/coord/" + id + "/telemetry";
}

String Mqtt::coordinatorSerialTopic() const {
    String id = coordId.length() ? coordId : WiFi.macAddress();
    return "site/" + siteId + "/coord/" + id + "/serial";
}

String Mqtt::coordinatorCmdTopic() const {
    String id = coordId.length() ? coordId : WiFi.macAddress();
    return "site/" + siteId + "/coord/" + id + "/cmd";
}

String Mqtt::coordinatorMmwaveTopic() const {
    String id = coordId.length() ? coordId : WiFi.macAddress();
    return "site/" + siteId + "/coord/" + id + "/mmwave";
}
