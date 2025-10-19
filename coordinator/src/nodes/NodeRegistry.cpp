#include "NodeRegistry.h"
#include "../utils/Logger.h"

const char* NodeRegistry::STORAGE_NAMESPACE = "nodes";

NodeRegistry::NodeRegistry()
    : pairingActive(false)
    , pairingEndTime(0) {
}

NodeRegistry::~NodeRegistry() {
    prefs.end();
}

bool NodeRegistry::begin() {
    if (!prefs.begin(STORAGE_NAMESPACE, false)) {
        Logger::error("Failed to initialize node storage");
        return false;
    }
    
    loadFromStorage();
    Logger::info("Node registry initialized with %d nodes", nodes.size());
    return true;
}

void NodeRegistry::loop() {
    uint32_t now = millis();
    
    // Check pairing timeout
    if (pairingActive && now >= pairingEndTime) {
        pairingActive = false;
        Logger::info("Pairing window closed");
    }
    
    // Periodically clean up stale nodes
    static uint32_t lastCleanup = 0;
    if (now - lastCleanup >= 60000) { // Every minute
        cleanupStaleNodes();
        lastCleanup = now;
    }
}

bool NodeRegistry::registerNode(const String& nodeId, const String& lightId) {
    if (nodes.find(nodeId) != nodes.end()) {
        Logger::warning("Node %s already registered", nodeId.c_str());
        return false;
    }
    
    NodeInfo info;
    info.nodeId = nodeId;
    info.lightId = lightId;
    info.lastDuty = 0;
    info.lastSeenMs = millis();
    info.temperature = 0;
    info.isDerated = false;
    info.derationLevel = 100;
    
    nodes[nodeId] = info;
    lightToNode[lightId] = nodeId;
    
    saveToStorage();
    Logger::info("Registered node %s with light %s", nodeId.c_str(), lightId.c_str());
    // Notify any listener about registration
    if (nodeRegisteredCallback) {
        nodeRegisteredCallback(nodeId, lightId);
    }
    return true;
}

bool NodeRegistry::unregisterNode(const String& nodeId) {
    auto it = nodes.find(nodeId);
    if (it == nodes.end()) {
        return false;
    }
    
    String lightId = it->second.lightId;
    nodes.erase(it);
    lightToNode.erase(lightId);
    
    saveToStorage();
    Logger::info("Unregistered node %s", nodeId.c_str());
    return true;
}

void NodeRegistry::startPairing(uint32_t durationMs) {
    pairingActive = true;
    pairingEndTime = millis() + durationMs;
    Logger::info("Started pairing window for %d ms", durationMs);
}

void NodeRegistry::stopPairing() {
    pairingActive = false;
    Logger::info("Pairing window closed manually");
}

void NodeRegistry::setNodeRegisteredCallback(std::function<void(const String& nodeId, const String& lightId)> callback) {
    nodeRegisteredCallback = callback;
}

bool NodeRegistry::isPairingActive() const {
    return pairingActive && millis() < pairingEndTime;
}

bool NodeRegistry::processPairingRequest(const uint8_t* mac, const String& nodeId) {
    if (!isPairingActive()) {
        Logger::warning("Rejected pairing request from %s: pairing not active", nodeId.c_str());
        return false;
    }
    
    // Generate a light ID based on the node ID
    String lightId = "L" + nodeId.substring(1); // Convert "N123" to "L123"
    
    if (registerNode(nodeId, lightId)) {
        pairingActive = false; // Close window after successful pairing
        return true;
    }
    return false;
}

void NodeRegistry::updateNodeStatus(const String& nodeId, uint8_t duty) {
    auto it = nodes.find(nodeId);
    if (it != nodes.end()) {
        it->second.lastDuty = duty;
        it->second.lastSeenMs = millis();
    }
}

NodeInfo NodeRegistry::getNodeStatus(const String& nodeId) const {
    auto it = nodes.find(nodeId);
    return it != nodes.end() ? it->second : NodeInfo();
}

std::vector<NodeInfo> NodeRegistry::getAllNodes() const {
    std::vector<NodeInfo> result;
    result.reserve(nodes.size());
    for (const auto& pair : nodes) {
        result.push_back(pair.second);
    }
    return result;
}

String NodeRegistry::getNodeForLight(const String& lightId) const {
    auto it = lightToNode.find(lightId);
    return it != lightToNode.end() ? it->second : String();
}

String NodeRegistry::getLightForNode(const String& nodeId) const {
    auto it = nodes.find(nodeId);
    return it != nodes.end() ? it->second.lightId : String();
}

void NodeRegistry::loadFromStorage() {
    nodes.clear();
    lightToNode.clear();
    
    size_t nodeCount = prefs.getUInt("count", 0);
    for (size_t i = 0; i < nodeCount; i++) {
        String key = "node" + String(i);
        String data = prefs.getString(key.c_str());
        
        // Parse node data (format: "nodeId,lightId,lastDuty")
        int comma1 = data.indexOf(',');
        int comma2 = data.indexOf(',', comma1 + 1);
        if (comma1 > 0 && comma2 > comma1) {
            String nodeId = data.substring(0, comma1);
            String lightId = data.substring(comma1 + 1, comma2);
            uint8_t lastDuty = data.substring(comma2 + 1).toInt();
            
            NodeInfo info;
            info.nodeId = nodeId;
            info.lightId = lightId;
            info.lastDuty = lastDuty;
            info.lastSeenMs = 0; // Mark as not seen in this session
            
            nodes[nodeId] = info;
            lightToNode[lightId] = nodeId;
        }
    }
}

void NodeRegistry::saveToStorage() {
    prefs.clear();
    prefs.putUInt("count", nodes.size());
    
    size_t i = 0;
    for (const auto& pair : nodes) {
        const NodeInfo& info = pair.second;
        String data = info.nodeId + "," + info.lightId + "," + String(info.lastDuty);
        prefs.putString(("node" + String(i)).c_str(), data);
        i++;
    }
}

void NodeRegistry::cleanupStaleNodes() {
    uint32_t now = millis();
    std::vector<String> staleNodes;
    
    for (const auto& pair : nodes) {
        if (now - pair.second.lastSeenMs >= NODE_TIMEOUT_MS) {
            staleNodes.push_back(pair.first);
        }
    }
    
    for (const String& nodeId : staleNodes) {
        Logger::warning("Removing stale node %s", nodeId.c_str());
        unregisterNode(nodeId);
    }
}
