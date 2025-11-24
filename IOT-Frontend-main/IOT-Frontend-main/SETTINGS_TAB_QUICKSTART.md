# Settings Tab Quick Start Guide

## What Was Implemented

A complete, password-protected Settings tab with 9 configuration sections for system-wide environment management. The implementation is **fully functional on the frontend** and ready for backend integration.

## Files Created

### Frontend Implementation (✅ Complete)

```
src/app/core/models/
└── settings.models.ts                    # All TypeScript interfaces

src/app/features/dashboard/tabs/settings/
├── settings-new.component.ts             # Main component logic
├── settings-new.component.html           # Complete UI template
└── settings-new-additional.scss          # Additional styles

Documentation/
├── SETTINGS_TAB_IMPLEMENTATION.md        # Full implementation guide
├── SETTINGS_TAB_BACKEND_REQUIREMENTS.md  # Backend specs
└── SETTINGS_TAB_QUICKSTART.md            # This file
```

### Files Modified

```
src/app/core/services/
└── api.service.ts                        # Added 4 new endpoints

src/app/core/models/
└── index.ts                              # Exported settings models

src/app/features/dashboard/
├── dashboard.component.ts                # Updated imports
└── dashboard.component.html              # Updated selector
```

## Backend Requirements (❌ Not Yet Implemented)

You need to implement these endpoints in your Go backend:

### 1. GET /api/v1/system/settings
```go
// Return all system settings from MongoDB
```

### 2. PUT /api/v1/system/settings
```go
// Update a specific section of settings
// Body: { section: string, settings: object }
```

### 3. POST /api/v1/system/settings/validate-password
```go
// Validate password (currently "1234")
// Body: { password: string }
// Response: { valid: boolean }
```

### 4. POST /api/v1/system/settings/reset
```go
// Reset all settings to defaults
```

## MongoDB Schema

Create a collection named `system_settings` with this structure:

```javascript
{
  _id: ObjectId,
  updated_at: Date,
  system_identity: {
    default_site_id: "site001",
    default_coord_id: "coord001",
    max_coordinators_per_site: 10,
    max_nodes_per_coordinator: 250
  },
  network: {
    esp32_wifi_ssid: "",
    esp32_wifi_password: "",  // Encrypted
    esp32_wifi_timeout: 10000,
    esp32_reconnect_delay: 5000,
    esp32_ping_interval: 30000,
    api_url: "http://localhost:3000",
    ws_url: "ws://localhost:3000/ws"
  },
  mqtt: {
    mqtt_qos: 1,
    mqtt_keepalive: 60,
    mqtt_client_id: "smartlight-backend"
  },
  coordinator: {
    esp32_firmware_version: "1.0.0",
    esp32_ota_url: "",
    coord_led_count: 12,
    coord_button_hold_time: 3000,
    coord_max_nodes: 250
  },
  node: {
    node_led_count: 60,
    node_default_brightness: 128,
    node_temp_sensor_enabled: true,
    node_button_enabled: true
  },
  zones: {
    default_zones: ["Living Room", "Bedroom", "Kitchen", "Bathroom", "Office", "Hallway"],
    allow_custom_zones: true,
    max_zones: 50
  },
  automation: {
    energy_saving_mode: false,
    auto_off_delay: 30,
    motion_sensitivity: 50,
    default_light_intensity: 80
  },
  telemetry: {
    telemetry_retention_days: 30,
    debug_mode: false,
    metrics_interval: 60,
    log_to_file: true
  },
  developer: {
    mock_devices_enabled: false,
    simulator_enabled: false,
    simulator_interval: 1000
  }
}
```

## Testing Without Backend

The Settings tab will show authentication screen but won't work without backend. To test UI only:

1. Temporarily modify `settings-new.component.ts`:
```typescript
async authenticate() {
  // Comment out API call and force success
  this.isAuthenticated.set(true);
  this.loadSettings();
}

async loadSettings() {
  // Mock data for testing
  const mockSettings: SystemSettings = {
    system_identity: {
      default_site_id: 'site001',
      default_coord_id: 'coord001',
      max_coordinators_per_site: 10,
      max_nodes_per_coordinator: 250
    },
    // ... add other sections as needed
  };
  this.settings.set(mockSettings);
}
```

2. Run `npm start` and navigate to Settings tab
3. Enter any password (will be bypassed)
4. Test UI interactions

## Implementation Priority

### Phase 1: Basic Backend (Required for Testing)
1. ✅ Create endpoint: `POST /api/v1/system/settings/validate-password`
   - Hard-code check for password "1234"
   - Return `{ valid: true }` if match

2. ✅ Create endpoint: `GET /api/v1/system/settings`
   - Return default settings (hard-coded for now)
   - No database yet

3. Test frontend authentication and UI

### Phase 2: Database Integration
1. ✅ Create MongoDB model for `system_settings`
2. ✅ Initialize default settings on first run
3. ✅ Implement `GET` to read from database
4. ✅ Implement `PUT` to update database
5. Test save functionality

