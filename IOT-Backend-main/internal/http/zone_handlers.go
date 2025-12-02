package http

import (
	"encoding/json"
	"net/http"
	"time"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/types"
	"github.com/gorilla/mux"
)

// CreateZoneRequest represents a request to create a new zone
type CreateZoneRequest struct {
	Name          string `json:"name"`
	SiteId        string `json:"site_id"`
	CoordinatorId string `json:"coordinator_id"`
}

// UpdateZoneRequest represents a request to update a zone
type UpdateZoneRequest struct {
	Name          string `json:"name"`
	CoordinatorId string `json:"coordinator_id"`
}

// CreateZone creates a new zone and assigns it to a coordinator
func (h *Handler) CreateZone(w http.ResponseWriter, r *http.Request) {
	var req CreateZoneRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	// Validate required fields
	if req.Name == "" || req.SiteId == "" || req.CoordinatorId == "" {
		http.Error(w, "Name, site_id, and coordinator_id are required", http.StatusBadRequest)
		return
	}

	// Check if coordinator exists
	coordinator, err := h.Repo.GetCoordinatorBySiteAndId(req.SiteId, req.CoordinatorId)
	if err != nil || coordinator == nil {
		http.Error(w, "Coordinator not found", http.StatusNotFound)
		return
	}

	// Check if coordinator is already assigned to another zone
	existingZone, err := h.Repo.GetZoneByCoordinator(req.SiteId, req.CoordinatorId)
	if err != nil {
		http.Error(w, "Failed to check existing zone", http.StatusInternalServerError)
		return
	}
	if existingZone != nil {
		http.Error(w, "Coordinator is already assigned to a zone", http.StatusConflict)
		return
	}

	// Create zone
	zone := &types.Zone{
		Name:          req.Name,
		SiteId:        req.SiteId,
		CoordinatorId: req.CoordinatorId,
		CreatedAt:     time.Now(),
		UpdatedAt:     time.Now(),
	}

	ctx := r.Context()
	if err := h.Repo.CreateZone(ctx, zone); err != nil {
		http.Error(w, "Failed to create zone", http.StatusInternalServerError)
		return
	}

	// Auto-assign all nodes paired to this coordinator to the zone
	nodes, err := h.Repo.GetNodesByCoordinator(req.SiteId, req.CoordinatorId)
	if err == nil && nodes != nil {
		for _, node := range nodes {
			node.ZoneId = zone.Id
			_ = h.Repo.UpsertNode(ctx, node)
		}
	}

	// Send MQTT command to flash coordinator green
	topic := "site/" + req.SiteId + "/coord/" + req.CoordinatorId + "/cmd"
	payload := map[string]interface{}{
		"cmd":   "flash_green",
		"times": 3,
	}

	payloadBytes, _ := json.Marshal(payload)
	token := h.mqttClient.Publish(topic, 1, false, payloadBytes)
	token.Wait()

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":  "success",
		"message": "Zone created successfully",
		"zone":    zone,
	})
}

// GetZones retrieves all zones for a site
func (h *Handler) GetZones(w http.ResponseWriter, r *http.Request) {
	siteId := r.URL.Query().Get("site_id")
	if siteId == "" {
		http.Error(w, "site_id parameter is required", http.StatusBadRequest)
		return
	}

	zones, err := h.Repo.GetZonesBySite(siteId)
	if err != nil {
		http.Error(w, "Failed to retrieve zones", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"zones": zones,
	})
}

// GetZone retrieves a specific zone by ID
func (h *Handler) GetZone(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	zoneId := vars["id"]

	zone, err := h.Repo.GetZoneById(zoneId)
	if err != nil {
		http.Error(w, "Zone not found", http.StatusNotFound)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(zone)
}

// UpdateZone updates a zone
func (h *Handler) UpdateZone(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	zoneId := vars["id"]

	var req UpdateZoneRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	// Get existing zone
	zone, err := h.Repo.GetZoneById(zoneId)
	if err != nil {
		http.Error(w, "Zone not found", http.StatusNotFound)
		return
	}

	// Update fields
	if req.Name != "" {
		zone.Name = req.Name
	}
	if req.CoordinatorId != "" && req.CoordinatorId != zone.CoordinatorId {
		// Check if new coordinator exists
		coordinator, err := h.Repo.GetCoordinatorBySiteAndId(zone.SiteId, req.CoordinatorId)
		if err != nil || coordinator == nil {
			http.Error(w, "Coordinator not found", http.StatusNotFound)
			return
		}

		// Check if new coordinator is already assigned to another zone
		existingZone, _ := h.Repo.GetZoneByCoordinator(zone.SiteId, req.CoordinatorId)
		if existingZone != nil && existingZone.Id != zoneId {
			http.Error(w, "Coordinator is already assigned to another zone", http.StatusConflict)
			return
		}

		zone.CoordinatorId = req.CoordinatorId
	}

	ctx := r.Context()
	if err := h.Repo.UpdateZone(ctx, zone); err != nil {
		http.Error(w, "Failed to update zone", http.StatusInternalServerError)
		return
	}

	// Flash coordinator green
	topic := "site/" + zone.SiteId + "/coord/" + zone.CoordinatorId + "/cmd"
	payload := map[string]interface{}{
		"cmd":   "flash_green",
		"times": 3,
	}

	payloadBytes, _ := json.Marshal(payload)
	token := h.mqttClient.Publish(topic, 1, false, payloadBytes)
	token.Wait()

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":  "success",
		"message": "Zone updated successfully",
		"zone":    zone,
	})
}

// DeleteZone deletes a zone and flashes coordinator green
func (h *Handler) DeleteZone(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	zoneId := vars["id"]

	// Get zone before deleting to flash coordinator
	zone, err := h.Repo.GetZoneById(zoneId)
	if err != nil {
		http.Error(w, "Zone not found", http.StatusNotFound)
		return
	}

	ctx := r.Context()
	if err := h.Repo.DeleteZone(ctx, zoneId); err != nil {
		http.Error(w, "Failed to delete zone", http.StatusInternalServerError)
		return
	}

	// Flash coordinator green to confirm deletion
	topic := "site/" + zone.SiteId + "/coord/" + zone.CoordinatorId + "/cmd"
	payload := map[string]interface{}{
		"cmd":   "flash_green",
		"times": 3,
	}

	payloadBytes, _ := json.Marshal(payload)
	token := h.mqttClient.Publish(topic, 1, false, payloadBytes)
	token.Wait()

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":  "success",
		"message": "Zone deleted successfully",
	})
}
