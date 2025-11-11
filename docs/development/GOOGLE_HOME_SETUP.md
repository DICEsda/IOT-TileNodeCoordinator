# Google Home API Integration Guide

## Overview

This guide explains how to integrate your IOT Smart Tile system with Google Home, enabling voice control and automation through Google Assistant.

## Features

- **Voice Control**: "Hey Google, turn on living room tile"
- **Brightness Control**: "Hey Google, set tile brightness to 50%"
- **Color Control**: "Hey Google, change tile to red"
- **Room Grouping**: Group tiles with other smart devices
- **Routines**: Include tiles in Google Home routines
- **Multi-User**: Family members can control tiles
- **Remote Access**: Control from anywhere via Google Home app

## Prerequisites

1. **Google Cloud Platform Account**
   - Create account at https://console.cloud.google.com/
   - Billing must be enabled (free tier available)

2. **Domain Name** (for production)
   - Required for OAuth and webhooks
   - Can use ngrok for testing

3. **Backend Deployment**
   - Backend must be accessible via HTTPS
   - Valid SSL certificate required

## Setup Steps

### 1. Create Google Cloud Project

```bash
# Go to: https://console.cloud.google.com/
# Click "New Project"
# Project Name: "IOT-SmartTile"
# Click "Create"
```

### 2. Enable Required APIs

```bash
# In Google Cloud Console, enable:
# 1. HomeGraph API
# 2. Actions API
# 3. Cloud Functions API (optional)

# Or via gcloud CLI:
gcloud services enable homegraph.googleapis.com
gcloud services enable actions.googleapis.com
```

### 3. Create OAuth 2.0 Credentials

1. Go to **APIs & Services > Credentials**
2. Click **Create Credentials > OAuth 2.0 Client ID**
3. Configure OAuth consent screen:
   - User Type: External
   - App name: "Smart Tile"
   - User support email: your-email@example.com
   - Developer contact: your-email@example.com
4. Add scopes:
   - `https://www.googleapis.com/auth/homegraph`
5. Create OAuth Client ID:
   - Application type: Web application
   - Name: "Smart Tile OAuth"
   - Authorized redirect URIs:
     ```
     https://yourdomain.com/oauth/google/callback
     https://oauth-redirect.googleusercontent.com/r/YOUR_PROJECT_ID
     ```
6. Save Client ID and Client Secret

### 4. Create Service Account

```bash
# Create service account
gcloud iam service-accounts create iot-smarttile \
    --display-name="IOT Smart Tile Service Account" \
    --project=YOUR_PROJECT_ID

# Grant HomeGraph permission
gcloud projects add-iam-policy-binding YOUR_PROJECT_ID \
    --member="serviceAccount:iot-smarttile@YOUR_PROJECT_ID.iam.gserviceaccount.com" \
    --role="roles/homegraph.admin"

# Download service account key
gcloud iam service-accounts keys create google-service-account.json \
    --iam-account=iot-smarttile@YOUR_PROJECT_ID.iam.gserviceaccount.com
```

### 5. Create Smart Home Action

1. Go to **Actions Console**: https://console.actions.google.com/
2. Click **New Project**
3. Select your existing GCP project
4. Choose **Smart Home**
5. Configure:
   - Name: "Smart Tile"
   - Pronunciation: "Smart Tile"
6. Set up Account Linking:
   - Authorization URL: `https://yourdomain.com/oauth/google/authorize`
   - Token URL: `https://yourdomain.com/oauth/google/token`
   - Client ID: (from OAuth credentials)
   - Client Secret: (from OAuth credentials)
   - Scopes: `https://www.googleapis.com/auth/homegraph`
7. Set fulfillment URL:
   ```
   https://yourdomain.com/api/v1/google/home/fulfillment
   ```

### 6. Configure Backend

Update your `.env` file:

