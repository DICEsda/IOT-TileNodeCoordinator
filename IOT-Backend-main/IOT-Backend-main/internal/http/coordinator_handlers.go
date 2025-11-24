package http

import (
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/gorilla/mux"
)

// ============================================================================
// Coordinator Control Handlers
// ============================================================================

// StartPairingRequest represents the request to start pairing mode
type StartPairingRequest struct {
	SiteID        string `json:"site_id"`
	CoordinatorID string `json:"coordinator_id"`
	DurationMs    int    `json:"duration_ms"`
}

// RestartCoordinatorRequest represents the request to restart a coordinator
type RestartCoordinatorRequest struct {
	SiteID        string `json:"site_id"`
	CoordinatorID string `json:"coordinator_id"`
}

// UpdateWiFiRequest represents the request to update WiFi configuration
type UpdateWiFiRequest struct {
	SiteID        string `json:"site_id"`
	CoordinatorID string `json:"coordinator_id"`
	SSID          string `json:"ssid"`
	Password      string `json:"password"`
}

// StartPairing initiates pairing mode on the coordinator
func (h *Handler) StartPairing(w http.ResponseWriter, r *http.Request) {
	var req StartPairingRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	// Default to 60 seconds if not specified
	if req.DurationMs == 0 {
		req.DurationMs = 60000
	}

	// Publish MQTT command to coordinator
	topic := fmt.Sprintf("site/%s/coord/%s/cmd", req.SiteID, req.CoordinatorID)
	payload := map[string]interface{}{
		"cmd":         "pair",
		"duration_ms": req.DurationMs,
	}

	payloadBytes, err := json.Marshal(payload)
	if err != nil {
		http.Error(w, "Failed to create command", http.StatusInternalServerError)
		return
	}

	token := h.mqttClient.Publish(topic, 1, false, payloadBytes)
	if token.Wait() && token.Error() != nil {
		http.Error(w, "Failed to publish command", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "success",
		"message": "Pairing mode activated",
	})
}

// RestartCoordinator sends restart command to the coordinator
func (h *Handler) RestartCoordinator(w http.ResponseWriter, r *http.Request) {
	var req RestartCoordinatorRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	// Publish MQTT command to coordinator
	topic := fmt.Sprintf("site/%s/coord/%s/cmd", req.SiteID, req.CoordinatorID)
	payload := map[string]interface{}{
		"cmd": "restart",
	}

	payloadBytes, err := json.Marshal(payload)
	if err != nil {
		http.Error(w, "Failed to create command", http.StatusInternalServerError)
		return
	}

	token := h.mqttClient.Publish(topic, 1, false, payloadBytes)
	if token.Wait() && token.Error() != nil {
		http.Error(w, "Failed to publish command", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "success",
		"message": "Restart command sent",
	})
}

// UpdateWiFiConfig sends WiFi configuration update to coordinator
func (h *Handler) UpdateWiFiConfig(w http.ResponseWriter, r *http.Request) {
	var req UpdateWiFiRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if req.SSID == "" || req.Password == "" {
		http.Error(w, "SSID and password are required", http.StatusBadRequest)
		return
	}

	// Publish MQTT command to coordinator
	topic := fmt.Sprintf("site/%s/coord/%s/cmd", req.SiteID, req.CoordinatorID)
	payload := map[string]interface{}{
		"cmd":      "update_wifi",
		"ssid":     req.SSID,
		"password": req.Password,
	}

	payloadBytes, err := json.Marshal(payload)
	if err != nil {
		http.Error(w, "Failed to create command", http.StatusInternalServerError)
		return
	}

	token := h.mqttClient.Publish(topic, 1, false, payloadBytes)
	if token.Wait() && token.Error() != nil {
		http.Error(w, "Failed to publish command", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "success",
		"message": "WiFi configuration updated",
	})
}

