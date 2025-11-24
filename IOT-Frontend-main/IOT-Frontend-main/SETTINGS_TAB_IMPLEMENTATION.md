# Settings Tab Implementation Guide

## Overview

The Settings tab is a new comprehensive system configuration interface for the SmartLight IoT system. It provides password-protected access to all environment-level settings with real-time synchronization across the entire system.

## Features Implemented

### 1. **Password Protection**
- Default password: `1234`
- Authentication required before accessing any settings
- Password validation through backend API
- Lock/logout functionality to re-secure settings

### 2. **Nine Configuration Sections**

#### a. System Identity
- Default Site ID
- Default Coordinator ID  
- Max Coordinators Per Site (dynamic scaling)
- Max Nodes Per Coordinator (dynamic scaling)

#### b. Network & Connectivity
- ESP32 WiFi SSID
- ESP32 WiFi Password (masked input)
- WiFi Timeout (ms)
- Reconnect Delay (ms)
- Ping Interval (ms)
- API URL (read-only)
- WebSocket URL (read-only)

#### c. MQTT Behavior
- MQTT QoS Level (0, 1, 2)
- Keep Alive (seconds)
- MQTT Client ID

#### d. Coordinator Configuration
- ESP32 Firmware Version (read-only)
- ESP32 OTA URL
- Coordinator LED Count
- Button Hold Time (ms)
- Max Nodes

#### e. Node Defaults
- Node LED Count
- Default Brightness (0-255 with slider)
- Temperature Sensor Enabled (toggle)
- Button Enabled (toggle)

#### f. Zones
- Default Zones (editable list)
- Allow Custom Zones (toggle)
- Max Zones
- Add/Remove/Edit zone functionality

#### g. Energy & Automation
- Energy Saving Mode (toggle)
- Auto-Off Delay (seconds)
- Motion Sensitivity (0-100 with slider)
- Default Light Intensity (0-100 with slider)

#### h. Telemetry & Logging
- Telemetry Retention (days)
- Debug Mode (toggle)
- Metrics Interval (seconds)
- Log to File (toggle)

#### i. Developer Tools (Conditional)
- Only visible when "Enable Developer Mode" is activated
- Mock Devices Enabled (toggle)
- Simulator Enabled (toggle)
- Simulator Interval (ms)

### 3. **Save Functionality**
- **Individual Section Save**: Each section has its own "Save Section" button
- **Bulk Save All**: Single button to save all sections at once
- **Reset to Defaults**: Restore all settings to factory defaults
- **Real-time Feedback**: Success/error messages with animations

### 4. **Real-Time Synchronization**
- WebSocket integration for live updates
- MQTT topic subscriptions for device-level changes
- Automatic UI refresh when backend updates settings
- Multi-client synchronization

### 5. **Responsive Design**
- Collapsible sections for mobile view
- Grid layout adapts to screen size
- Touch-friendly controls
- Optimized for tablets and mobile devices

## File Structure

```
src/app/
├── core/
│   ├── models/
│   │   └── settings.models.ts          # TypeScript interfaces for all settings
│   └── services/
│       └── api.service.ts              # Updated with settings endpoints
├── features/
│   └── dashboard/
│       └── tabs/
│           └── settings/
│               ├── settings-new.component.ts              # Main component logic
│               ├── settings-new.component.html            # Template with all sections
│               ├── settings.component.scss                # Base styles
│               └── settings-new-additional.scss           # Additional styles
```

## Components Created/Modified

### New Files
1. **settings.models.ts** - Complete TypeScript interfaces for:
   - `SystemSettings` (main interface)
   - Individual section interfaces (`SystemIdentitySettings`, `NetworkSettings`, etc.)
   - `SettingsResponse`, `UpdateSettingsRequest`
   - `SettingsUpdateMessage` (WebSocket/MQTT)

2. **settings-new.component.ts** - Main component with:
   - Password authentication logic
   - Settings loading and saving
   - Real-time update handlers
   - Zone management functions
   - Section collapse/expand logic
   - Developer mode toggle

3. **settings-new.component.html** - Comprehensive UI with:
   - Password authentication screen
   - 9 collapsible configuration sections
   - Form controls (inputs, toggles, sliders, selects)
   - Zone list management
   - Action buttons (Save All, Reset, Lock)

4. **settings-new-additional.scss** - Styling for:
   - Collapsible sections
   - Zone management UI
   - Developer mode highlight
   - Save messages and animations

### Modified Files
1. **api.service.ts** - Added endpoints:
   - `getSystemSettings()`
   - `updateSystemSettings(section, settings)`
   - `validateSettingsPassword(password)`
   - `resetSettingsToDefaults()`

2. **dashboard.component.ts** - Updated imports:
   - Changed from `SettingsComponent` to `SettingsNewComponent`

3. **dashboard.component.html** - Updated selector:
   - Changed from `<app-settings>` to `<app-settings-new>`

## Backend Requirements

