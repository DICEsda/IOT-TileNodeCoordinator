#pragma once

#include <Arduino.h>
#include <map>
#include <vector>
#include <functional>
#include <Preferences.h>
#include "../Models.h"

class NodeRegistry {
public:
    NodeRegistry();
    ~NodeRegistry();

    bool begin();
    void loop();

    // Node registration
    bool registerNode(const String& nodeId, const String& lightId);
    bool unregisterNode(const String& nodeId);
    void clearAllNodes();
    
    // Pairing
    void startPairing(uint32_t durationMs = 30000);
    void stopPairing();
    bool isPairingActive() const;
    bool processPairingRequest(const uint8_t* mac, const String& nodeId);
    // Notification callback when a node is successfully registered
    void setNodeRegisteredCallback(std::function<void(const String& nodeId, const String& lightId)> callback);
    
    // Node status
    void updateNodeStatus(const String& nodeId, uint8_t duty);
    NodeInfo getNodeStatus(const String& nodeId) const;
    std::vector<NodeInfo> getAllNodes() const;
    
    // Node-Light mapping
    String getNodeForLight(const String& lightId) const;
    String getLightForNode(const String& nodeId) const;

private:
    std::map<String, NodeInfo> nodes;
    std::map<String, String> lightToNode;  // lightId -> nodeId
    Preferences prefs;
    bool prefsInitialized;
    
    bool pairingActive;
    uint32_t pairingEndTime;
    
    void loadFromStorage();
    void saveToStorage();
    void cleanupStaleNodes();
    std::function<void(const String& nodeId, const String& lightId)> nodeRegisteredCallback = nullptr;
    
    static const char* STORAGE_NAMESPACE;
    static const uint32_t NODE_TIMEOUT_MS = 300000; // 5 minutes
};