// GetCoordinator retrieves coordinator details
func (h *Handler) GetCoordinator(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	siteID := vars["siteId"]
	coordID := vars["coordId"]

	// Get coordinator from database
	coordinator, err := h.Repo.GetCoordinatorBySiteAndId(siteID, coordID)
	if err != nil {
		http.Error(w, "Coordinator not found", http.StatusNotFound)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(coordinator)
}

// ============================================================================
// Node Management Handlers
// ============================================================================

// NodeColorRequest represents a color command request
type NodeColorRequest struct {
	SiteID        string `json:"site_id"`
	CoordinatorID string `json:"coordinator_id"`
	NodeID        string `json:"node_id"`
	R             uint8  `json:"r"`
	G             uint8  `json:"g"`
	B             uint8  `json:"b"`
	W             uint8  `json:"w"`
}

// NodeBrightnessRequest represents a brightness command request
type NodeBrightnessRequest struct {
	SiteID        string `json:"site_id"`
	CoordinatorID string `json:"coordinator_id"`
	NodeID        string `json:"node_id"`
	Brightness    uint8  `json:"brightness"`
}

// NodeOffRequest represents a turn off command request
type NodeOffRequest struct {
	SiteID        string `json:"site_id"`
	CoordinatorID string `json:"coordinator_id"`
	NodeID        string `json:"node_id"`
}

// NodeZoneRequest represents a zone update request
type NodeZoneRequest struct {
	SiteID        string `json:"site_id"`
	CoordinatorID string `json:"coordinator_id"`
	NodeID        string `json:"node_id"`
	ZoneID        string `json:"zone_id"`
}

// NodeNameRequest represents a name update request
type NodeNameRequest struct {
	SiteID        string `json:"site_id"`
	CoordinatorID string `json:"coordinator_id"`
	NodeID        string `json:"node_id"`
	Name          string `json:"name"`
}

// GetNodes retrieves all nodes for a coordinator
func (h *Handler) GetNodes(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	siteID := vars["siteId"]
	coordID := vars["coordId"]

	nodes, err := h.Repo.GetNodesByCoordinator(siteID, coordID)
	if err != nil {
		http.Error(w, "Failed to retrieve nodes", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(nodes)
}

// DeleteNode removes a node from the system
func (h *Handler) DeleteNode(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	siteID := vars["siteId"]
	coordID := vars["coordId"]
	nodeID := vars["nodeId"]

	// Publish unpair command via MQTT
	topic := fmt.Sprintf("site/%s/coord/%s/cmd", siteID, coordID)
	payload := map[string]interface{}{
		"cmd":     "unpair_node",
		"node_id": nodeID,
	}

	payloadBytes, err := json.Marshal(payload)
	if err != nil {
		http.Error(w, "Failed to create command", http.StatusInternalServerError)
		return
	}

	token := h.mqttClient.Publish(topic, 1, false, payloadBytes)
	if token.Wait() && token.Error() != nil {
		http.Error(w, "Failed to publish command", http.StatusInternalServerError)
		return
	}

	// Delete from database
	if err := h.Repo.DeleteNode(siteID, coordID, nodeID); err != nil {
		http.Error(w, "Failed to delete node from database", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "success",
		"message": "Node deleted",
	})
}

// SendNodeColor sends a test color command to a node
func (h *Handler) SendNodeColor(w http.ResponseWriter, r *http.Request) {
	var req NodeColorRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	// Publish MQTT command to node via coordinator
	topic := fmt.Sprintf("site/%s/node/%s/cmd", req.SiteID, req.NodeID)
	payload := map[string]interface{}{
		"cmd": "set_color",
		"r":   req.R,
		"g":   req.G,
		"b":   req.B,
		"w":   req.W,
	}

	payloadBytes, err := json.Marshal(payload)
	if err != nil {
		http.Error(w, "Failed to create command", http.StatusInternalServerError)
		return
	}

	token := h.mqttClient.Publish(topic, 1, false, payloadBytes)
	if token.Wait() && token.Error() != nil {
		http.Error(w, "Failed to publish command", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "success",
		"message": "Color command sent",
	})
}

// TurnOffNode sends a turn off command to a node
func (h *Handler) TurnOffNode(w http.ResponseWriter, r *http.Request) {
	var req NodeOffRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	// Publish MQTT command to node
	topic := fmt.Sprintf("site/%s/node/%s/cmd", req.SiteID, req.NodeID)
	payload := map[string]interface{}{
		"cmd": "off",
	}

	payloadBytes, err := json.Marshal(payload)
	if err != nil {
		http.Error(w, "Failed to create command", http.StatusInternalServerError)
		return
	}

	token := h.mqttClient.Publish(topic, 1, false, payloadBytes)
	if token.Wait() && token.Error() != nil {
		http.Error(w, "Failed to publish command", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "success",
		"message": "Turn off command sent",
	})
}

// SetNodeBrightness sends a brightness command to a node
func (h *Handler) SetNodeBrightness(w http.ResponseWriter, r *http.Request) {
	var req NodeBrightnessRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	// Publish MQTT command to node
	topic := fmt.Sprintf("site/%s/node/%s/cmd", req.SiteID, req.NodeID)
	payload := map[string]interface{}{
		"cmd":   "set_brightness",
		"value": req.Brightness,
	}

	payloadBytes, err := json.Marshal(payload)
	if err != nil {
		http.Error(w, "Failed to create command", http.StatusInternalServerError)
		return
	}

	token := h.mqttClient.Publish(topic, 1, false, payloadBytes)
	if token.Wait() && token.Error() != nil {
		http.Error(w, "Failed to publish command", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "success",
		"message": "Brightness command sent",
	})
}

// UpdateNodeZone updates the zone assignment for a node
func (h *Handler) UpdateNodeZone(w http.ResponseWriter, r *http.Request) {
	var req NodeZoneRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	// Update in database
	if err := h.Repo.UpdateNodeZone(req.SiteID, req.CoordinatorID, req.NodeID, req.ZoneID); err != nil {
		http.Error(w, "Failed to update zone", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "success",
		"message": "Zone updated",
	})
}

// UpdateNodeName updates the name for a node
func (h *Handler) UpdateNodeName(w http.ResponseWriter, r *http.Request) {
	var req NodeNameRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	// Update in database
	if err := h.Repo.UpdateNodeName(req.SiteID, req.CoordinatorID, req.NodeID, req.Name); err != nil {
		http.Error(w, "Failed to update name", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "success",
		"message": "Name updated",
	})
}