```bash
# Enable Google Home integration
GOOGLE_HOME_ENABLED=true

# Google Cloud Project
GOOGLE_HOME_PROJECT_ID=your-project-id
GOOGLE_HOME_API_KEY=your-api-key

# OAuth Credentials
GOOGLE_HOME_CLIENT_ID=your-client-id.apps.googleusercontent.com
GOOGLE_HOME_CLIENT_SECRET=your-client-secret

# OAuth URLs
GOOGLE_OAUTH_REDIRECT_URI=https://yourdomain.com/oauth/google/callback
GOOGLE_OAUTH_TOKEN_URL=https://oauth2.googleapis.com/token
GOOGLE_OAUTH_AUTH_URL=https://accounts.google.com/o/oauth2/v2/auth
GOOGLE_OAUTH_SCOPES=https://www.googleapis.com/auth/homegraph

# HomeGraph API
GOOGLE_HOMEGRAPH_API_URL=https://homegraph.googleapis.com/v1
GOOGLE_SERVICE_ACCOUNT_KEY=/app/config/google-service-account.json

# Device Configuration
GOOGLE_HOME_DEVICE_MODEL=smart-tile-v1
GOOGLE_HOME_DEVICE_TYPE=action.devices.types.LIGHT
GOOGLE_TRAITS=action.devices.traits.OnOff,action.devices.traits.Brightness,action.devices.traits.ColorSetting
```

### 7. Deploy Service Account Key

```bash
# Copy service account key to backend config folder
cp google-service-account.json IOT-Backend-main/IOT-Backend-main/internal/config/

# Update docker-compose.yml to mount the key
# Add to backend service volumes:
volumes:
  - ./IOT-Backend-main/IOT-Backend-main/internal/config/google-service-account.json:/app/config/google-service-account.json:ro
```

### 8. Rebuild and Deploy

```bash
# Rebuild backend with Google Home support
docker-compose down
docker-compose build backend
docker-compose up -d

# Verify backend is running
curl http://localhost:8000/health
```

## Testing Integration

### 1. Test with Google Home App

1. Open **Google Home app** on your phone
2. Tap **+** (Add)
3. Select **Set up device**
4. Choose **Works with Google**
5. Search for "Smart Tile"
6. Sign in with your credentials
7. Authorize the app
8. Your tiles should appear in the device list

### 2. Test Voice Commands

```
"Hey Google, sync my devices"
"Hey Google, turn on living room tile"
"Hey Google, set tile brightness to 75%"
"Hey Google, change tile color to blue"
"Hey Google, turn off all tiles"
```

### 3. Test via API

```bash
# Test SYNC request
curl -X POST http://localhost:8000/api/v1/google/home/fulfillment \
  -H "Content-Type: application/json" \
  -d '{
    "requestId": "test-request-1",
    "inputs": [{
      "intent": "action.devices.SYNC"
    }]
  }'

# Test QUERY request
curl -X POST http://localhost:8000/api/v1/google/home/fulfillment \
  -H "Content-Type: application/json" \
  -d '{
    "requestId": "test-request-2",
    "inputs": [{
      "intent": "action.devices.QUERY",
      "payload": {
        "devices": [{"id": "node-001"}]
      }
    }]
  }'

# Test EXECUTE request (turn on)
curl -X POST http://localhost:8000/api/v1/google/home/fulfillment \
  -H "Content-Type: application/json" \
  -d '{
    "requestId": "test-request-3",
    "inputs": [{
      "intent": "action.devices.EXECUTE",
      "payload": {
        "commands": [{
          "devices": [{"id": "node-001"}],
          "execution": [{
            "command": "action.devices.commands.OnOff",
            "params": {"on": true}
          }]
        }]
      }
    }]
  }'
```

## Supported Device Traits

### 1. OnOff
```javascript
// Commands
"action.devices.commands.OnOff"
// Parameters
{ "on": true | false }

// Voice commands
"Turn on the tile"
"Turn off bedroom tile"
```

### 2. Brightness
```javascript
// Commands
"action.devices.commands.BrightnessAbsolute"
// Parameters
{ "brightness": 0-100 }

// Voice commands
"Set tile brightness to 50%"
"Dim the tile"
"Brighten living room tile"
```

### 3. ColorSetting
```javascript
// Commands
"action.devices.commands.ColorAbsolute"
// Parameters
{ 
  "color": {
    "spectrumRGB": 16711680  // Red in decimal
  }
}

// Voice commands
"Set tile to red"
"Change tile color to blue"
"Make the tile green"
```

## Integration with Node Control

The Google Home handlers will automatically send commands to your nodes via MQTT:

```go
// When Google sends an OnOff command:
// Backend publishes to: site/{siteId}/coord/{coordId}/cmd
{
  "action": "set_light",
  "node_id": "node-001",
  "light_id": "L01",
  "r": 0,
  "g": 0,
  "b": 0,
  "w": 255,
  "on": true
}
```

