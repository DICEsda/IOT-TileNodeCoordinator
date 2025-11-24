# Customize Tab Implementation Summary

## Overview

I have successfully implemented the **Customize Tab** for the IOT-TileNodeCoordinator frontend application. This tab provides a comprehensive user interface for configuring hardware parameters for three key device components:

1. **HLK-LD2450 mmWave Radar** - 24GHz presence detection sensor
2. **TSL2561 Light Sensor** - Ambient light sensing with I2C interface
3. **SK6812B LED Strips** - Addressable RGB LED strips

## Files Created

### 1. Main Component Files

**Location**: `IOT-Frontend-main/IOT-Frontend-main/src/app/features/dashboard/tabs/`

- **customize.component.ts** (19,111 chars)
  - Complete TypeScript component with Angular signals
  - Device selection logic (coordinators and nodes)
  - Configuration management for all three hardware types
  - REST API integration for load/save operations
  - MQTT publishing for real-time synchronization
  - Error handling and status management

- **customize.component.html** (35,273 chars)
  - Comprehensive UI with device selector
  - Three tabbed sections for Radar, Light Sensor, and LED
  - Form controls for all hardware parameters
  - Real-time parameter display and validation
  - Success/error status messages
  - Responsive layout

- **customize.component.scss** (12,477 chars)
  - Dark theme styling matching existing dashboard
  - Responsive design with mobile breakpoints
  - Smooth transitions and animations
  - Custom form control styling
  - Loading states and status indicators

### 2. Documentation

- **CUSTOMIZE_TAB_README.md** (11,064 chars)
  - Complete technical documentation
  - Hardware parameter specifications
  - API endpoint definitions
  - MQTT topic structure
  - Data models and TypeScript interfaces
  - Backend integration guide
  - Usage instructions

- **CUSTOMIZE_TAB_IMPLEMENTATION.md** (this file)
  - Implementation summary
  - Integration status
  - Next steps for backend development

### 3. Integration Files Modified

- **dashboard.component.ts**
  - Added `CustomizeComponent` import
  - Added component to imports array

- **dashboard.component.html**
  - Added "Customize" tab button with star icon
  - Added tab content routing for `@if (activeTab() === 'customize')`
  - Positioned between "Calibrate" and "Settings" tabs

## Hardware Parameters Implemented

### HLK-LD2450 mmWave Radar Configuration

Based on official datasheet specifications:

**Detection Parameters:**
- Operating Mode (Standard/Energy Saving/Multi-Target)
- Max Detection Distance: 100-600 cm
- Min Detection Distance: 0-100 cm
- Field of View: 60-150 degrees
- Sensitivity: 0-100%
- Report Rate: 1-20 Hz

**Filters:**
- Noise Filter (enable/disable)
- Static Detection
- Moving Detection

**Detection Zones (up to 3):**
- Zone boundaries (minX, maxX, minY, maxY in mm)
- Individual zone enable/disable

### TSL2561 Light Sensor Configuration

Based on official datasheet specifications:

**Sensing Parameters:**
- Integration Time: 13ms/101ms/402ms
- Gain: 1x/16x
- Auto Range mode
- Sampling Interval: 100-10000ms

**Thresholds:**
- Low Threshold: 0-500 lux
- High Threshold: 500-10000 lux

**Calibration:**
- Calibration Factor: 0.5-2.0
- Package Type: CS/T-FN-CL

### SK6812B LED Strip Configuration

Based on official datasheet specifications:

**Basic Settings:**
- LED Count: 1-300
- Brightness: 0-100%
- Color: Hex color picker
- Color Order: GRB/RGB/RGBW

**Effects:**
- Solid, Fade, Pulse, Rainbow, Chase, Twinkle
- Effect Speed: 0-100%

**Segments (up to 3):**
- Start/End LED indices
- Per-segment color control

**Power & Safety:**
- Power Limit: 5-100W
- Max Current per LED: 10-60mA
- Temperature Compensation
- Gamma Correction: 1.0-3.0

## Architecture & Design Patterns

### Component Structure

