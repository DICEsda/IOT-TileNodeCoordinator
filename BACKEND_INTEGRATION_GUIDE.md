# Backend Integration Guide for Customize Tab

## Quick Start for Backend Developer

This guide provides step-by-step instructions for integrating the Customize Tab with the backend.

## Overview

The Customize Tab (frontend) is complete and needs the following backend components:

1. REST API endpoints
2. MongoDB storage
3. MQTT message handlers
4. Coordinator firmware integration

## Step 1: Add API Routes

### File: `internal/http/handlers.go`

Add these routes to `RegisterHandlers()`:

```go
// Customize API
router.HandleFunc("/api/v1/{type}/{id}/customize", h.GetDeviceCustomization).Methods("GET")
router.HandleFunc("/api/v1/{type}/{id}/customize/radar", h.UpdateRadarConfig).Methods("PUT")
router.HandleFunc("/api/v1/{type}/{id}/customize/light", h.UpdateLightConfig).Methods("PUT")
router.HandleFunc("/api/v1/{type}/{id}/customize/led", h.UpdateLedConfig).Methods("PUT")
router.HandleFunc("/api/v1/{type}/{id}/customize/reset", h.ResetCustomization).Methods("POST")
router.HandleFunc("/api/v1/{type}/{id}/led/preview", h.PreviewLed).Methods("POST")
```

### Create Handler File: `internal/http/customize_handlers.go`

