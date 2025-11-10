# Debug Dashboard Component

## Overview
The Debug Dashboard is a comprehensive monitoring and diagnostics tool for the IoT Tile System. It provides real-time visibility into all system connections, telemetry flows, and backend health status.

## Location
`IOT-Frontend-main/src/app/features/debug/debug.component.ts`

## Features

### 1. **Environment Configuration Display** ⚙️
- Shows current API URL
- Shows WebSocket URL
- Shows MQTT broker URL (WebSocket)
- Useful for verifying configuration across different environments

### 2. **Connection Status Monitoring** 🔌
Real-time monitoring of all system connections:
- **HTTP API**: Connection to backend REST API with latency measurement
- **WebSocket**: Real-time event stream connection status
- **MQTT**: Message broker connection for telemetry

Each connection shows:
- ✅ Connected / ❌ Disconnected / ⚠️ Error status
- Last update timestamp
- Latency (for HTTP API)
- Connection details/URL

### 3. **Backend Health Check** 🏥
- Fetches `/api/v1/health` endpoint
- Shows backend status (healthy/unhealthy)
- Displays uptime information
- Manual retry button if health check fails

### 4. **Live Telemetry Monitor** 📡
Real-time monitoring of all telemetry messages:
- Start/Stop monitoring controls
- Monitors both MQTT and WebSocket messages
- Color-coded by source:
  - 🔵 Blue border = MQTT messages
  - 🟢 Green border = WebSocket messages
- Shows timestamp, topic, and full JSON payload
- Automatically subscribes to:
  - `site/site001/node/+/telemetry` (all node telemetry)
  - `site/site001/coord/+/telemetry` (all coordinator telemetry)
- Keeps last 50 messages (auto-clears older ones)
- Clear button to manually reset log

### 5. **MQTT Topic Test** 🧪
Manual MQTT subscription testing:
- Input field for custom topic patterns
- Support for wildcards (`+`, `#`)
- Subscribe/Unsubscribe controls
- Live message log showing received data
- Useful for debugging topic mismatches

Example topics to test:
```
site/site001/coord/+/telemetry
site/site001/node/+/telemetry
site/site001/coord/+/mmwave
site/#
```

## How to Use

### Access the Dashboard
1. Navigate to `/debug` route in the frontend
2. All monitoring starts automatically on page load

### Monitor Connections
- Connection statuses refresh every 5 seconds automatically
- Green cards = Connected
- Red cards = Disconnected/Error
- Check "Last Update" to see freshness of data

### Monitor Telemetry
1. Click "▶️ Start Monitor" to begin capturing telemetry
2. Watch messages appear in real-time
3. Click on any message to see full JSON payload
4. Click "Stop" to pause monitoring
5. Click "Clear" to reset the message log

### Test Custom MQTT Topics
1. Enter a topic pattern in the input field
2. Click "Subscribe" to start listening
3. Watch the test log for incoming messages
4. Click "Unsubscribe" when done

### Check Backend Health
- Health status is checked on page load
- Click "Retry" button if health check fails
- Green card = Healthy backend
- Red card = Backend issue

## Troubleshooting with Debug Dashboard

### Problem: "HTTP API" shows Disconnected ❌
- **Cause**: Backend server is not running or wrong API URL
- **Solution**: 
  1. Check environment configuration section for API URL
  2. Verify backend is running: `docker ps | grep iot-backend`
  3. Test manually: `curl http://localhost:8080/api/v1/health`

### Problem: "WebSocket" shows Disconnected ❌
- **Cause**: WebSocket endpoint not available or firewall blocking
- **Solution**:
  1. Check WebSocket URL in configuration
  2. Verify backend WebSocket endpoint is running
  3. Check browser console for WebSocket errors

### Problem: "MQTT" shows Disconnected ❌
- **Cause**: MQTT broker not running or WebSocket proxy not configured
- **Solution**:
  1. Verify MQTT broker is running: `docker ps | grep mosquitto`
  2. Check MQTT WebSocket URL (default: `ws://localhost:8080/mqtt`)
  3. Verify backend MQTT proxy is configured

