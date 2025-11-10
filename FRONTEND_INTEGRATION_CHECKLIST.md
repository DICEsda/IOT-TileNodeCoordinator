# Frontend Services Integration Checklist

## ✅ Services Created

- [x] **EnvironmentService** - Configuration management
- [x] **ApiService** - HTTP client with all endpoints
- [x] **WebSocketService** - Real-time bidirectional communication
- [x] **MqttService** - MQTT pub/sub with topic wildcards
- [x] **DataService** - High-level orchestration layer
- [x] **TypeScript Models** - Complete type definitions (20+ interfaces)
- [x] **Service Documentation** - Comprehensive README with examples
- [x] **Example Component** - Working dashboard demonstrating all features
- [x] **Barrel Exports** - Clean import paths
- [x] **App Configuration** - Services provided in app.config.ts

## 📋 Integration Steps

### Step 1: Install Dependencies ⏳
```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm install
```

### Step 2: Configure Environment ⏳
Update `src/environments/environment.ts`:
```typescript
export const environment = {
  production: false,
  apiUrl: 'http://localhost:8080/api/v1',
  wsUrl: 'ws://localhost:8080/ws',
  mqttWsUrl: 'ws://localhost:8080/mqtt'
};
```

### Step 3: Update Dashboard Component ⏳
**File:** `src/app/features/dashboard/dashboard.component.ts`

**Before:**
```typescript
// Mock data or hardcoded values
```

**After:**
```typescript
import { inject } from '@angular/core';
import { DataService } from '../../core/services';

export class DashboardComponent {
  data = inject(DataService);

  ngOnInit() {
    this.data.loadSites();
  }
}
```

### Step 4: Update Devices Component ⏳
**File:** `src/app/features/dashboard/tabs/devices/devices.component.ts`

**Add:**
```typescript
import { inject, effect } from '@angular/core';
import { DataService } from '../../../../core/services';

export class DevicesComponent {
  data = inject(DataService);
  
  constructor() {
    effect(() => {
      const nodes = this.data.nodes();
      // Update UI with real nodes
    });
  }
}
```

### Step 5: Update Light Monitor Component ⏳
**File:** `src/app/features/dashboard/components/light-monitor/light-monitor.component.ts`

**Add:**
```typescript
import { inject, effect } from '@angular/core';
import { DataService } from '../../../../core/services';

export class LightMonitorComponent {
  data = inject(DataService);
  
  constructor() {
    effect(() => {
      const telemetry = this.data.latestTelemetry();
      // Update light displays with real telemetry
    });
  }
}
```

### Step 6: Update Room Visualizer Component ⏳
**File:** `src/app/features/dashboard/components/room-visualizer/room-visualizer.component.ts`

**Add:**
```typescript
import { inject, effect } from '@angular/core';
import { DataService } from '../../../../core/services';

export class RoomVisualizerComponent {
  data = inject(DataService);
  
  constructor() {
    effect(() => {
      const nodes = this.data.nodes();
      // Update 3D visualization with real node positions
    });
    
    effect(() => {
      const telemetry = this.data.latestTelemetry();
      // Update 3D lights with real colors
    });
  }
}
```

### Step 7: Test Backend Connection ⏳
```bash
# Start backend
docker-compose up -d

# Test health endpoint
curl http://localhost:8080/health

# Test API
curl http://localhost:8080/api/v1/sites
```

### Step 8: Test Frontend ⏳
```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm start

# Visit http://localhost:4200
# Check browser console for connection status
```

### Step 9: Verify Real-time Updates ⏳
1. Open frontend dashboard
2. Flash ESP32 coordinator and nodes
3. Verify telemetry appears in real-time
4. Test light controls from UI
5. Check MQTT messages in backend logs

### Step 10: Error Handling ⏳
Add error handling to components:
```typescript
try {
  await this.data.loadSites();
} catch (error) {
  this.showErrorToast('Failed to load sites');
}
```

## 🧪 Testing Checklist

### Unit Tests ⏳
- [ ] Test EnvironmentService configuration
- [ ] Test ApiService HTTP methods
- [ ] Test WebSocketService reconnection
- [ ] Test MqttService topic matching
- [ ] Test DataService state management

### Integration Tests ⏳
- [ ] Test API → DataService flow
- [ ] Test WebSocket → Component updates
- [ ] Test MQTT → Telemetry display
- [ ] Test Component → API commands
- [ ] Test Error handling

