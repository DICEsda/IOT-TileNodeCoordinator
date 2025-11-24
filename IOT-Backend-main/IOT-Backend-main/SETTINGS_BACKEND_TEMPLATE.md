# Settings Backend Implementation Template

This template provides starter code for implementing the Settings tab backend endpoints in Go.

## Directory Structure

```
cmd/iot/
internal/
├── models/
│   └── settings.go          # Settings model
├── handlers/
│   └── settings.go          # Settings handlers
├── services/
│   └── settings_service.go  # Business logic
└── middleware/
    └── auth.go              # Password validation
```

## 1. Settings Model (internal/models/settings.go)

```go
package models

import (
    "time"
    "go.mongodb.org/mongo-driver/bson/primitive"
)

type SystemSettings struct {
    ID              primitive.ObjectID    `bson:"_id,omitempty" json:"id,omitempty"`
    UpdatedAt       time.Time            `bson:"updated_at" json:"updated_at"`
    SystemIdentity  SystemIdentitySettings  `bson:"system_identity" json:"system_identity"`
    Network         NetworkSettings         `bson:"network" json:"network"`
    MQTT            MQTTBehaviorSettings    `bson:"mqtt" json:"mqtt"`
    Coordinator     CoordinatorSettings     `bson:"coordinator" json:"coordinator"`
    Node            NodeDefaultSettings     `bson:"node" json:"node"`
    Zones           ZoneSettings            `bson:"zones" json:"zones"`
    Automation      AutomationSettings      `bson:"automation" json:"automation"`
    Telemetry       TelemetrySettings       `bson:"telemetry" json:"telemetry"`
    Developer       DeveloperSettings       `bson:"developer" json:"developer"`
}

type SystemIdentitySettings struct {
    DefaultSiteID            string `bson:"default_site_id" json:"default_site_id"`
    DefaultCoordID           string `bson:"default_coord_id" json:"default_coord_id"`
    MaxCoordinatorsPerSite   int    `bson:"max_coordinators_per_site" json:"max_coordinators_per_site"`
    MaxNodesPerCoordinator   int    `bson:"max_nodes_per_coordinator" json:"max_nodes_per_coordinator"`
}

type NetworkSettings struct {
    ESP32WifiSSID        string `bson:"esp32_wifi_ssid" json:"esp32_wifi_ssid"`
    ESP32WifiPassword    string `bson:"esp32_wifi_password" json:"esp32_wifi_password"`
    ESP32WifiTimeout     int    `bson:"esp32_wifi_timeout" json:"esp32_wifi_timeout"`
    ESP32ReconnectDelay  int    `bson:"esp32_reconnect_delay" json:"esp32_reconnect_delay"`
    ESP32PingInterval    int    `bson:"esp32_ping_interval" json:"esp32_ping_interval"`
    APIURL               string `bson:"api_url" json:"api_url"`
    WSURL                string `bson:"ws_url" json:"ws_url"`
}

type MQTTBehaviorSettings struct {
    MQTTQoS        int    `bson:"mqtt_qos" json:"mqtt_qos"`
    MQTTKeepalive  int    `bson:"mqtt_keepalive" json:"mqtt_keepalive"`
    MQTTClientID   string `bson:"mqtt_client_id" json:"mqtt_client_id"`
}

type CoordinatorSettings struct {
    ESP32FirmwareVersion string `bson:"esp32_firmware_version" json:"esp32_firmware_version"`
    ESP32OTAURL          string `bson:"esp32_ota_url" json:"esp32_ota_url"`
    CoordLEDCount        int    `bson:"coord_led_count" json:"coord_led_count"`
    CoordButtonHoldTime  int    `bson:"coord_button_hold_time" json:"coord_button_hold_time"`
    CoordMaxNodes        int    `bson:"coord_max_nodes" json:"coord_max_nodes"`
}

type NodeDefaultSettings struct {
    NodeLEDCount            int  `bson:"node_led_count" json:"node_led_count"`
    NodeDefaultBrightness   int  `bson:"node_default_brightness" json:"node_default_brightness"`
    NodeTempSensorEnabled   bool `bson:"node_temp_sensor_enabled" json:"node_temp_sensor_enabled"`
    NodeButtonEnabled       bool `bson:"node_button_enabled" json:"node_button_enabled"`
}

type ZoneSettings struct {
    DefaultZones      []string `bson:"default_zones" json:"default_zones"`
    AllowCustomZones  bool     `bson:"allow_custom_zones" json:"allow_custom_zones"`
    MaxZones          int      `bson:"max_zones" json:"max_zones"`
}

type AutomationSettings struct {
    EnergySavingMode        bool `bson:"energy_saving_mode" json:"energy_saving_mode"`
    AutoOffDelay            int  `bson:"auto_off_delay" json:"auto_off_delay"`
    MotionSensitivity       int  `bson:"motion_sensitivity" json:"motion_sensitivity"`
    DefaultLightIntensity   int  `bson:"default_light_intensity" json:"default_light_intensity"`
}

type TelemetrySettings struct {
    TelemetryRetentionDays int  `bson:"telemetry_retention_days" json:"telemetry_retention_days"`
    DebugMode              bool `bson:"debug_mode" json:"debug_mode"`
    MetricsInterval        int  `bson:"metrics_interval" json:"metrics_interval"`
    LogToFile              bool `bson:"log_to_file" json:"log_to_file"`
}

type DeveloperSettings struct {
    MockDevicesEnabled bool `bson:"mock_devices_enabled" json:"mock_devices_enabled"`
    SimulatorEnabled   bool `bson:"simulator_enabled" json:"simulator_enabled"`
    SimulatorInterval  int  `bson:"simulator_interval" json:"simulator_interval"`
}

// GetDefaultSettings returns system default settings
func GetDefaultSettings() SystemSettings {
    return SystemSettings{
        UpdatedAt: time.Now(),
        SystemIdentity: SystemIdentitySettings{
            DefaultSiteID:          "site001",
            DefaultCoordID:         "coord001",
            MaxCoordinatorsPerSite: 10,
            MaxNodesPerCoordinator: 250,
        },
        Network: NetworkSettings{
            ESP32WifiSSID:       "",
            ESP32WifiPassword:   "",
            ESP32WifiTimeout:    10000,
            ESP32ReconnectDelay: 5000,
            ESP32PingInterval:   30000,
            APIURL:              "http://localhost:3000",
            WSURL:               "ws://localhost:3000/ws",
        },
        MQTT: MQTTBehaviorSettings{
            MQTTQoS:       1,
            MQTTKeepalive: 60,
            MQTTClientID:  "smartlight-backend",
        },
        Coordinator: CoordinatorSettings{
            ESP32FirmwareVersion: "1.0.0",
            ESP32OTAURL:          "",
            CoordLEDCount:        12,
            CoordButtonHoldTime:  3000,
            CoordMaxNodes:        250,
        },
        Node: NodeDefaultSettings{
            NodeLEDCount:          60,
            NodeDefaultBrightness: 128,
            NodeTempSensorEnabled: true,
            NodeButtonEnabled:     true,
        },
        Zones: ZoneSettings{
            DefaultZones:     []string{"Living Room", "Bedroom", "Kitchen", "Bathroom", "Office", "Hallway"},
            AllowCustomZones: true,
            MaxZones:         50,
        },
        Automation: AutomationSettings{
            EnergySavingMode:      false,
            AutoOffDelay:          30,
            MotionSensitivity:     50,
            DefaultLightIntensity: 80,
        },
        Telemetry: TelemetrySettings{
            TelemetryRetentionDays: 30,
            DebugMode:              false,
            MetricsInterval:        60,
            LogToFile:              true,
        },
        Developer: DeveloperSettings{
            MockDevicesEnabled: false,
            SimulatorEnabled:   false,
            SimulatorInterval:  1000,
        },
    }
}
```