### Problem: No Telemetry Messages Appearing
- **Cause**: Nodes/coordinator not publishing or topic mismatch
- **Solution**:
  1. Click "Start Monitor" first
  2. Verify firmware is connected to MQTT broker (check coordinator serial logs)
  3. Use MQTT Topic Test to subscribe to specific topics manually
  4. Check backend MQTT handlers are receiving messages

### Problem: Backend Health shows Unhealthy ❌
- **Cause**: Backend unable to connect to MongoDB or MQTT broker
- **Solution**:
  1. Check backend logs: `docker logs iot-backend`
  2. Verify MongoDB is running
  3. Verify MQTT broker is accessible
  4. Click "Retry" after fixing issues

## Technical Details

### Auto-Refresh
- Connection statuses refresh every 5 seconds
- Uses RxJS `interval()` with `takeUntil()` for cleanup

### Signal-Based State
All state is managed with Angular signals:
- `connectionStatuses()` - Array of connection info
- `healthStatus()` - Backend health data
- `telemetryMessages()` - Array of captured messages
- `mqttTestLog()` - Test subscription logs
- `monitoring()` - Boolean monitoring state

### Memory Management
- Telemetry messages limited to last 50 entries
- Proper RxJS subscription cleanup on component destroy
- Signal updates are efficient and reactive

## Integration with Services

### Environment Service
```typescript
this.environmentService.apiUrl
this.environmentService.wsUrl
this.environmentService.mqttWsUrl
```

### API Service
```typescript
this.apiService.getSites()      // Connection test
this.apiService.getHealth()     // Health check
```

### WebSocket Service
```typescript
this.wsService.connected()      // Connection status
this.wsService.messages$        // Message stream
```

### MQTT Service
```typescript
this.mqttService.connected()                    // Connection status
this.mqttService.subscribeNodeTelemetry()       // Node data
this.mqttService.subscribeCoordinatorTelemetry() // Coordinator data
this.mqttService.subscribe(topic)               // Custom topics
```

## Routing Setup

Add to your `app.routes.ts`:
```typescript
{
  path: 'debug',
  loadComponent: () => 
    import('./features/debug/debug.component').then(m => m.DebugComponent)
}
```

## Styling

The component includes comprehensive inline styles with:
- Responsive grid layouts
- Status color coding (green/red/orange)
- Card-based sections
- Scrollable logs with max-height
- Mobile-friendly breakpoints
- Professional button styling

## Future Enhancements

Potential additions:
- [ ] Export telemetry log to JSON file
- [ ] Graphical latency charts over time
- [ ] WebSocket message filtering by type
- [ ] MQTT topic subscription management (bulk subscribe/unsubscribe)
- [ ] System performance metrics (memory, CPU usage if available)
- [ ] Historical telemetry playback
- [ ] Alert notifications for disconnections

## Example Usage Scenarios

### Scenario 1: Verifying End-to-End Telemetry Flow
1. Open Debug Dashboard
2. Ensure all connections show ✅ Connected
3. Click "Start Monitor"
4. Power on a node
5. Watch for node telemetry messages appearing in real-time
6. Verify topic matches: `site/site001/node/{nodeId}/telemetry`

### Scenario 2: Testing New MQTT Topic Structure
1. Go to MQTT Topic Test section
2. Enter new topic pattern: `site/site001/coord/A4:CF:12:34:56:78/mmwave`
3. Click "Subscribe"
4. Trigger mmWave sensor event
5. Watch test log for incoming messages
6. Verify payload structure

### Scenario 3: Diagnosing Connection Issues
1. Check Connection Status section
2. Identify which service shows disconnected
3. Check details field for error message
4. Use appropriate troubleshooting steps above
5. Click "Refresh All" after fixing
6. Verify green status

---

**Component Version**: 1.0  
**Last Updated**: November 8, 2025  
**Status**: ✅ Ready for Use
