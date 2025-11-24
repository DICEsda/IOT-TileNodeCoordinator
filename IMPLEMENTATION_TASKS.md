# Implementation Tasks for Single Node Development Phase

## Completed ✅

1. **Comprehensive .env file created** (`.env.complete`)
   - All configuration options documented with comments
   - WiFi, MQTT, Google Home, all credentials
   - Copy to `.env` and fill in your values

2. **Radar visualization changed to cone view**
   - X-axis: -3m to +3m (cone width)
   - Y-axis: 0m to 6m (forward distance)
   - Represents 120° FOV from sensor

3. **Settings component template created** (`settings-new.component.html`)
   - Coordinator control section
   - Node management (single node)
   - Zone assignment
   - LED color testing
   - WiFi configuration
   - MQTT configuration
   - Google Home integration
   - System settings

## To Complete 🔨

### 1. Frontend - Settings Component TypeScript

**File:** `IOT-Frontend-main/IOT-Frontend-main/src/app/features/dashboard/tabs/settings/settings.component.ts`

**Required signals/properties:**
```typescript
// Coordinator
coordinatorId = 'coord001';
coordinatorOnline = signal(false);
firmwareVersion = '1.0.0';
pairingMode = signal(false);
pairingTimeLeft = 0;

// Node (single node)
node = signal<NodeStatus | null>(null);
nodeZone = '';
nodeName = '';
nodeBrightness = 80;
testColor = '#00FFBF';

// Zones
zones = ['Living Room', 'Bedroom', 'Kitchen', 'Bathroom'];

// WiFi
wifiSSID = '';
wifiPassword = '';

// MQTT
mqttBroker = 'tcp://mosquitto:1883';
mqttUsername = '';
mqttPassword = '';

// Google Home
googleHomeEnabled = false;
googleHomeConnected = false;
googleProjectId = '';
googleClientId = '';
googleClientSecret = '';
googleApiKey = '';

// System
autoMode = true;
motionSensitivity = 50;
lightIntensity = 80;
autoOffDelay = 30;
```

**Required methods:**
```typescript
enterPairingMode() {
  // POST /api/v1/coordinator/pair
  // Start 60s countdown timer
}

restartCoordinator() {
  // POST /api/v1/coordinator/restart
}

deleteNode() {
  // DELETE /api/v1/nodes/{nodeId}
  // Confirm dialog first
}

updateNodeZone() {
  // PUT /api/v1/nodes/{nodeId}/zone
}

updateNodeName() {
  // PUT /api/v1/nodes/{nodeId}/name
}

sendTestColor() {
  // POST /api/v1/nodes/{nodeId}/test-color
  // Send RGB color as command
}

turnOffLEDs() {
  // POST /api/v1/nodes/{nodeId}/off
}

updateBrightness() {
  // POST /api/v1/nodes/{nodeId}/brightness
}

addZone() {
  zones.push('New Zone');
}

removeZone(index: number) {
  zones.splice(index, 1);
}

updateWiFiConfig() {
  // POST /api/v1/coordinator/wifi
}

connectGoogleHome() {
  // Open OAuth flow
  window.open('/api/v1/google/auth', '_blank');
}

disconnectGoogleHome() {
  // POST /api/v1/google/disconnect
}

saveSettings() {
  // PUT /api/v1/settings
  // Save all configuration
}
```

### 2. Backend - API Endpoints

**File:** `IOT-Backend-main/IOT-Backend-main/internal/http/handlers.go` (or create coordinator_handler.go)

**Required endpoints:**

```go
// Coordinator Control
POST   /api/v1/coordinator/pair        // Start pairing mode
POST   /api/v1/coordinator/restart     // Restart coordinator
POST   /api/v1/coordinator/wifi        // Update WiFi config

// Node Management (Single Node)
GET    /api/v1/node                    // Get the single node status
DELETE /api/v1/node                    // Remove/unpair the node
PUT    /api/v1/node/zone               // Update zone assignment
PUT    /api/v1/node/name               // Update node name
POST   /api/v1/node/test-color         // Send test color command
POST   /api/v1/node/off                // Turn off LEDs
POST   /api/v1/node/brightness         // Set brightness

// Zone Management
GET    /api/v1/zones                   // Get all zones
POST   /api/v1/zones                   // Create zone
PUT    /api/v1/zones/:id               // Update zone
DELETE /api/v1/zones/:id               // Delete zone

// Settings
GET    /api/v1/settings                // Get all settings
PUT    /api/v1/settings                // Update settings

// Google Home
GET    /api/v1/google/auth             // Initiate OAuth flow
GET    /api/v1/google/callback         // OAuth callback
POST   /api/v1/google/disconnect       // Unlink account
POST   /api/v1/google/sync             // Sync devices to Google
```

### 3. Backend - Google Home Handler

**File:** `IOT-Backend-main/IOT-Backend-main/internal/googlehome/handler.go`

