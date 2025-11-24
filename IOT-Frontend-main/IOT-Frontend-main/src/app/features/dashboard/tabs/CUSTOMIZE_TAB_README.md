# Customize Tab Implementation

## Overview

The **Customize Tab** provides a comprehensive UI for configuring hardware parameters for IoT devices in the system. It supports three main categories of device customization:

1. **HLK-LD2450 mmWave Radar** - 24GHz presence detection sensor
2. **TSL2561 Light Sensor** - Ambient light sensing
3. **SK6812B LED Strips** - Addressable RGB LEDs

## Architecture

### Component Structure

```
tabs/
├── customize.component.ts         # Main component
├── customize.component.html       # Template
└── customize.component.scss       # Styles
```

### Key Features

- **Device Selection**: Select coordinators or nodes to customize
- **Real-time Sync**: Changes synchronized via REST API, WebSocket, and MQTT
- **Parameter Validation**: Input validation based on hardware datasheets
- **Live Preview**: Test LED configurations before saving
- **Reset to Defaults**: Factory reset for each hardware section

## Hardware Configuration Parameters

### 1. HLK-LD2450 mmWave Radar

Based on datasheet specifications:

#### Detection Parameters
- **Operating Mode**: Standard, Energy Saving, Multi-Target
- **Max Detection Distance**: 100-600 cm
- **Min Detection Distance**: 0-100 cm
- **Field of View**: 60-150 degrees (horizontal)
- **Sensitivity**: 0-100% (adjustable detection threshold)
- **Report Rate**: 1-20 Hz (data update frequency)

#### Detection Filters
- **Noise Filter**: Enable/disable noise reduction
- **Static Detection**: Detect stationary presence
- **Moving Detection**: Detect moving targets

#### Detection Zones
Configure up to 3 independent detection zones with custom boundaries:
- **Zone Boundaries**: minX, maxX, minY, maxY (in millimeters)
- **Enable/Disable**: Individual zone control

### 2. TSL2561 Light Sensor

Based on datasheet specifications:

#### Sensing Parameters
- **Integration Time**: 13ms (fast), 101ms (medium), 402ms (high precision)
- **Gain**: 1x (bright), 16x (low light)
- **Auto Range**: Automatic gain adjustment
- **Sampling Interval**: 100-10000 ms

#### Thresholds
- **Low Threshold**: 0-500 lux (trigger when light falls below)
- **High Threshold**: 500-10000 lux (trigger when light exceeds)

#### Calibration
- **Calibration Factor**: 0.5-2.0 (environmental adjustment)
- **Package Type**: CS or T-FN-CL (affects calibration)

### 3. SK6812B LED Strips

Based on datasheet specifications:

#### Basic Configuration
- **LED Count**: 1-300 LEDs
- **Brightness**: 0-100%
- **Default Color**: RGB hex color picker
- **Color Order**: GRB (default), RGB, RGBW

#### Effects
- **Solid**: Static color
- **Fade**: Fade in/out
- **Pulse**: Pulsing effect
- **Rainbow**: Rainbow cycle
- **Chase**: Chase/running lights
- **Twinkle**: Random twinkling

#### Segments
Divide LED strip into up to 3 independent segments:
- **Start/End LED**: Define segment boundaries
- **Segment Color**: Individual color per segment

#### Power & Safety
- **Power Limit**: 5-100 W (maximum power consumption)
- **Max Current**: 10-60 mA per LED (SK6812B: ~60mA at full white)
- **Temperature Compensation**: Auto-reduce brightness at high temps
- **Gamma Correction**: 1.0-3.0 (color correction curve, 2.2 typical)

## API Integration

### REST API Endpoints

The Customize tab uses the following backend endpoints (to be implemented):

```typescript
// Load device configuration
GET /api/v1/{type}/{id}/customize
Response: { radar: {...}, light: {...}, led: {...} }

// Update radar configuration
PUT /api/v1/{type}/{id}/customize/radar
Body: { siteId: string, config: RadarConfig }

// Update light sensor configuration
PUT /api/v1/{type}/{id}/customize/light
Body: { siteId: string, config: LightSensorConfig }

// Update LED configuration
PUT /api/v1/{type}/{id}/customize/led
Body: { siteId: string, config: LedConfig }

// Reset to factory defaults
POST /api/v1/{type}/{id}/customize/reset
Body: { siteId: string, section: 'radar' | 'light' | 'led' }

// LED preview (5 second test)
POST /api/v1/{type}/{id}/led/preview
Body: { siteId: string, color: string, brightness: number, effect: string, duration: number }
```

Where `{type}` is either `coordinator` or `node`, and `{id}` is the device ID.

### MQTT Topics

Real-time synchronization via MQTT:

```
site/{siteId}/coordinator/{coordId}/customize/radar
site/{siteId}/coordinator/{coordId}/customize/light
site/{siteId}/coordinator/{coordId}/customize/led

site/{siteId}/node/{nodeId}/customize/radar
site/{siteId}/node/{nodeId}/customize/light
site/{siteId}/node/{nodeId}/customize/led
```

**Payload Format:**
```json
{
  "enabled": boolean,
  // Configuration parameters specific to each section
  ...
}
```

### WebSocket Events

The component listens for configuration updates via WebSocket to reflect changes made by other clients or the hardware itself.

## Usage

### For Users

1. **Select Device**: Choose a coordinator or node from the device list
2. **Choose Section**: Select Radar, Light Sensor, or LED configuration
3. **Adjust Parameters**: Use sliders, inputs, and toggles to customize
4. **Test (LED only)**: Preview LED configuration for 5 seconds
5. **Save**: Apply configuration to the device
6. **Reset**: Restore factory defaults if needed

