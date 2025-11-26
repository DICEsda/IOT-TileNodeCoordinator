package http

import (
	"encoding/json"
	"net/http"
	"os"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/repository"
)

// ============================================================================
// Settings Handlers
// ============================================================================

// GetSettings retrieves settings for a site
func (h *Handler) GetSettings(w http.ResponseWriter, r *http.Request) {
	siteID := r.URL.Query().Get("site_id")
	if siteID == "" {
		siteID = "site001" // Default
	}

	settings, err := h.Repo.GetSettings(siteID)
	if err != nil {
		// Return default settings if not found
		settings = &repository.Settings{
			SiteID:            siteID,
			AutoMode:          true,
			MotionSensitivity: 50,
			LightIntensity:    80,
			AutoOffDelay:      30,
			Zones:             []string{"Living Room", "Bedroom", "Kitchen", "Bathroom", "Office", "Hallway"},
			GoogleHomeEnabled: false,
		}
	}

	// Populate current MQTT credentials from environment
	if broker := os.Getenv("MQTT_BROKER"); broker != "" {
		settings.MqttBroker = broker
	} else {
		settings.MqttBroker = "tcp://mosquitto:1883"
	}
	
	if username := os.Getenv("MQTT_USERNAME"); username != "" {
		settings.MqttUsername = username
	}
	
	if password := os.Getenv("MQTT_PASSWORD"); password != "" {
		settings.MqttPassword = password
	}
	
	// Populate WiFi credentials from environment (if set)
	if wifiSSID := os.Getenv("ESP32_WIFI_SSID"); wifiSSID != "" {
		settings.WifiSSID = wifiSSID
	}
	
	if wifiPassword := os.Getenv("ESP32_WIFI_PASSWORD"); wifiPassword != "" {
		settings.WifiPassword = wifiPassword
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(settings)
}

// SaveSettings saves or updates settings for a site
func (h *Handler) SaveSettings(w http.ResponseWriter, r *http.Request) {
	var settings repository.Settings
	if err := json.NewDecoder(r.Body).Decode(&settings); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	if settings.SiteID == "" {
		settings.SiteID = "site001" // Default
	}

	if err := h.Repo.SaveSettings(&settings); err != nil {
		http.Error(w, "Failed to save settings", http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "success",
		"message": "Settings saved",
	})
}