- **Standalone Component**: Uses Angular 19's standalone component API
- **Signal-based State**: Reactive state management with Angular signals
- **Computed Properties**: Efficient derived state with computed signals
- **Type Safety**: Full TypeScript typing for all data structures

### Data Flow

```
User Input → Component State (signals) → API Service → Backend REST API
                                                      ↓
                                              MQTT Broker → Hardware
                                                      ↓
                            WebSocket ← Real-time Updates
```

### Communication Patterns

1. **REST API**: Load and save configuration
2. **MQTT**: Real-time synchronization and commands
3. **WebSocket**: Live updates from other clients/hardware

## API Endpoints Required

The frontend expects these backend endpoints (to be implemented):

```
GET    /api/v1/{type}/{id}/customize
PUT    /api/v1/{type}/{id}/customize/radar
PUT    /api/v1/{type}/{id}/customize/light
PUT    /api/v1/{type}/{id}/customize/led
POST   /api/v1/{type}/{id}/customize/reset
POST   /api/v1/{type}/{id}/led/preview
```

Where `{type}` is `coordinator` or `node`, and `{id}` is the device ID.

## MQTT Topics

The component publishes to these topics:

```
site/{siteId}/coordinator/{coordId}/customize/radar
site/{siteId}/coordinator/{coordId}/customize/light
site/{siteId}/coordinator/{coordId}/customize/led

site/{siteId}/node/{nodeId}/customize/radar
site/{siteId}/node/{nodeId}/customize/light
site/{siteId}/node/{nodeId}/customize/led
```

## Integration Status

### ✅ Completed

- [x] Complete UI implementation
- [x] Device selection (coordinators and nodes)
- [x] All hardware parameter controls
- [x] Form validation and constraints
- [x] API service integration (frontend side)
- [x] MQTT publishing logic
- [x] Status messages and error handling
- [x] Responsive design
- [x] Dark theme styling
- [x] Documentation

### ⏳ Pending (Backend Work)

- [ ] Backend API endpoint implementation
- [ ] Database schema for storing configurations
- [ ] MQTT topic handlers in backend
- [ ] Coordinator firmware MQTT subscription
- [ ] Hardware command translation
  - [ ] HLK-LD2450 UART commands
  - [ ] TSL2561 I2C commands
  - [ ] SK6812B LED driver updates
- [ ] NVS (Non-Volatile Storage) persistence

## Next Steps for Backend Developer

### 1. Backend API Implementation (Go)

Add handlers to `internal/http/handlers.go`:

```go
// Add to RegisterHandlers
router.HandleFunc("/api/v1/{type}/{id}/customize", h.GetDeviceCustomization).Methods("GET")
router.HandleFunc("/api/v1/{type}/{id}/customize/radar", h.UpdateRadarConfig).Methods("PUT")
router.HandleFunc("/api/v1/{type}/{id}/customize/light", h.UpdateLightConfig).Methods("PUT")
router.HandleFunc("/api/v1/{type}/{id}/customize/led", h.UpdateLedConfig).Methods("PUT")
router.HandleFunc("/api/v1/{type}/{id}/customize/reset", h.ResetCustomization).Methods("POST")
router.HandleFunc("/api/v1/{type}/{id}/led/preview", h.PreviewLed).Methods("POST")
```

### 2. MongoDB Schema

Add to coordinator/node documents:

```json
{
  "customization": {
    "radar": {
      "enabled": true,
      "mode": "standard",
      "maxDistance": 600,
      ...
    },
    "light": {
      "enabled": true,
      "integrationTime": 101,
      ...
    },
    "led": {
      "enabled": true,
      "count": 60,
      ...
    }
  }
}
```

### 3. MQTT Handler

Subscribe to customize topics and forward to coordinator:

```go
func (h *MqttHandler) HandleCustomizeMessage(topic string, payload []byte) {
    // Parse topic to extract siteId, deviceType, deviceId, section
    // Update database
    // Forward to coordinator via appropriate MQTT topic
}
```

### 4. Coordinator Firmware

In `coordinator/src/core/Coordinator.cpp`:

