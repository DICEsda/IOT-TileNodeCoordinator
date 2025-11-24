# Automatic MQTT Broker Discovery

## Overview
The coordinator automatically discovers the MQTT broker on your network, eliminating the need for manual IP configuration when your phone's hotspot IP changes.

## How It Works

### 1. **Priority Discovery Order**
1. **Gateway First** (Phone/Hotspot IP)
   - When connected to a mobile hotspot, the gateway is typically the phone running Docker
   - Fast check (250ms timeout)
   
2. **Subnet Scan** (Nearby IPs)
   - Scans up to 50 nearby hosts on the local subnet
   - Prioritizes IPs closest to the coordinator's IP

### 2. **Automatic Rediscovery Triggers**

The coordinator will automatically rediscover when:

- **Initial boot** with no saved configuration
- **Network change detected** (different subnet)
- **6 consecutive connection failures** (30 seconds)
- **Manual trigger** via serial command: `erase mqtt` + `reboot`

### 3. **Smart Network Detection**

Before attempting connection, checks if:
- Stored broker IP is on the current subnet
- If not → triggers immediate rediscovery
- Prevents wasted connection attempts to old/stale IPs

## Serial Commands

### View Current MQTT Config
```
status
```

### Force Rediscovery
```
erase mqtt
reboot
```

### Manual Configuration (if needed)
After `erase mqtt` and reboot, you'll be prompted:
```
Configure MQTT broker now? (y/n): y
MQTT broker IP/hostname: <enter IP or leave blank for auto>
```
Leave blank or press Enter to use auto-discovery.

## Typical Scenarios

### Scenario 1: First Boot
1. Coordinator boots → No saved config
2. Auto-discovers gateway (your phone at 10.103.41.195)
3. Connects successfully
4. Saves IP to NVS for faster reconnection

### Scenario 2: Phone IP Changes
1. Phone hotspot restarts → New IP (10.103.42.100)
2. Coordinator tries old IP → Fails
3. Detects different subnet → Triggers rediscovery
4. Finds new gateway IP → Reconnects

### Scenario 3: Switch Networks
1. Move from phone hotspot to home WiFi
2. Coordinator connects to new network
3. Old broker IP detected as wrong subnet
4. Auto-discovers broker on new network

## Configuration Storage

- Broker IP stored in NVS partition under `"mqtt"` namespace
- Persists across reboots for fast reconnection
- Automatically invalidated on network change

## Debug Output

Watch for these log messages:

```
INFO  | Trying gateway 10.103.41.195 as MQTT broker...
INFO  | MQTT broker found at gateway: 10.103.41.195
INFO  | MQTT connected!
```

Or if discovery needed:
```
WARN  | Broker 10.140.61.195 not on current subnet - triggering rediscovery
INFO  | Multiple MQTT failures - attempting rediscovery
INFO  | Scanning 50 nearby hosts for MQTT...
INFO  | Auto-discovered MQTT broker at 10.103.41.195
```

## Performance

- **Gateway check**: ~250ms
- **Subnet scan (50 hosts)**: ~12 seconds
- **Total worst-case**: ~13 seconds to discover
- **Cached reconnect**: ~1 second

## Requirements

Your Docker MQTT broker must:
1. Bind to `0.0.0.0:1883` (all interfaces)
2. Be accessible from the WiFi network
3. Accept connections on port 1883

Check `docker-compose.yml`:
```yaml
mosquitto:
  ports:
    - "1883:1883"  # ✓ Correct
```

## Troubleshooting

### Discovery Always Fails
- Check Windows Firewall allows inbound TCP 1883
- Verify Docker Desktop networking mode
- Ensure phone hotspot allows peer-to-peer connections

### Slow Discovery
- Reduce scan range in code (already optimized to 50 hosts)
- Check for network congestion

### Wrong Broker Found
- Multiple MQTT brokers on network
- Use manual configuration to specify exact IP