```go
package http

import (
	"encoding/json"
	"net/http"
	"github.com/gorilla/mux"
)

// RadarConfig represents HLK-LD2450 configuration
type RadarConfig struct {
	Enabled          bool       `json:"enabled"`
	Mode             string     `json:"mode"` // "standard", "energy-saving", "multi-target"
	MaxDistance      int        `json:"maxDistance"` // cm
	MinDistance      int        `json:"minDistance"` // cm
	FieldOfView      int        `json:"fieldOfView"` // degrees
	Sensitivity      int        `json:"sensitivity"` // 0-100
	ReportRate       int        `json:"reportRate"` // Hz
	FilterEnabled    bool       `json:"filterEnabled"`
	StaticDetection  bool       `json:"staticDetection"`
	MovingDetection  bool       `json:"movingDetection"`
	Zones            []RadarZone `json:"zones"`
}

type RadarZone struct {
	Enabled bool `json:"enabled"`
	MinX    int  `json:"minX"` // mm
	MaxX    int  `json:"maxX"` // mm
	MinY    int  `json:"minY"` // mm
	MaxY    int  `json:"maxY"` // mm
}

// LightSensorConfig represents TSL2561 configuration
type LightSensorConfig struct {
	Enabled           bool    `json:"enabled"`
	IntegrationTime   int     `json:"integrationTime"` // 13, 101, or 402 ms
	Gain              int     `json:"gain"` // 1 or 16
	AutoRange         bool    `json:"autoRange"`
	LowThreshold      int     `json:"lowThreshold"` // lux
	HighThreshold     int     `json:"highThreshold"` // lux
	SamplingInterval  int     `json:"samplingInterval"` // ms
	CalibrationFactor float64 `json:"calibrationFactor"`
	PackageType       string  `json:"packageType"` // "CS" or "T-FN-CL"
}

// LedConfig represents SK6812B LED strip configuration
type LedConfig struct {
	Enabled                 bool           `json:"enabled"`
	Count                   int            `json:"count"`
	Brightness              int            `json:"brightness"` // 0-100
	Color                   string         `json:"color"` // hex color
	Effect                  string         `json:"effect"` // "solid", "fade", "pulse", etc.
	EffectSpeed             int            `json:"effectSpeed"` // 0-100
	Segments                int            `json:"segments"` // 1-3
	SegmentConfig           []LedSegment   `json:"segmentConfig"`
	PowerLimit              int            `json:"powerLimit"` // watts
	MaxCurrent              int            `json:"maxCurrent"` // mA per LED
	TemperatureCompensation bool           `json:"temperatureCompensation"`
	GammaCorrection         float64        `json:"gammaCorrection"`
	ColorOrder              string         `json:"colorOrder"` // "GRB", "RGB", "RGBW"
}

type LedSegment struct {
	Start int    `json:"start"`
	End   int    `json:"end"`
	Color string `json:"color"`
}

// DeviceCustomization holds all customization data
type DeviceCustomization struct {
	Radar *RadarConfig       `json:"radar,omitempty"`
	Light *LightSensorConfig `json:"light,omitempty"`
	Led   *LedConfig         `json:"led,omitempty"`
}

// GetDeviceCustomization retrieves customization for a device
func (h *Handler) GetDeviceCustomization(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"] // "coordinator" or "node"
	deviceID := vars["id"]

	// Load from database
	customization, err := h.Repo.GetDeviceCustomization(deviceType, deviceID)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(customization)
}

// UpdateRadarConfig updates radar configuration
func (h *Handler) UpdateRadarConfig(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"]
	deviceID := vars["id"]

	var req struct {
		SiteID string      `json:"siteId"`
		Config RadarConfig `json:"config"`
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	// Save to database
	if err := h.Repo.SaveRadarConfig(deviceType, deviceID, req.Config); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	// Publish to MQTT
	topic := fmt.Sprintf("site/%s/%s/%s/customize/radar", req.SiteID, deviceType, deviceID)
	payload, _ := json.Marshal(req.Config)
	h.mqttClient.Publish(topic, 0, false, payload)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]bool{"success": true})
}

// UpdateLightConfig updates light sensor configuration
func (h *Handler) UpdateLightConfig(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"]
	deviceID := vars["id"]

	var req struct {
		SiteID string            `json:"siteId"`
		Config LightSensorConfig `json:"config"`
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	// Save to database
	if err := h.Repo.SaveLightConfig(deviceType, deviceID, req.Config); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	// Publish to MQTT
	topic := fmt.Sprintf("site/%s/%s/%s/customize/light", req.SiteID, deviceType, deviceID)
	payload, _ := json.Marshal(req.Config)
	h.mqttClient.Publish(topic, 0, false, payload)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]bool{"success": true})
}

// UpdateLedConfig updates LED strip configuration
func (h *Handler) UpdateLedConfig(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"]
	deviceID := vars["id"]

	var req struct {
		SiteID string    `json:"siteId"`
		Config LedConfig `json:"config"`
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	// Save to database
	if err := h.Repo.SaveLedConfig(deviceType, deviceID, req.Config); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	// Publish to MQTT
	topic := fmt.Sprintf("site/%s/%s/%s/customize/led", req.SiteID, deviceType, deviceID)
	payload, _ := json.Marshal(req.Config)
	h.mqttClient.Publish(topic, 0, false, payload)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]bool{"success": true})
}

// ResetCustomization resets a section to factory defaults
func (h *Handler) ResetCustomization(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"]
	deviceID := vars["id"]

	var req struct {
		SiteID  string `json:"siteId"`
		Section string `json:"section"` // "radar", "light", or "led"
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	// Get factory defaults
	defaults := getFactoryDefaults(req.Section)

	// Save to database
	if err := h.Repo.ResetCustomization(deviceType, deviceID, req.Section, defaults); err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	// Publish to MQTT
	topic := fmt.Sprintf("site/%s/%s/%s/customize/%s", req.SiteID, deviceType, deviceID, req.Section)
	payload, _ := json.Marshal(defaults)
	h.mqttClient.Publish(topic, 0, false, payload)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]bool{"success": true})
}

// PreviewLed sends a temporary LED preview command
func (h *Handler) PreviewLed(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	deviceType := vars["type"]
	deviceID := vars["id"]

	var req struct {
		SiteID     string `json:"siteId"`
		Color      string `json:"color"`
		Brightness int    `json:"brightness"`
		Effect     string `json:"effect"`
		Duration   int    `json:"duration"` // milliseconds
	}

	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	// Publish preview command to MQTT
	topic := fmt.Sprintf("site/%s/%s/%s/led/preview", req.SiteID, deviceType, deviceID)
	payload, _ := json.Marshal(req)
	h.mqttClient.Publish(topic, 0, false, payload)

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]bool{"success": true})
}

// Helper function to get factory defaults
func getFactoryDefaults(section string) interface{} {
	switch section {
	case "radar":
		return RadarConfig{
			Enabled:         true,
			Mode:            "standard",
			MaxDistance:     600,
			MinDistance:     0,
			FieldOfView:     120,
			Sensitivity:     50,
			ReportRate:      10,
			FilterEnabled:   true,
			StaticDetection: true,
			MovingDetection: true,
			Zones: []RadarZone{
				{Enabled: true, MinX: -150, MaxX: 150, MinY: 0, MaxY: 600},
				{Enabled: false, MinX: -300, MaxX: -150, MinY: 0, MaxY: 600},
				{Enabled: false, MinX: 150, MaxX: 300, MinY: 0, MaxY: 600},
			},
		}
	case "light":
		return LightSensorConfig{
			Enabled:           true,
			IntegrationTime:   101,
			Gain:              1,
			AutoRange:         true,
			LowThreshold:      10,
			HighThreshold:     1000,
			SamplingInterval:  1000,
			CalibrationFactor: 1.0,
			PackageType:       "CS",
		}
	case "led":
		return LedConfig{
			Enabled:                 true,
			Count:                   60,
			Brightness:              80,
			Color:                   "#00FFBF",
			Effect:                  "solid",
			EffectSpeed:             50,
			Segments:                1,
			SegmentConfig:           []LedSegment{{Start: 0, End: 60, Color: "#00FFBF"}},
			PowerLimit:              30,
			MaxCurrent:              60,
			TemperatureCompensation: true,
			GammaCorrection:         2.2,
			ColorOrder:              "GRB",
		}
	}
	return nil
}
```

