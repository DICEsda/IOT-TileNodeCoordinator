# MQTT API Documentation

## Topic Structure

Base topic: `smart_tile/`

### State Topics (Coordinator → Frontend)
- `smart_tile/nodes/{node_id}/state`      - Current node state
- `smart_tile/nodes/{node_id}/temp`       - Temperature updates
- `smart_tile/nodes/{node_id}/health`     - Node health status
- `smart_tile/zones/{zone_id}/presence`   - Zone presence state
- `smart_tile/system/status`              - Overall system status

### Command Topics (Frontend → Coordinator)
- `smart_tile/nodes/{node_id}/cmd`        - Node commands
- `smart_tile/zones/{zone_id}/cmd`        - Zone commands
- `smart_tile/system/cmd`                 - System-wide commands

### Configuration Topics
- `smart_tile/nodes/{node_id}/config`     - Node configuration
- `smart_tile/zones/{zone_id}/config`     - Zone configuration
- `smart_tile/system/config`              - System configuration

### Discovery Topics
- `smart_tile/discovery/nodes`            - Available nodes
- `smart_tile/discovery/zones`            - Zone definitions

## Message Formats

### Node State
```json
{
  "node_id": "string",
  "light_id": "string",
  "brightness": number,
  "temperature": number,
  "deration_level": number,
  "status": "online|offline|error",
  "last_seen": "timestamp",
  "error": "string|null"
}
```

### Zone State
```json
{
  "zone_id": "string",
  "presence": boolean,
  "lights": [
    {
      "light_id": "string",
      "brightness": number
    }
  ],
  "last_event": "timestamp"
}
```

### Command Format
```json
{
  "cmd": "set_light|set_zone|pair|unpair",
  "params": {
    // Command specific parameters
  },
  "id": "unique_command_id"
}
```

### Configuration Format
```json
{
  "thermal": {
    "derate_start": number,
    "derate_max": number,
    "min_brightness": number
  },
  "timing": {
    "presence_timeout": number,
    "fade_duration": number
  },
  "power": {
    "report_interval": number,
    "deep_sleep": boolean
  }
}
```