## 2. Settings Service (internal/services/settings_service.go)

```go
package services

import (
    "context"
    "encoding/json"
    "os"
    "time"

    "go.mongodb.org/mongo-driver/bson"
    "go.mongodb.org/mongo-driver/mongo"
    "go.mongodb.org/mongo-driver/mongo/options"

    "your-module/internal/models"
)

type SettingsService struct {
    collection *mongo.Collection
    mqttClient interface{} // Your MQTT client type
}

func NewSettingsService(db *mongo.Database, mqttClient interface{}) *SettingsService {
    return &SettingsService{
        collection: db.Collection("system_settings"),
        mqttClient: mqttClient,
    }
}

// GetSettings retrieves current system settings
func (s *SettingsService) GetSettings(ctx context.Context) (*models.SystemSettings, error) {
    var settings models.SystemSettings
    
    err := s.collection.FindOne(ctx, bson.M{}).Decode(&settings)
    if err == mongo.ErrNoDocuments {
        // Initialize with defaults on first run
        settings = models.GetDefaultSettings()
        _, err = s.collection.InsertOne(ctx, settings)
        if err != nil {
            return nil, err
        }
        return &settings, nil
    }
    
    return &settings, err
}

// UpdateSettings updates a specific section of settings
func (s *SettingsService) UpdateSettings(ctx context.Context, section string, data map[string]interface{}) error {
    updateField := "." + section
    update := bson.M{
        "$set": bson.M{
            updateField:  data,
            "updated_at": time.Now(),
        },
    }
    
    _, err := s.collection.UpdateOne(ctx, bson.M{}, update, options.Update().SetUpsert(true))
    if err != nil {
        return err
    }
    
    // Publish MQTT update for real-time sync
    s.publishSettingsUpdate(section, data)
    
    // Apply device-specific configurations
    s.applyDeviceConfiguration(section, data)
    
    return nil
}

// ResetSettings resets all settings to defaults
func (s *SettingsService) ResetSettings(ctx context.Context) error {
    defaults := models.GetDefaultSettings()
    
    _, err := s.collection.ReplaceOne(
        ctx,
        bson.M{},
        defaults,
        options.Replace().SetUpsert(true),
    )
    
    return err
}

// ValidatePassword checks if the provided password is correct
func (s *SettingsService) ValidatePassword(password string) bool {
    expectedPassword := os.Getenv("SETTINGS_PASSWORD")
    if expectedPassword == "" {
        expectedPassword = "1234" // Default fallback
    }
    return password == expectedPassword
}

// publishSettingsUpdate publishes settings change to MQTT
func (s *SettingsService) publishSettingsUpdate(section string, data map[string]interface{}) {
    message := map[string]interface{}{
        "type":      "settings_update",
        "section":   section,
        "settings":  data,
        "timestamp": time.Now().Format(time.RFC3339),
    }
    
    payload, _ := json.Marshal(message)
    
    // Publish to MQTT topic
    topic := "system/settings/update"
    // s.mqttClient.Publish(topic, 1, false, payload)
    // TODO: Uncomment and implement with your MQTT client
}

// applyDeviceConfiguration publishes device-specific configs
func (s *SettingsService) applyDeviceConfiguration(section string, data map[string]interface{}) {
    // TODO: Implement device configuration updates
    // Based on section, publish to appropriate device topics
    
    switch section {
    case "network":
        // Publish WiFi config to coordinators
        // topic: site/{siteId}/coord/{coordId}/config
    case "coordinator":
        // Publish coordinator config
    case "node":
        // Publish node defaults
    }
}
```