## Step 2: Add Repository Methods

### File: `internal/repository/repository.go`

Add these methods to the `Repository` interface:

```go
// Customization methods
GetDeviceCustomization(deviceType, deviceID string) (*DeviceCustomization, error)
SaveRadarConfig(deviceType, deviceID string, config RadarConfig) error
SaveLightConfig(deviceType, deviceID string, config LightSensorConfig) error
SaveLedConfig(deviceType, deviceID string, config LedConfig) error
ResetCustomization(deviceType, deviceID string, section string, defaults interface{}) error
```

### File: `internal/repository/mongodb.go`

Implement the methods:

```go
func (r *MongoRepository) GetDeviceCustomization(deviceType, deviceID string) (*DeviceCustomization, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	collection := r.getCollection(deviceType)
	
	var result struct {
		Customization DeviceCustomization `bson:"customization"`
	}

	filter := bson.M{}
	if deviceType == "coordinator" {
		filter["coord_id"] = deviceID
	} else {
		filter["node_id"] = deviceID
	}

	err := collection.FindOne(ctx, filter).Decode(&result)
	if err != nil {
		if err == mongo.ErrNoDocuments {
			// Return default configuration
			return getDefaultCustomization(), nil
		}
		return nil, err
	}

	return &result.Customization, nil
}

func (r *MongoRepository) SaveRadarConfig(deviceType, deviceID string, config RadarConfig) error {
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	collection := r.getCollection(deviceType)
	
	filter := bson.M{}
	if deviceType == "coordinator" {
		filter["coord_id"] = deviceID
	} else {
		filter["node_id"] = deviceID
	}

	update := bson.M{
		"$set": bson.M{
			"customization.radar": config,
			"updated_at":          time.Now(),
		},
	}

	_, err := collection.UpdateOne(ctx, filter, update, options.Update().SetUpsert(true))
	return err
}

// Similar implementations for SaveLightConfig, SaveLedConfig, ResetCustomization
```

## Step 3: Update MongoDB Schema

Add `customization` field to coordinator and node collections:

```javascript
// MongoDB migration or manual update
db.coordinators.updateMany(
  {},
  {
    $set: {
      customization: {
        radar: {
          enabled: true,
          mode: "standard",
          maxDistance: 600,
          // ... other defaults
        },
        light: {
          enabled: true,
          integrationTime: 101,
          // ... other defaults
        },
        led: {
          enabled: true,
          count: 60,
          // ... other defaults
        }
      }
    }
  }
);
```

## Step 4: MQTT Subscription in Coordinator

### File: `coordinator/src/managers/MqttManager.cpp`

Subscribe to customize topics:

```cpp
void MqttManager::begin() {
    // Existing subscriptions...
    
    // Subscribe to customization topics
    String radarTopic = "site/" + siteId + "/coordinator/" + coordId + "/customize/radar";
    mqttClient.subscribe(radarTopic.c_str(), 0);
    
    String lightTopic = "site/" + siteId + "/coordinator/" + coordId + "/customize/light";
    mqttClient.subscribe(lightTopic.c_str(), 0);
    
    String ledTopic = "site/" + siteId + "/coordinator/" + coordId + "/customize/led";
    mqttClient.subscribe(ledTopic.c_str(), 0);
    
    String previewTopic = "site/" + siteId + "/coordinator/" + coordId + "/led/preview";
    mqttClient.subscribe(previewTopic.c_str(), 0);
}

void MqttManager::onMessage(char* topic, byte* payload, unsigned int length) {
    String topicStr = String(topic);
    
    if (topicStr.indexOf("/customize/radar") > 0) {
        handleRadarConfig(payload, length);
    } else if (topicStr.indexOf("/customize/light") > 0) {
        handleLightConfig(payload, length);
    } else if (topicStr.indexOf("/customize/led") > 0) {
        handleLedConfig(payload, length);
    } else if (topicStr.indexOf("/led/preview") > 0) {
        handleLedPreview(payload, length);
    }
}
```

## Step 5: Hardware Configuration Handlers

### File: `coordinator/src/managers/RadarManager.cpp` (new or update existing)