### For Developers

#### Backend Integration Required

The backend needs to implement the following:

1. **API Routes**: Add the customize endpoints to the HTTP handler
2. **Configuration Storage**: Store device customization in MongoDB
3. **MQTT Handler**: Subscribe to customize topics and forward to hardware
4. **Hardware Commands**: Translate UI parameters to device-specific commands

#### Example Backend Handler (Go)

```go
func (h *Handler) UpdateRadarConfig(w http.ResponseWriter, r *http.Request) {
    var req struct {
        SiteID string      `json:"siteId"`
        Config RadarConfig `json:"config"`
    }
    
    if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
        http.Error(w, err.Error(), http.StatusBadRequest)
        return
    }
    
    // Save to database
    err := h.Repo.SaveRadarConfig(deviceID, req.Config)
    
    // Publish to MQTT
    topic := fmt.Sprintf("site/%s/coordinator/%s/customize/radar", req.SiteID, deviceID)
    payload, _ := json.Marshal(req.Config)
    h.mqttClient.Publish(topic, 0, false, payload)
    
    json.NewEncoder(w).Encode(map[string]bool{"success": true})
}
```

#### Coordinator Firmware Integration

The coordinator firmware needs to:

1. Subscribe to customize MQTT topics
2. Parse configuration payloads
3. Apply hardware settings:
   - Configure HLK-LD2450 via UART commands
   - Configure TSL2561 via I2C
   - Update SK6812B LED driver parameters
4. Persist configuration in NVS (Non-Volatile Storage)
5. Publish acknowledgment messages

## Data Models

### Radar Configuration

```typescript
interface RadarConfig {
  enabled: boolean;
  mode: 'standard' | 'energy-saving' | 'multi-target';
  maxDistance: number;        // cm, 100-600
  minDistance: number;        // cm, 0-100
  fieldOfView: number;        // degrees, 60-150
  sensitivity: number;        // 0-100
  reportRate: number;         // Hz, 1-20
  filterEnabled: boolean;
  staticDetection: boolean;
  movingDetection: boolean;
  zones: Array<{
    enabled: boolean;
    minX: number;             // mm
    maxX: number;             // mm
    minY: number;             // mm
    maxY: number;             // mm
  }>;
}
```

### Light Sensor Configuration

```typescript
interface LightSensorConfig {
  enabled: boolean;
  integrationTime: 13 | 101 | 402;  // ms
  gain: 1 | 16;                      // 1x or 16x
  autoRange: boolean;
  lowThreshold: number;               // lux
  highThreshold: number;              // lux
  samplingInterval: number;           // ms
  calibrationFactor: number;          // 0.5-2.0
  packageType: 'CS' | 'T-FN-CL';
}
```

### LED Configuration

```typescript
interface LedConfig {
  enabled: boolean;
  count: number;                      // 1-300
  brightness: number;                 // 0-100
  color: string;                      // hex color
  effect: 'solid' | 'fade' | 'pulse' | 'rainbow' | 'chase' | 'twinkle';
  effectSpeed: number;                // 0-100
  segments: number;                   // 1-3
  segmentConfig: Array<{
    start: number;
    end: number;
    color: string;
  }>;
  powerLimit: number;                 // watts
  maxCurrent: number;                 // mA per LED
  temperatureCompensation: boolean;
  gammaCorrection: number;            // 1.0-3.0
  colorOrder: 'GRB' | 'RGB' | 'RGBW';
}
```

## State Management

The component uses Angular signals for reactive state:

- **selectedDevice**: Currently selected device
- **activeSection**: Active configuration section (radar/light/led)
- **Configuration signals**: Separate signals for each parameter
- **Status signals**: loading, saving, saveSuccess, errorMessage

## Styling

The component follows the existing dashboard design system:

- **Dark theme** with #1a1a1a and #2a2a2a backgrounds
- **Accent color** #00ffbf (cyan/green)
- **Smooth transitions** and hover effects
- **Responsive design** with mobile breakpoints

## Testing

### Manual Testing Checklist

- [ ] Device selection (coordinator/node)
- [ ] Load existing configuration
- [ ] Modify radar parameters
- [ ] Modify light sensor parameters
- [ ] Modify LED parameters
- [ ] LED preview functionality
- [ ] Save configuration
- [ ] Reset to defaults
- [ ] Error handling (network failures)
- [ ] Multiple devices switching
- [ ] Real-time sync via MQTT

### Integration Testing

- [ ] Backend API endpoints respond correctly
- [ ] Configuration persists to database
- [ ] MQTT messages are published
- [ ] Coordinator receives and applies configuration
- [ ] Hardware parameters actually change

## Future Enhancements

1. **Visual Radar Zone Editor**: Drag-and-drop zone boundary editor
2. **LED Strip Visualizer**: Real-time preview of LED effects
3. **Configuration Profiles**: Save and load custom presets
4. **Bulk Configuration**: Apply settings to multiple devices
5. **Configuration History**: Track and rollback changes
6. **Advanced Diagnostics**: Live sensor readings and calibration tools

## References

- **HLK-LD2450 Datasheet**: `Datasheet/HLK-LD2450 Datasheet.pdf`
- **TSL2561 Datasheet**: `Datasheet/tsl2561 Datasheet.pdf`
- **SK6812B Datasheet**: `Datasheet/sk6812b.pdf`
- **MQTT API**: `docs/mqtt_api.md`
- **Coordinator Firmware**: `coordinator/src/core/Coordinator.*`

## Support

For questions or issues related to the Customize tab:
- Check the hardware datasheets for parameter specifications
- Review MQTT API documentation for message formats
- Examine coordinator firmware for implementation details
- Test with actual hardware to verify functionality
