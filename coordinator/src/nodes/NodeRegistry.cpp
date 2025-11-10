#include "NodeRegistry.h"
#include "../utils/Logger.h"

const char* NodeRegistry::STORAGE_NAMESPACE = "nodes";

NodeRegistry::NodeRegistry()
    : prefsInitialized(false)
    , pairingActive(false)
    , pairingEndTime(0) {
}

NodeRegistry::~NodeRegistry() {
    prefs.end();
}

bool NodeRegistry::begin() {
    prefsInitialized = prefs.begin(STORAGE_NAMESPACE, false);
    if (!prefsInitialized) {
        Logger::info("No saved node data found - starting with empty registry");
        Logger::info("(This is normal on first boot or after flash erase)");
        return true; // Continue anyway, just without persistence
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

void NodeRegistry::clearAllNodes() {
    size_t count = nodes.size();
    nodes.clear();
    lightToNode.clear();
    
    if (prefsInitialized) {
        prefs.clear();
        prefs.putUInt("count", 0);
    }
    
    Logger::info("Cleared all nodes (count: %d)", count);
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
    
    // Check if already registered - if so, just update lastSeen and return success
    if (nodes.find(nodeId) != nodes.end()) {
        Logger::info("Re-pairing known node %s", nodeId.c_str());
        nodes[nodeId].lastSeenMs = millis();
        return true; // Allow re-pairing
    }
    
    // Generate a stable light ID from MAC last 3 bytes
    char lightIdBuf[16];
    snprintf(lightIdBuf, sizeof(lightIdBuf), "L%02X%02X%02X", mac[3], mac[4], mac[5]);
    String lightId(lightIdBuf);
    
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
    if (!prefsInitialized) {
        return; // Skip saving if preferences not available
    }
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
    staleNodes.reserve(4); // Pre-allocate for typical case
    
    for (const auto& pair : nodes) {
        // Skip nodes that have never been seen (lastSeenMs == 0)
        if (pair.second.lastSeenMs > 0 && now - pair.second.lastSeenMs >= NODE_TIMEOUT_MS) {
            staleNodes.push_back(pair.first);
        }
    }
    
    for (const String& nodeId : staleNodes) {
        Logger::warning("Removing stale node %s", nodeId.c_str());
        unregisterNode(nodeId);
    }
}