```cpp
void RadarManager::applyConfig(const RadarConfig& config) {
    if (!config.enabled) {
        // Disable radar
        return;
    }
    
    // Send UART commands to HLK-LD2450
    // Example commands (refer to datasheet for exact format):
    
    // Set max distance
    sendCommand(0x60, config.maxDistance);
    
    // Set sensitivity
    sendCommand(0x64, config.sensitivity);
    
    // Configure detection zones
    for (int i = 0; i < config.zones.size(); i++) {
        if (config.zones[i].enabled) {
            configureZone(i, config.zones[i]);
        }
    }
    
    // Save to NVS
    saveConfigToNVS(config);
}

void RadarManager::sendCommand(uint8_t cmd, uint16_t value) {
    // Implement HLK-LD2450 UART protocol
    uint8_t packet[10];
    packet[0] = 0xFD;  // Header
    packet[1] = 0xFC;
    packet[2] = 0xFB;
    packet[3] = 0xFA;
    packet[4] = cmd;
    packet[5] = value & 0xFF;
    packet[6] = (value >> 8) & 0xFF;
    // ... checksum and footer
    
    Serial2.write(packet, sizeof(packet));
}
```

### File: `coordinator/src/managers/LightSensorManager.cpp`

```cpp
void LightSensorManager::applyConfig(const LightSensorConfig& config) {
    if (!config.enabled) {
        return;
    }
    
    // Configure TSL2561 via I2C
    
    // Set integration time
    uint8_t timing = 0;
    switch (config.integrationTime) {
        case 13:  timing = 0x00; break;
        case 101: timing = 0x01; break;
        case 402: timing = 0x02; break;
    }
    
    // Set gain
    if (config.gain == 16) {
        timing |= 0x10;  // Gain bit
    }
    
    Wire.beginTransmission(TSL2561_ADDR);
    Wire.write(0x80 | 0x01);  // Command + TIMING register
    Wire.write(timing);
    Wire.endTransmission();
    
    // Set thresholds (similar I2C writes)
    
    // Save to NVS
    saveConfigToNVS(config);
}
```

### File: `coordinator/src/managers/LedManager.cpp`

```cpp
void LedManager::applyConfig(const LedConfig& config) {
    if (!config.enabled) {
        FastLED.clear();
        FastLED.show();
        return;
    }
    
    // Update LED count
    if (config.count != currentCount) {
        // Reinitialize FastLED
        FastLED.addLeds<SK6812, LED_PIN, GRB>(leds, config.count);
    }
    
    // Apply color
    CRGB color = hexToRGB(config.color);
    
    // Apply effect
    switch (config.effect) {
        case "solid":
            fill_solid(leds, config.count, color);
            break;
        case "fade":
            applyFadeEffect(color, config.effectSpeed);
            break;
        case "rainbow":
            applyRainbowEffect(config.effectSpeed);
            break;
        // ... other effects
    }
    
    // Apply brightness
    FastLED.setBrightness(map(config.brightness, 0, 100, 0, 255));
    
    // Apply segments if configured
    if (config.segments > 1) {
        applySegments(config.segmentConfig);
    }
    
    FastLED.show();
    
    // Save to NVS
    saveConfigToNVS(config);
}
```

## Step 6: Testing

1. **Start the backend**:
   ```bash
   cd IOT-Backend-main/IOT-Backend-main
   go run cmd/iot/main.go
   ```

2. **Start the frontend**:
   ```bash
   cd IOT-Frontend-main/IOT-Frontend-main
   npm start
   ```

3. **Test API endpoints** using curl or Postman:
   ```bash
   # Get configuration
   curl http://localhost:8080/api/v1/coordinator/coord001/customize
   
   # Update radar config
   curl -X PUT http://localhost:8080/api/v1/coordinator/coord001/customize/radar \
     -H "Content-Type: application/json" \
     -d '{"siteId":"site001","config":{"enabled":true,"maxDistance":500}}'
   ```

4. **Monitor MQTT messages**:
   ```bash
   mosquitto_sub -h localhost -t 'site/#' -v
   ```

5. **Flash and test coordinator** with new configuration handlers

## Troubleshooting

- **CORS Issues**: Ensure backend allows requests from frontend origin
- **MQTT Not Publishing**: Check MQTT client connection and topics
- **Database Errors**: Verify MongoDB connection and collection names
- **Hardware Not Responding**: Check UART/I2C connections and command formats

## Success Criteria

- [ ] Frontend loads existing configuration
- [ ] Changes save to database
- [ ] MQTT messages are published
- [ ] Coordinator receives messages
- [ ] Hardware parameters actually change
- [ ] Configuration persists after reboot
- [ ] Multiple clients stay synchronized

## Support

For questions or issues:
1. Check the `CUSTOMIZE_TAB_README.md` for detailed API specs
2. Review hardware datasheets for command formats
3. Test MQTT messages with mosquitto_pub/sub
4. Enable debug logging in coordinator firmware
