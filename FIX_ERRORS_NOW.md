# Fix Your Connection Errors NOW

## The Problem
Your frontend can't connect because the **backend is NOT running**.

## The Fix (Choose ONE method)

### Method 1: Start Everything (Easiest) ⭐

**Run this file:**
```
START_ALL_SERVICES.bat
```

This will:
1. Start MongoDB (Docker)
2. Start MQTT Broker (Docker)
3. Start Backend (new window)
4. Start Frontend (new window)

**Wait 30 seconds**, then open: http://localhost:4200

---

### Method 2: Start Backend Only (If MongoDB/MQTT already running)

**Run this file:**
```
START_BACKEND_NOW.bat
```

Then **refresh your browser**.

---

### Method 3: Check What's Running

**Run this file:**
```
CHECK_SERVICES.bat
```

This shows which services are running/not running.

---

## Quick Diagnostic

**Open PowerShell and run:**
```powershell
# Check if backend is accessible
curl http://localhost:8000/health
```

**If you get an error**, the backend is not running.

**Run:**
```
START_BACKEND_NOW.bat
```

---

## Expected Results

### After Starting Backend

**PowerShell:**
```powershell
PS> curl http://localhost:8000/health
```

**Should return:**
```json
{
  "status": "healthy",
  "database": true,
  "mqtt": true
}
```

### In Browser Console (F12)

**Before** (with errors):
```
✗ ERR_SOCKET_NOT_CONNECTED
✗ WebSocket connection failed
✗ Failed to fetch
```

**After** (working):
```
✓ [WebSocket] Connected
✓ [MQTT] Connected
✓ [DataService] Initialized
```

---

## Troubleshooting

### "go: command not found"
**Install Go**: https://go.dev/dl/

### "docker: command not found"
**Install Docker Desktop**: https://www.docker.com/products/docker-desktop

### "Port 8000 already in use"
```bash
# Find and kill the process
netstat -ano | findstr :8000
taskkill /PID <PID_NUMBER> /F
```

### Backend window closes immediately
1. Open `START_BACKEND_NOW.bat`
2. Check the error message
3. Common issues:
   - MongoDB not running → Start with `docker run -d -p 27017:27017 mongo`
   - MQTT not running → Start with `docker run -d -p 1883:1883 eclipse-mosquitto`
   - Go not installed → Install from https://go.dev/dl/

---

## Files Created to Help You

| File | Purpose |
|------|---------|
| **START_ALL_SERVICES.bat** | Start everything at once |
| **START_BACKEND_NOW.bat** | Start just the backend |
| **CHECK_SERVICES.bat** | Check what's running |
| **FIX_ERRORS_NOW.md** | This guide |

---

## Next Steps After Backend Starts

1. **Verify backend**: `curl http://localhost:8000/health`
2. **Refresh browser**: Your frontend should connect
3. **Check console**: No more connection errors!
4. **Use the system**: Navigate to Live Monitor, etc.

---

## Still Having Issues?

### Check Backend Logs
Look at the window that opened when you ran `START_BACKEND_NOW.bat` or `START_ALL_SERVICES.bat`.

**Common log messages:**

✓ **Good**:
```
HTTP server listening on :8000
MQTT connected
Database connected
WebSocket broadcaster started
```

✗ **Bad**:
```
Failed to connect to MongoDB
MQTT connection error
Port already in use
```

### Check Docker Containers
```bash
docker ps
```

Should show `mongodb` and `mosquitto` running.

### Manual Verification
```bash
# Test MongoDB
docker exec -it mongodb mongosh --eval "db.version()"

# Test MQTT
docker exec -it mosquitto mosquitto_sub -t "#" -C 1

# Test Backend
curl http://localhost:8000/health

# Test Frontend
curl http://localhost:4200
```

---

## Summary

**To fix your errors RIGHT NOW:**

1. Double-click: `START_ALL_SERVICES.bat`
2. Wait 30 seconds
3. Open: http://localhost:4200
4. **Errors gone!** ✅

OR if you just need backend:

1. Double-click: `START_BACKEND_NOW.bat`
2. Refresh browser
3. **Errors gone!** ✅

---

**Questions?** Check the detailed guides:
- `QUICK_START.md`
- `docs/FRONTEND_BACKEND_STARTUP.md`

