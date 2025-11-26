package http

import (
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/gorilla/mux"
)

// ============================================================================
// Customization Handlers
// ============================================================================

// GetCustomizationConfig retrieves configuration for coordinator or node
func (h *Handler) GetCustomizationConfig(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"]  // "coordinator" or "node"

	if deviceType != "coordinator" && deviceType != "node" {
		http.Error(w, "Invalid device type", http.StatusBadRequest)
		return
	}

	// Build default configuration based on known coordinator parameters
	config := buildDefaultConfig(deviceType)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(config)
}

// UpdateCoordinatorConfig updates coordinator-specific configuration
func (h *Handler) UpdateCoordinatorConfig(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"]
	deviceId := vars["id"]

	var req struct {
		SiteID string                 `json:"siteId"`
		Config map[string]interface{} `json:"config"`
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if req.SiteID == "" {
		req.SiteID = "site001"
	}

	// Publish MQTT command to device to update ConfigManager values
	topic := fmt.Sprintf("site/%s/%s/%s/cmd", req.SiteID, deviceType, deviceId)
	payload := map[string]interface{}{
		"cmd":    "update_config",
		"config": req.Config,
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
		"message": "Configuration updated",
	})
}

// UpdateLightConfig updates light sensor configuration
func (h *Handler) UpdateLightConfig(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"]
	deviceId := vars["id"]

	var req struct {
		SiteID string                 `json:"siteId"`
		Config map[string]interface{} `json:"config"`
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if req.SiteID == "" {
		req.SiteID = "site001"
	}

	// Publish MQTT command to device
	topic := fmt.Sprintf("site/%s/%s/%s/cmd", req.SiteID, deviceType, deviceId)
	payload := map[string]interface{}{
		"cmd":    "config_light",
		"config": req.Config,
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
		"message": "Light sensor configuration updated",
	})
}

// UpdateLEDConfig updates LED strip configuration
func (h *Handler) UpdateLEDConfig(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"]
	deviceId := vars["id"]

	var req struct {
		SiteID string                 `json:"siteId"`
		Config map[string]interface{} `json:"config"`
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if req.SiteID == "" {
		req.SiteID = "site001"
	}

	// Publish MQTT command to device
	topic := fmt.Sprintf("site/%s/%s/%s/cmd", req.SiteID, deviceType, deviceId)
	payload := map[string]interface{}{
		"cmd":    "config_led",
		"config": req.Config,
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
		"message": "LED configuration updated",
	})
}

// ResetToDefaults resets device configuration to factory defaults
func (h *Handler) ResetToDefaults(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"]
	deviceId := vars["id"]

	var req struct {
		SiteID  string `json:"siteId"`
		Section string `json:"section"`
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if req.SiteID == "" {
		req.SiteID = "site001"
	}

	// Publish MQTT command to device
	topic := fmt.Sprintf("site/%s/%s/%s/cmd", req.SiteID, deviceType, deviceId)
	payload := map[string]interface{}{
		"cmd":     "reset_config",
		"section": req.Section,
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
		"message": "Configuration reset to defaults",
	})
}

// LEDPreview sends a temporary LED preview command
func (h *Handler) LEDPreview(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"]
	deviceId := vars["id"]

	var req struct {
		SiteID     string `json:"siteId"`
		Color      string `json:"color"`
		Brightness int    `json:"brightness"`
		Effect     string `json:"effect"`
		Duration   int    `json:"duration"`
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if req.SiteID == "" {
		req.SiteID = "site001"
	}

	// Publish MQTT command to device
	topic := fmt.Sprintf("site/%s/%s/%s/cmd", req.SiteID, deviceType, deviceId)
	payload := map[string]interface{}{
		"cmd":        "led_preview",
		"color":      req.Color,
		"brightness": req.Brightness,
		"effect":     req.Effect,
		"duration":   req.Duration,
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
		"message": "LED preview started",
	})
}

// buildDefaultConfig creates default configuration based on ConfigManager defaults
func buildDefaultConfig(deviceType string) map[string]interface{} {
	config := make(map[string]interface{})

	// Coordinator-specific configuration (from ConfigKeys/Defaults in ConfigManager.h)
	if deviceType == "coordinator" {
		config["coordinator"] = map[string]interface{}{
			"presence_debounce_ms": 150,  // Defaults::PRESENCE_DEBOUNCE_MS
			"occupancy_hold_ms":    5000, // Defaults::OCCUPANCY_HOLD_MS
			"fade_in_ms":           150,  // Defaults::FADE_IN_MS
			"fade_out_ms":          1000, // Defaults::FADE_OUT_MS
			"pairing_window_s":     120,  // Defaults::PAIRING_WINDOW_S
		}

		// Radar status (read-only, managed by MmWave class)
		config["radar"] = map[string]interface{}{
			"enabled": true,
			"online":  false, // Will be updated from telemetry
		}

		// Light sensor status (read-only, managed by AmbientLightSensor class)
		config["light"] = map[string]interface{}{
			"enabled": true,
		}
	}

	// Node-specific configuration (from ConfigKeys/Defaults in ConfigManager.h)
	if deviceType == "node" {
		config["node"] = map[string]interface{}{
			"pwm_freq_hz":         1000,  // Defaults::PWM_FREQ_HZ
			"pwm_res_bits":        12,    // Defaults::PWM_RESOLUTION_BITS
			"telemetry_s":         5,     // Defaults::TELEMETRY_INTERVAL_S
			"rx_window_ms":        20,    // Defaults::RX_WINDOW_MS
			"rx_period_ms":        100,   // Defaults::RX_PERIOD_MS
			"derate_start_c":      70.0,  // Defaults::DERATE_START_C
			"derate_min_duty_pct": 30,    // Defaults::DERATE_MIN_DUTY_PCT
			"retry_count":         3,     // Defaults::RETRY_COUNT
			"cmd_ttl_ms":          1500,  // Defaults::CMD_TTL_MS
		}

		// LED configuration (4-pixel SK6812B status indicator)
		config["led"] = map[string]interface{}{
			"enabled":    true,
			"brightness": 80,   // 0-100%
			"color":      "#00FFBF", // Default color
		}
	}

	return config
}