### E2E Tests ⏳
- [ ] Load sites from backend
- [ ] Display nodes in dashboard
- [ ] Show real-time telemetry
- [ ] Control lights from UI
- [ ] Handle connection failures
- [ ] Recover from disconnections

## 📁 Files to Update

### Core Files (Already Done ✅)
- [x] `src/app/core/services/environment.service.ts`
- [x] `src/app/core/services/api.service.ts`
- [x] `src/app/core/services/websocket.service.ts`
- [x] `src/app/core/services/mqtt.service.ts`
- [x] `src/app/core/services/data.service.ts`
- [x] `src/app/core/models/api.models.ts`
- [x] `src/app/app.config.ts`

### Component Files (To Update ⏳)
- [ ] `src/app/features/dashboard/dashboard.component.ts`
- [ ] `src/app/features/dashboard/tabs/devices/devices.component.ts`
- [ ] `src/app/features/dashboard/tabs/logs/logs.component.ts`
- [ ] `src/app/features/dashboard/tabs/settings/settings.component.ts`
- [ ] `src/app/features/dashboard/components/light-monitor/light-monitor.component.ts`
- [ ] `src/app/features/dashboard/components/room-visualizer/room-visualizer.component.ts`

### Configuration Files (To Create ⏳)
- [ ] `src/environments/environment.ts`
- [ ] `src/environments/environment.prod.ts`

## 🔍 Verification Steps

### 1. Check Service Injection
```typescript
// In any component
data = inject(DataService);
console.log('DataService:', this.data);
```

### 2. Check API Connection
```typescript
ngOnInit() {
  this.data.api.getHealth().subscribe({
    next: (health) => console.log('Health:', health),
    error: (err) => console.error('API Error:', err)
  });
}
```

### 3. Check WebSocket Connection
```typescript
constructor() {
  effect(() => {
    console.log('WS Connected:', this.data.wsConnected());
  });
}
```

### 4. Check MQTT Connection
```typescript
constructor() {
  effect(() => {
    console.log('MQTT Connected:', this.data.mqttConnected());
  });
}
```

### 5. Check Telemetry Flow
```typescript
constructor() {
  effect(() => {
    const telemetry = this.data.latestTelemetry();
    console.log('Telemetry count:', telemetry.size);
  });
}
```

## 🐛 Common Issues

### Issue: Services not found
**Solution:** Run `npm install`

### Issue: TypeScript errors
**Solution:** Compile will work after `npm install`

### Issue: WebSocket not connecting
**Solution:** 
1. Check backend is running: `docker-compose ps`
2. Verify WebSocket URL in environment
3. Check CORS settings in backend

### Issue: No telemetry data
**Solution:**
1. Check MQTT broker: `docker-compose logs mosquitto`
2. Verify topic subscriptions
3. Flash ESP32 devices with correct credentials

### Issue: API timeout errors
**Solution:**
1. Increase timeout in EnvironmentService
2. Check backend logs: `docker-compose logs backend`
3. Verify MongoDB connection

## 📊 Success Criteria

- [x] All 5 services implemented
- [x] TypeScript models defined
- [x] Documentation complete
- [x] Example component created
- [ ] Dependencies installed (`npm install`)
- [ ] Environment configured
- [ ] Components updated to use services
- [ ] Backend connection verified
- [ ] Real-time updates working
- [ ] Error handling implemented
- [ ] Tests passing
- [ ] E2E flow tested

## 🎯 Next Steps After Integration

1. **Add Authentication**
   - Login component
   - Auth token storage
   - Protected routes

2. **Enhance UI**
   - Loading states
   - Error notifications
   - Success toasts
   - Connection status indicator

3. **Add Features**
   - Site management (CRUD)
   - Node configuration
   - Zone management
   - Analytics dashboard

4. **Performance**
   - Virtual scrolling for large lists
   - Lazy loading
   - Memoization
   - Debouncing

5. **PWA**
   - Service worker
   - Offline support
   - Push notifications
   - App manifest

## 📞 Support

- **Service Docs:** `src/app/core/services/README.md`
- **Quick Reference:** `FRONTEND_QUICK_REFERENCE.md`
- **Example Component:** `src/app/features/example-dashboard/`
- **Models:** `src/app/core/models/api.models.ts`

## ✨ You're Almost There!

The services are ready. Just:
1. Run `npm install`
2. Update your components
3. Test the integration
4. Deploy!

🚀 Happy coding!