**Implement:**
- OAuth2 flow (authorization_code grant type)
- Token storage in MongoDB
- Device sync with HomeGraph API
- Intent handlers:
  - `action.devices.SYNC` - List devices
  - `action.devices.QUERY` - Get device state
  - `action.devices.EXECUTE` - Control devices
  - `action.devices.DISCONNECT` - Unlink account

**Reference:** https://developers.google.com/assistant/smarthome/develop/process-intents

### 4. Backend - MQTT Command Publishing

**File:** `IOT-Backend-main/IOT-Backend-main/internal/mqtt/publisher.go`

**Required publish methods:**
```go
func (p *Publisher) StartPairing(duration int) error {
    // Publish to: site/{siteId}/coord/{coordId}/cmd
    // Payload: {"cmd":"pair","duration_ms":60000}
}

func (p *Publisher) SetNodeColor(nodeId string, r, g, b, w uint8) error {
    // Publish to: site/{siteId}/node/{nodeId}/cmd
    // Payload: {"cmd":"set_color","r":r,"g":g,"b":b,"w":w}
}

func (p *Publisher) SetNodeBrightness(nodeId string, brightness uint8) error {
    // Payload: {"cmd":"set_brightness","value":brightness}
}

func (p *Publisher) TurnOffNode(nodeId string) error {
    // Payload: {"cmd":"off"}
}

func (p *Publisher) RestartCoordinator() error {
    // Payload: {"cmd":"restart"}
}

func (p *Publisher) UpdateWiFi(ssid, password string) error {
    // Payload: {"cmd":"update_wifi","ssid":ssid,"password":password}
}
```

### 5. Frontend - Light Monitor Update

**File:** `IOT-Frontend-main/IOT-Frontend-main/src/app/features/dashboard/components/light-monitor/light-monitor.component.ts`

**Change to single node:**
```typescript
// Remove loop over 6 nodes
// Show single node with its 4-6 LEDs
// Update bindings to work with single node data
```

### 6. Database Schema Updates

**Create MongoDB collections:**

```javascript
// zones collection
{
  _id: ObjectId,
  site_id: "site001",
  name: "Living Room",
  created_at: ISODate,
  updated_at: ISODate
}

// node_assignments collection
{
  _id: ObjectId,
  site_id: "site001",
  coordinator_id: "coord001",
  node_id: "node001",
  zone_id: ObjectId,
  name: "Main Tile",
  created_at: ISODate,
  updated_at: ISODate
}

// google_tokens collection
{
  _id: ObjectId,
  user_id: "smarttile-user-001",
  access_token: "encrypted",
  refresh_token: "encrypted",
  expires_at: ISODate,
  created_at: ISODate
}

// settings collection
{
  _id: ObjectId,
  site_id: "site001",
  auto_mode: true,
  motion_sensitivity: 50,
  light_intensity: 80,
  auto_off_delay: 30,
  created_at: ISODate,
  updated_at: ISODate
}
```

## Testing Steps

### 1. Test Pairing Flow
1. Click "Enter Pairing Mode" in settings
2. Verify 60s countdown shows
3. Verify coordinator LED status changes (addressable strip)
4. Press button on node to pair
5. Verify node appears in Node Management section

### 2. Test LED Control
1. Select color from color picker
2. Click to send test color
3. Verify node LEDs change color
4. Adjust brightness slider
5. Verify brightness changes
6. Click "Turn Off"
7. Verify LEDs turn off

### 3. Test Zone Assignment
1. Create new zone
2. Assign node to zone
3. Verify zone saved in backend
4. Delete zone
5. Verify node zone assignment cleared

### 4. Test Node Deletion
1. Click delete button on node
2. Confirm deletion
3. Verify node removed from system
4. Verify can pair again

### 5. Test Google Home
1. Fill in Google credentials
2. Enable Google Home toggle
3. Click "Link Account"
4. Complete OAuth flow
5. Say "Hey Google, turn on lights"
6. Verify lights turn on via MQTT

## Priority Order

1. ✅ `.env.complete` file (DONE)
2. ✅ Radar cone visualization (DONE)
3. ✅ Settings template (DONE)
4. 🔨 Backend API endpoints for coordinator/node control
5. 🔨 Frontend settings component TypeScript
6. 🔨 MQTT command publishers
7. 🔨 Update light monitor for single node
8. 🔨 Google Home OAuth flow
9. 🔨 Google Home intent handlers
10. 🔨 Database migrations for new collections

## Files to Replace

1. Replace `settings.component.html` with `settings-new.component.html`
2. Rewrite `settings.component.ts` with new functionality
3. Update `.env.example` with `.env.complete` contents
4. Update `light-monitor.component` for single node

## Notes

- All API endpoints should publish MQTT commands to coordinator
- Coordinator firmware already handles these commands (see coordinator code)
- Node pairing uses ESP-NOW callbacks (already implemented)
- LED colors sent as RGBW (4 channels)
- Brightness is 0-100% in API, converted to 0-255 for ESP32
- Google Home requires HTTPS in production (use ngrok for testing)