### Phase 3: Real-Time Updates
1. ✅ Add MQTT publisher for settings changes
2. ✅ Update WebSocket broadcaster
3. ✅ Publish to device configuration topics
4. Test multi-client synchronization

### Phase 4: Device Integration
1. ✅ Coordinators listen to `site/{siteId}/coord/{coordId}/config`
2. ✅ Nodes listen to `site/{siteId}/node/{nodeId}/config`
3. ✅ Apply configuration changes automatically
4. Test end-to-end flow

## How to Start Backend Implementation

### Go Backend Example (Minimal)

```go
// handlers/settings.go

package handlers

import (
    "encoding/json"
    "net/http"
)

type SettingsHandler struct {
    // Add your dependencies
}

// Validate password endpoint
func (h *SettingsHandler) ValidatePassword(w http.ResponseWriter, r *http.Request) {
    var req struct {
        Password string `json:"password"`
    }
    
    if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
        http.Error(w, err.Error(), http.StatusBadRequest)
        return
    }
    
    // TODO: Get password from env variable
    const SETTINGS_PASSWORD = "1234"
    
    response := map[string]bool{
        "valid": req.Password == SETTINGS_PASSWORD,
    }
    
    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// Get settings endpoint
func (h *SettingsHandler) GetSettings(w http.ResponseWriter, r *http.Request) {
    // TODO: Get from database
    // For now, return defaults
    settings := getDefaultSettings()
    
    response := map[string]interface{}{
        "success": true,
        "data": settings,
    }
    
    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// Update settings endpoint
func (h *SettingsHandler) UpdateSettings(w http.ResponseWriter, r *http.Request) {
    var req struct {
        Section  string                 `json:"section"`
        Settings map[string]interface{} `json:"settings"`
    }
    
    if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
        http.Error(w, err.Error(), http.StatusBadRequest)
        return
    }
    
    // TODO: Update database
    // TODO: Publish MQTT update
    
    response := map[string]interface{}{
        "success": true,
        "message": "Settings updated successfully",
    }
    
    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

// Reset settings endpoint
func (h *SettingsHandler) ResetSettings(w http.ResponseWriter, r *http.Request) {
    // TODO: Reset database to defaults
    
    response := map[string]interface{}{
        "success": true,
        "message": "Settings reset to defaults",
    }
    
    w.Header().Set("Content-Type", "application/json")
    json.NewEncoder(w).Encode(response)
}

func getDefaultSettings() map[string]interface{} {
    return map[string]interface{}{
        "system_identity": map[string]interface{}{
            "default_site_id": "site001",
            "default_coord_id": "coord001",
            "max_coordinators_per_site": 10,
            "max_nodes_per_coordinator": 250,
        },
        // ... add other sections
    }
}
```

### Register Routes

```go
// main.go or routes.go

settingsHandler := &handlers.SettingsHandler{}

router.HandleFunc("/api/v1/system/settings", settingsHandler.GetSettings).Methods("GET")
router.HandleFunc("/api/v1/system/settings", settingsHandler.UpdateSettings).Methods("PUT")
router.HandleFunc("/api/v1/system/settings/validate-password", settingsHandler.ValidatePassword).Methods("POST")
router.HandleFunc("/api/v1/system/settings/reset", settingsHandler.ResetSettings).Methods("POST")
```

## Verification Checklist

Frontend (✅ Complete):
- [x] TypeScript interfaces defined
- [x] Password authentication screen
- [x] 9 configuration sections with all controls
- [x] Individual section save buttons
- [x] Bulk "Save All" button
- [x] Reset to defaults button
- [x] Zone management (add/remove/edit)
- [x] Developer mode toggle
- [x] Real-time update handlers
- [x] Responsive design
- [x] API service methods
- [x] Component routing

Backend (❌ To Be Implemented):
- [ ] Password validation endpoint
- [ ] Get settings endpoint
- [ ] Update settings endpoint
- [ ] Reset settings endpoint
- [ ] MongoDB schema
- [ ] MQTT publisher for updates
- [ ] WebSocket broadcaster
- [ ] Device configuration topics

## Next Actions

1. **Backend Developer**: Implement the 4 REST endpoints listed above
2. **Database Admin**: Create `system_settings` collection with schema
3. **DevOps**: Add `SETTINGS_PASSWORD` to environment variables
4. **Frontend Developer**: Test integration once backend is ready
5. **QA**: Run through testing checklist in SETTINGS_TAB_IMPLEMENTATION.md

## Getting Help

- Full implementation details: `SETTINGS_TAB_IMPLEMENTATION.md`
- Backend specifications: `SETTINGS_TAB_BACKEND_REQUIREMENTS.md`
- TypeScript interfaces: `src/app/core/models/settings.models.ts`
- Component code: `src/app/features/dashboard/tabs/settings/settings-new.component.ts`

## Summary

✅ **Frontend is 100% complete and ready**
❌ **Backend needs implementation**
📚 **Complete documentation provided**

The Settings tab will appear in your dashboard but will require password authentication (which needs backend implementation). Once the backend endpoints are created, the entire feature will be fully functional with real-time synchronization across all devices.
