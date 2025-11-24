# Frontend Connectivity Fix

## Problem
When accessing the frontend at `http://localhost:4200`, the API and MQTT connections showed as disconnected (×) while only WebSocket was connected (✓).

## Root Cause
The frontend was configured to connect directly to `http://localhost:8000` for the API and `ws://localhost:8000/mqtt` for MQTT WebSocket. However, when running in Docker:

1. **Browser-side connections** must go through the host machine's `localhost:4200` (nginx proxy)
2. The nginx configuration was missing the `/mqtt` proxy route
3. The backend didn't expose a `/mqtt` endpoint (only `/ws`)

## Changes Made

### 1. Frontend Environment Service
**File**: `IOT-Frontend-main/IOT-Frontend-main/src/app/core/services/environment.service.ts`

Changed default URLs to use nginx proxy:
```typescript
// Before:
apiUrl: 'http://localhost:8000'
wsUrl: 'ws://localhost:8000/ws'
mqttWsUrl: 'ws://localhost:8000/mqtt'

// After:
apiUrl: 'http://localhost:4200/api'
wsUrl: 'ws://localhost:4200/ws'
mqttWsUrl: 'ws://localhost:4200/mqtt'
```

### 2. Nginx Configuration
**File**: `IOT-Frontend-main/IOT-Frontend-main/nginx.conf`

Added MQTT WebSocket proxy:
```nginx
# MQTT WebSocket support
location /mqtt {
    proxy_pass http://backend:8000/mqtt;
    proxy_http_version 1.1;
    proxy_set_header Upgrade $http_upgrade;
    proxy_set_header Connection "upgrade";
    proxy_set_header Host $host;
    proxy_set_header X-Real-IP $remote_addr;
    proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
    proxy_read_timeout 86400;
}
```

### 3. Backend HTTP Handlers
**File**: `IOT-Backend-main/IOT-Backend-main/internal/http/handlers.go`

Added `/mqtt` endpoint as alias to existing WebSocket handler:
```go
// WebSocket endpoints
router.HandleFunc("/ws", h.websocket)
router.HandleFunc("/mqtt", h.websocket) // MQTT WebSocket bridge (same as /ws)
```

## How It Works Now

### Request Flow
```
Browser → http://localhost:4200/api/health
       → nginx proxy → http://backend:8000/health
       → Go backend

Browser → ws://localhost:4200/ws
       → nginx proxy → ws://backend:8000/ws
       → Go WebSocket handler

Browser → ws://localhost:4200/mqtt
       → nginx proxy → ws://backend:8000/mqtt
       → Go WebSocket handler (MQTT bridge)
```

### Docker Network
- Frontend container: Runs nginx on port 80, exposed as `localhost:4200`
- Backend container: Runs Go API on port 8000, accessible as `backend:8000` within Docker network
- Browser connects to `localhost:4200` only
- Nginx proxies requests to internal `backend:8000`

## Testing

After rebuilding containers:
```bash
docker-compose build backend frontend
docker-compose up -d backend frontend
```

Open `http://localhost:4200` and verify all three connection indicators are green:
- ✓ API (http://localhost:4200/api)
- ✓ MQTT (ws://localhost:4200/mqtt)
- ✓ WebSocket (ws://localhost:4200/ws)

## Notes

- The backend's `/ws` and `/mqtt` endpoints are functionally identical
- Both support MQTT operations via message types: `subscribe`, `unsubscribe`, `publish`
- The separation exists for frontend semantic clarity (general WebSocket vs MQTT-specific)
- All browser requests go through nginx to avoid CORS issues
- Environment variables can override defaults via `window.env` object