A comprehensive backend implementation guide has been created:
**SETTINGS_TAB_BACKEND_REQUIREMENTS.md**

This document includes:
- REST API endpoint specifications
- MongoDB schema definitions
- MQTT topic structure
- WebSocket message formats
- Default values
- Security considerations
- Implementation steps
- Testing checklist

## Usage

### For Users
1. Navigate to Settings tab in dashboard
2. Enter password: `1234`
3. Modify any settings as needed
4. Save individual sections or use "Save All"
5. Click "Lock" button to re-secure settings

### For Developers
1. Enable "Developer Mode" checkbox in header
2. Developer Tools section appears at bottom
3. Configure simulator and mock device settings
4. Save changes

### For Administrators
1. Access Network & Connectivity to update WiFi credentials
2. Modify MQTT behavior for message reliability
3. Configure system-wide automation rules
4. Manage zones for room organization
5. Set telemetry retention policies

## Dynamic Scaling

The Settings tab is designed to support any number of coordinators and nodes:

- `max_coordinators_per_site`: Configure how many coordinators a site can have
- `max_nodes_per_coordinator`: Configure how many nodes each coordinator supports
- System automatically adapts to configured limits
- No hard-coded restrictions

## Integration Points

### Frontend → Backend
- REST API calls for CRUD operations
- Password validation before access
- Bulk and individual section updates

### Backend → Devices (via MQTT)
- Coordinator configuration updates published to: `site/{siteId}/coord/{coordId}/config`
- Node configuration updates published to: `site/{siteId}/node/{nodeId}/config`
- Devices listen and apply changes automatically

### Backend → Frontend (via WebSocket)
- Real-time settings updates pushed to all connected clients
- UI auto-updates when changes occur
- Multi-user synchronization

## Security

1. **Password Protection**: Settings require authentication
2. **Sensitive Data**: WiFi passwords are masked, API keys not exposed
3. **Read-Only Fields**: Firmware version and URLs cannot be modified
4. **Validation**: Backend validates all setting changes
5. **Rate Limiting**: Recommended on backend endpoints

## Testing

### Manual Testing Checklist
- [ ] Password authentication works
- [ ] All 9 sections load correctly
- [ ] Individual section save works
- [ ] Bulk "Save All" works
- [ ] Reset to defaults restores values
- [ ] Real-time updates appear in UI
- [ ] Developer mode toggle works
- [ ] Zone add/remove/edit works
- [ ] Sliders update values correctly
- [ ] Toggles switch states
- [ ] Read-only fields cannot be edited
- [ ] Lock button secures settings again
- [ ] Responsive design works on mobile
- [ ] Section collapse/expand works

### Integration Testing
- [ ] Backend endpoints respond correctly
- [ ] MongoDB persists settings
- [ ] MQTT messages published to devices
- [ ] WebSocket broadcasts to all clients
- [ ] Devices apply configuration changes
- [ ] Multi-client synchronization works

## Next Steps

### Backend Implementation
1. Create `SystemSettingsController` in Go backend
2. Define MongoDB model for `system_settings` collection
3. Implement password validation middleware
4. Add MQTT publishers for device configuration
5. Integrate WebSocket broadcaster for real-time updates
6. Write unit tests for all endpoints
7. Add integration tests for full flow

### Frontend Enhancements (Optional)
1. Add setting change history/audit log
2. Implement setting import/export (JSON)
3. Add preset configurations (e.g., "Energy Saver", "Performance")
4. Show which coordinator/node will be affected by changes
5. Add tooltips explaining each setting
6. Implement search/filter for settings

### Documentation
1. Create user manual for Settings tab
2. Write administrator guide
3. Document MQTT topic schema
4. Add API documentation to Swagger/OpenAPI

## Troubleshooting

### Password Not Working
- Verify backend endpoint `/api/v1/system/settings/validate-password` is implemented
- Check backend logs for authentication errors
- Ensure password is "1234" (default)

### Settings Not Saving
- Check browser console for API errors
- Verify MongoDB connection in backend
- Confirm backend receives PUT request
- Check backend logs for validation errors

### Real-Time Updates Not Working
- Verify WebSocket connection is established
- Check MQTT broker is running
- Ensure backend publishes to `system/settings/update` topic
- Check frontend subscriptions in MqttService

### Sections Not Expanding
- Check browser console for JavaScript errors
- Verify `isSectionExpanded()` function works
- Ensure SCSS styles are loaded
- Try refreshing the page

## Support

For issues or questions about the Settings tab implementation:
1. Check this documentation first
2. Review SETTINGS_TAB_BACKEND_REQUIREMENTS.md for backend details
3. Check browser console for errors
4. Review backend logs
5. Verify MQTT broker status

## Version History

- **v1.0** - Initial implementation
  - Password protection
  - 9 configuration sections
  - Real-time synchronization
  - Dynamic scaling support
  - Developer mode
  - Zone management
  - Responsive design