## State Reporting

The backend automatically reports state changes to Google:

```go
// When node state changes (via MQTT telemetry):
// Backend calls: ReportState()
{
  "requestId": "state-report-123",
  "agentUserId": "user-001",
  "payload": {
    "devices": {
      "states": {
        "node-001": {
          "online": true,
          "on": true,
          "brightness": 80,
          "color": {
            "spectrumRgb": 16711680
          }
        }
      }
    }
  }
}
```

## Troubleshooting

### Devices Not Appearing

1. **Check OAuth Configuration**
   ```bash
   # Verify credentials in .env
   echo $GOOGLE_HOME_CLIENT_ID
   echo $GOOGLE_HOME_CLIENT_SECRET
   ```

2. **Verify Backend Logs**
   ```bash
   docker-compose logs -f backend | grep "google"
   ```

3. **Test Fulfillment Endpoint**
   ```bash
   curl https://yourdomain.com/api/v1/google/home/fulfillment
   ```

4. **Request Manual Sync**
   ```bash
   curl -X POST http://localhost:8000/api/v1/google/home/request-sync \
     -H "Content-Type: application/json" \
     -d '{"user_id": "user-001"}'
   ```

### Voice Commands Not Working

1. **Sync Devices**
   ```
   "Hey Google, sync my devices"
   ```

2. **Check Device Names**
   - Device names must be unique
   - Avoid special characters
   - Use simple, pronounceable names

3. **Verify Traits**
   ```bash
   # Check backend supports required traits
   grep GOOGLE_TRAITS .env
   ```

### State Not Updating

1. **Check MQTT Connection**
   ```bash
   docker-compose logs -f backend | grep "mqtt"
   ```

2. **Verify State Reporting**
   ```bash
   # Enable debug logging
   GOOGLE_HOME_ENABLED=true
   # Check logs for "Reporting state to Google"
   ```

3. **Test Manual State Report**
   ```bash
   curl -X POST http://localhost:8000/api/v1/google/home/report-state \
     -H "Content-Type: application/json" \
     -d '{
       "user_id": "user-001",
       "device_id": "node-001",
       "state": {
         "online": true,
         "on": true,
         "brightness": 80
       }
     }'
   ```

## Security Best Practices

1. **Use HTTPS Only**
   - Never use HTTP in production
   - Use valid SSL certificates (Let's Encrypt)

2. **Secure OAuth Tokens**
   - Store tokens encrypted
   - Implement token rotation
   - Use short token lifetimes

3. **Validate Requests**
   - Verify request signatures
   - Implement rate limiting
   - Log all OAuth attempts

4. **Service Account Key**
   - Never commit to git
   - Rotate keys regularly
   - Restrict file permissions

## Production Deployment

### Using ngrok (Testing)

```bash
# Install ngrok
# Download from: https://ngrok.com/

# Start ngrok tunnel
ngrok http 8000

# Update .env with ngrok URL
GOOGLE_OAUTH_REDIRECT_URI=https://abc123.ngrok.io/oauth/google/callback
```

### Using Custom Domain

```bash
# Configure nginx reverse proxy
server {
    listen 443 ssl;
    server_name api.yourdomain.com;
    
    ssl_certificate /etc/letsencrypt/live/yourdomain.com/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/yourdomain.com/privkey.pem;
    
    location / {
        proxy_pass http://localhost:8000;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

## Cost Considerations

- **HomeGraph API**: Free tier includes 1M requests/month
- **Actions Console**: Free
- **OAuth**: No additional cost
- **Service Account**: Free
- **Cloud Functions** (if used): Pay-per-use

## Next Steps

1. ✅ Configure Google Cloud Project
2. ✅ Set up OAuth credentials
3. ✅ Create Smart Home Action
4. ✅ Deploy backend with Google Home support
5. ⏳ Test with Google Home app
6. ⏳ Add more device traits
7. ⏳ Implement multi-user support
8. ⏳ Add custom device names per user

## Support

For issues:
- Check backend logs: `docker-compose logs backend`
- Review Google Cloud logs: https://console.cloud.google.com/logs
- Test API endpoints manually
- Contact: support@yourdomain.com

---

**Version:** 1.0.0  
**Last Updated:** 2025-11-05