## 3. Settings Handlers (internal/handlers/settings.go)

```go
package handlers

import (
    "encoding/json"
    "net/http"

    "your-module/internal/services"
)

type SettingsHandler struct {
    service *services.SettingsService
}

func NewSettingsHandler(service *services.SettingsService) *SettingsHandler {
    return &SettingsHandler{service: service}
}

// GetSettings handles GET /api/v1/system/settings
func (h *SettingsHandler) GetSettings(w http.ResponseWriter, r *http.Request) {
    settings, err := h.service.GetSettings(r.Context())
    if err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }
    
    response := map[string]interface{}{
        "success": true,
        "data":    settings,
    }
    
    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// UpdateSettings handles PUT /api/v1/system/settings
func (h *SettingsHandler) UpdateSettings(w http.ResponseWriter, r *http.Request) {
    var req struct {
        Section  string                 `json:"section"`
        Settings map[string]interface{} `json:"settings"`
    }
    
    if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
        http.Error(w, "Invalid request body", http.StatusBadRequest)
        return
    }
    
    err := h.service.UpdateSettings(r.Context(), req.Section, req.Settings)
    if err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }
    
    response := map[string]interface{}{
        "success": true,
        "message": "Settings updated successfully",
    }
    
    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// ValidatePassword handles POST /api/v1/system/settings/validate-password
func (h *SettingsHandler) ValidatePassword(w http.ResponseWriter, r *http.Request) {
    var req struct {
        Password string `json:"password"`
    }
    
    if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
        http.Error(w, "Invalid request body", http.StatusBadRequest)
        return
    }
    
    valid := h.service.ValidatePassword(req.Password)
    
    response := map[string]bool{
        "valid": valid,
    }
    
    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// ResetSettings handles POST /api/v1/system/settings/reset
func (h *SettingsHandler) ResetSettings(w http.ResponseWriter, r *http.Request) {
    err := h.service.ResetSettings(r.Context())
    if err != nil {
        http.Error(w, err.Error(), http.StatusInternalServerError)
        return
    }
    
    response := map[string]interface{}{
        "success": true,
        "message": "Settings reset to defaults successfully",
    }
    
    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}
```

