# Debug Dashboard - Quick Summary

## ✅ What Was Created

A comprehensive **System Debug Dashboard** component for the IoT Tile System frontend that provides real-time monitoring and diagnostics capabilities.

## 📁 Files Created

1. **`IOT-Frontend-main/src/app/features/debug/debug.component.ts`** (650+ lines)
   - Angular standalone component with full debugging capabilities
   - TypeScript + inline template + inline styles
   - Signal-based state management
   - Responsive, professional UI

2. **`IOT-Frontend-main/docs/DEBUG_DASHBOARD.md`** (350+ lines)
   - Complete documentation
   - Usage guide
   - Troubleshooting section
   - Example scenarios

## 🎯 Key Features

### 1. Connection Status Monitor 🔌
- **HTTP API**: Tests REST endpoint connectivity with latency measurement
- **WebSocket**: Real-time event stream status
- **MQTT**: Message broker connection status
- Auto-refreshes every 5 seconds
- Color-coded status cards (green = connected, red = disconnected, orange = error)

### 2. Environment Configuration Display ⚙️
- Shows all configured URLs (API, WebSocket, MQTT)
- Useful for verifying environment settings
- Helps diagnose URL/endpoint issues

### 3. Backend Health Check 🏥
- Fetches `/api/v1/health` endpoint
- Shows backend status and uptime
- Manual retry button
- Clear healthy/unhealthy indicators

### 4. Live Telemetry Monitor 📡
**The most powerful feature for debugging**:
- Start/Stop controls for monitoring
- Captures all MQTT and WebSocket messages in real-time
- Automatically subscribes to:
  - `site/site001/node/+/telemetry` (all node data)
  - `site/site001/coord/+/telemetry` (all coordinator data)
- Shows timestamp, source (MQTT/WebSocket), topic, and full JSON payload
- Color-coded by source (blue = MQTT, green = WebSocket)
- Keeps last 50 messages
- Clear button to reset log

### 5. MQTT Topic Test 🧪
**Custom topic subscription testing**:
- Input field for any topic pattern
- Supports wildcards (`+`, `#`)
- Subscribe/Unsubscribe controls
- Live message log
- Perfect for testing topic alignment

Example test topics:
```
site/site001/coord/+/telemetry
site/site001/node/+/telemetry
site/site001/coord/+/mmwave
site/#
```

## 🚀 How to Use

### 1. Add Route
In your `app.routes.ts`:
```typescript
{
  path: 'debug',
  loadComponent: () => 
    import('./features/debug/debug.component').then(m => m.DebugComponent)
}
```

### 2. Navigate
Open your browser to: `http://localhost:4200/debug`

### 3. Monitor
- Connection statuses update automatically every 5 seconds
- Click "Start Monitor" to begin capturing telemetry
- Use "Refresh All" to manually update all statuses

### 4. Test MQTT Topics
1. Enter a topic pattern in the MQTT Test section
2. Click "Subscribe"
3. Watch for messages in the test log
4. Click "Unsubscribe" when done

## 🔍 Troubleshooting Use Cases

### Use Case 1: Verify System is Connected
✅ All connection cards should show green "CONNECTED" status

### Use Case 2: Debug Missing Telemetry
1. Start monitoring
2. Trigger an event (e.g., button press on node)
3. Watch for messages appearing in real-time
4. If no messages appear:
   - Check connection statuses
   - Verify firmware is publishing
   - Test specific topics manually

### Use Case 3: Verify MQTT Topic Alignment
1. Use MQTT Topic Test
2. Subscribe to: `site/site001/coord/+/telemetry`
3. Power on coordinator
4. Verify messages arrive with correct topic format
5. Check payload structure matches expected JSON

### Use Case 4: Monitor Backend Health
- Health card shows backend status
- If unhealthy:
  - Check backend logs
  - Verify MongoDB connection
  - Verify MQTT broker connection
  - Click "Retry" after fixing

## 📊 Visual Features

- **Responsive grid layouts** - Works on mobile, tablet, desktop
- **Status color coding** - Green (good), Red (bad), Orange (warning)
- **Card-based UI** - Clean, professional sections
- **Scrollable logs** - Won't overflow page
- **Real-time updates** - No manual refresh needed
- **Monospace font** - For URLs, topics, and JSON
- **Emojis** - Visual indicators for quick scanning

## 🎨 UI Preview

```
🔧 System Debug Dashboard                  [🔄 Refresh All] [🗑️ Clear Logs]

⚙️ Environment Configuration
┌─────────────────────────────────────┐
│ API URL: http://localhost:8080/api  │
│ WebSocket URL: ws://localhost:8080  │
│ MQTT WebSocket: ws://localhost:9001 │
└─────────────────────────────────────┘

🔌 Connection Status
┌──────────────┐ ┌──────────────┐ ┌──────────────┐
│ ✅ HTTP API  │ │ ✅ WebSocket │ │ ✅ MQTT      │
│ CONNECTED    │ │ CONNECTED    │ │ CONNECTED    │
│ Latency: 45ms│ │ Last: 10:30  │ │ Last: 10:30  │
└──────────────┘ └──────────────┘ └──────────────┘

🏥 Backend Health
┌────────────────────────┐
│ Status: ✅ Healthy     │
│ Uptime: 2h 34m 12s     │
└────────────────────────┘

📡 Live Telemetry Monitor
[▶️ Start Monitor] [Stop] [Clear]
┌─────────────────────────────────────┐
│ 10:30:45  MQTT  site/site001/node/  │
│ { "ts": 1699459845, "node_id": ... }│
├─────────────────────────────────────┤
│ 10:30:44  WebSocket  node_update    │
│ { "type": "telemetry", "data": ... }│
└─────────────────────────────────────┘

🧪 MQTT Topic Test
Topic: [site/site001/coord/+/telemetry]
[Subscribe] [Unsubscribe]
┌─────────────────────────────────────┐
│ Subscribing to: site/site001/...    │
│ ✅ Received: {"ts":1699459845...}   │
└─────────────────────────────────────┘
```

## 🔗 Integration Points

The debug component integrates with all core services:

```typescript
EnvironmentService  → Configuration display
ApiService          → HTTP connection test + health check
WebSocketService    → WebSocket connection status + message stream
MqttService         → MQTT connection status + telemetry subscriptions
```

## ✨ Benefits

1. **Instant Visibility** - See all system connections at a glance
2. **Real-Time Debugging** - Watch telemetry flow in real-time
3. **Topic Validation** - Test MQTT topics match PRD specification
4. **Troubleshooting Aid** - Quickly identify connectivity issues
5. **Development Tool** - Essential for integration testing
6. **Production Monitoring** - Can be used in production for ops team

## 📝 Next Steps

1. **Install Dependencies** (if not already):
   ```bash
   cd IOT-Frontend-main/IOT-Frontend-main
   npm install
   ```

2. **Add Route** to `app.routes.ts`

3. **Run Frontend**:
   ```bash
   ng serve
   ```

4. **Navigate** to `http://localhost:4200/debug`

5. **Start Backend** to see connections working

6. **Flash Firmware** and watch telemetry flow!

## 🎓 Documentation

Full documentation available in:
- **`IOT-Frontend-main/docs/DEBUG_DASHBOARD.md`** - Complete guide
- **Component itself** - Inline comments and clear code structure

---

**Status**: ✅ **Ready to Use**  
**Version**: 1.0  
**Created**: November 8, 2025

The debug dashboard is a powerful tool for monitoring and diagnosing the entire IoT Tile System. Use it to verify end-to-end connectivity, monitor live telemetry, and troubleshoot any issues during development and deployment.