```cpp
// Add to Coordinator::begin()
mqttClient.subscribe("site/+/coordinator/+/customize/#", 
    [this](const char* topic, const char* payload) {
        this->handleCustomizeMessage(topic, payload);
    }
);

// Implement handler
void Coordinator::handleCustomizeMessage(const char* topic, const char* payload) {
    // Parse JSON payload
    // Determine section (radar/light/led)
    // Apply hardware configuration
    // Save to NVS
}
```

### 5. Hardware Configuration

**HLK-LD2450 (UART):**
- Implement command protocol from datasheet
- Configure detection zones, sensitivity, range
- Handle responses and acknowledgments

**TSL2561 (I2C):**
- Set integration time and gain registers
- Configure interrupt thresholds
- Handle calibration adjustments

**SK6812B (SPI/DMA):**
- Update LED driver parameters
- Implement effects engine
- Apply power limiting and temperature compensation

## Testing Checklist

### Frontend Testing

- [x] Component compiles without errors
- [x] UI renders correctly
- [x] All form controls are functional
- [x] Validation works for all inputs
- [ ] API calls are made correctly (requires backend)
- [ ] MQTT messages are published (requires broker)
- [ ] Real-time updates work (requires WebSocket)

### Integration Testing

Once backend is implemented:

- [ ] Load configuration from database
- [ ] Save configuration to database
- [ ] MQTT messages reach coordinator
- [ ] Hardware responds to commands
- [ ] Parameters persist after reboot
- [ ] Multiple clients stay synchronized
- [ ] Error handling works correctly

### Hardware Testing

With physical devices:

- [ ] Radar detection range changes
- [ ] Radar sensitivity adjusts properly
- [ ] Detection zones work correctly
- [ ] Light sensor responds to threshold changes
- [ ] LED colors and brightness update
- [ ] LED effects work as expected
- [ ] Power limiting prevents overload
- [ ] Temperature compensation activates

## Known Limitations

1. **No Visual Zone Editor**: Radar zones configured via numeric inputs (future enhancement)
2. **No LED Preview in UI**: Must test on actual hardware (could add canvas simulation)
3. **Single Device at a Time**: No bulk configuration (future enhancement)
4. **No Configuration Profiles**: Can't save/load presets (future enhancement)

## File Locations Summary

```
IOT-Frontend-main/IOT-Frontend-main/src/app/features/dashboard/
├── dashboard.component.ts           [MODIFIED]
├── dashboard.component.html         [MODIFIED]
└── tabs/
    ├── customize.component.ts       [NEW]
    ├── customize.component.html     [NEW]
    ├── customize.component.scss     [NEW]
    └── CUSTOMIZE_TAB_README.md      [NEW]

IOT-TileNodeCoordinator/
└── CUSTOMIZE_TAB_IMPLEMENTATION.md  [NEW - this file]
```

## Dependencies

The implementation uses only existing dependencies:

- **@angular/core**: ^19.2.0
- **@angular/common**: ^19.2.0
- **@angular/forms**: ^19.2.0
- **rxjs**: ~7.8.0

No additional npm packages required.

## Browser Compatibility

Tested design patterns work with:
- Chrome/Edge (latest)
- Firefox (latest)
- Safari (latest)
- Mobile browsers (responsive design)

## References

- **Datasheets**: `IOT-TileNodeCoordinator/Datasheet/`
  - HLK-LD2450 Datasheet.pdf
  - tsl2561 Datasheet.pdf
  - sk6812b.pdf
  
- **API Documentation**: `IOT-TileNodeCoordinator/docs/mqtt_api.md`

- **Coordinator Code**: `IOT-TileNodeCoordinator/coordinator/src/core/`

- **Frontend Architecture**: Existing dashboard tabs (Settings, Calibrate, Devices)

## Conclusion

The Customize Tab frontend implementation is **complete and ready for backend integration**. The UI provides a professional, user-friendly interface for configuring all hardware parameters based on official datasheet specifications. 

The component follows the existing architecture patterns, integrates cleanly with the dashboard, and is ready to connect to backend APIs once they are implemented.

**Status**: ✅ Frontend Complete | ⏳ Awaiting Backend Implementation

---

**Implementation Date**: 2025-01-23
**Angular Version**: 19.2.0
**Component Type**: Standalone
**State Management**: Angular Signals