## 4. Router Setup (main.go or routes.go)

```go
// In your main.go or routes setup

import (
    "your-module/internal/handlers"
    "your-module/internal/services"
)

func setupRoutes(router *mux.Router, db *mongo.Database, mqttClient interface{}) {
    // Initialize service and handler
    settingsService := services.NewSettingsService(db, mqttClient)
    settingsHandler := handlers.NewSettingsHandler(settingsService)
    
    // Register routes
    apiV1 := router.PathPrefix("/api/v1").Subrouter()
    
    apiV1.HandleFunc("/system/settings", settingsHandler.GetSettings).Methods("GET")
    apiV1.HandleFunc("/system/settings", settingsHandler.UpdateSettings).Methods("PUT")
    apiV1.HandleFunc("/system/settings/validate-password", settingsHandler.ValidatePassword).Methods("POST")
    apiV1.HandleFunc("/system/settings/reset", settingsHandler.ResetSettings).Methods("POST")
}
```

## 5. Environment Variables

Add to your `.env` file:

```bash
# Settings Password
SETTINGS_PASSWORD=1234

# MongoDB
MONGODB_URI=mongodb://localhost:27017
MONGODB_DATABASE=smartlight

# API URLs (for read-only fields)
API_URL=http://localhost:3000
WS_URL=ws://localhost:3000/ws
```

## 6. Testing

### Test Password Validation
```bash
curl -X POST http://localhost:3000/api/v1/system/settings/validate-password \
  -H "Content-Type: application/json" \
  -d '{"password":"1234"}'
```

### Test Get Settings
```bash
curl http://localhost:3000/api/v1/system/settings
```

### Test Update Settings
```bash
curl -X PUT http://localhost:3000/api/v1/system/settings \
  -H "Content-Type: application/json" \
  -d '{
    "section": "automation",
    "settings": {
      "energy_saving_mode": true,
      "auto_off_delay": 60
    }
  }'
```

### Test Reset Settings
```bash
curl -X POST http://localhost:3000/api/v1/system/settings/reset
```

## Next Steps

1. Copy these templates to your backend project
2. Adjust package imports to match your project structure
3. Integrate with your existing MongoDB connection
4. Wire up MQTT client for real-time updates
5. Add CORS headers if needed
6. Test each endpoint
7. Implement device configuration publishing

## Additional Considerations

### Password Security
Consider hashing the password:
```go
import "golang.org/x/crypto/bcrypt"

func (s *SettingsService) ValidatePassword(password string) bool {
    hashedPassword := os.Getenv("SETTINGS_PASSWORD_HASH")
    err := bcrypt.CompareHashAndPassword([]byte(hashedPassword), []byte(password))
    return err == nil
}
```

### WiFi Password Encryption
Encrypt WiFi password before storing:
```go
import "crypto/aes"

// Encrypt before saving to DB
func encryptPassword(password string) (string, error) {
    // Implementation
}

// Decrypt when sending to device
func decryptPassword(encrypted string) (string, error) {
    // Implementation
}
```

### Rate Limiting
Add rate limiting middleware:
```go
import "golang.org/x/time/rate"

func rateLimitMiddleware(next http.Handler) http.Handler {
    limiter := rate.NewLimiter(10, 20) // 10 req/sec, burst of 20
    
    return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
        if !limiter.Allow() {
            http.Error(w, "Rate limit exceeded", http.StatusTooManyRequests)
            return
        }
        next.ServeHTTP(w, r)
    })
}
```

## Support

- Frontend implementation: `IOT-Frontend-main/SETTINGS_TAB_IMPLEMENTATION.md`
- Backend requirements: `IOT-Frontend-main/SETTINGS_TAB_BACKEND_REQUIREMENTS.md`
- Quick start: `IOT-Frontend-main/SETTINGS_TAB_QUICKSTART.md`
