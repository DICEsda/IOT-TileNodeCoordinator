# Debug Dashboard - Integration Steps

## Quick Integration (5 minutes)

### Step 1: Add Route

Edit `src/app/app.routes.ts`:

```typescript
import { Routes } from '@angular/router';

export const routes: Routes = [
  // ... your existing routes ...
  
  // Add Debug Dashboard route
  {
    path: 'debug',
    loadComponent: () => 
      import('./features/debug/debug.component').then(m => m.DebugComponent)
  }
];
```

### Step 2: Add Navigation Link (Optional)

If you have a navigation menu, add a link to the debug page.

Example in your main navigation component:

```html
<nav>
  <a routerLink="/">Home</a>
  <a routerLink="/dashboard">Dashboard</a>
  <a routerLink="/lights">Lights</a>
  <a routerLink="/zones">Zones</a>
  <!-- Add debug link -->
  <a routerLink="/debug">🔧 Debug</a>
</nav>
```

### Step 3: Test

1. Start your frontend:
   ```bash
   cd IOT-Frontend-main/IOT-Frontend-main
   ng serve
   ```

2. Open browser to: `http://localhost:4200/debug`

3. You should see the Debug Dashboard with all monitoring sections

## Verification Checklist

- [ ] Page loads without errors
- [ ] Connection Status section appears
- [ ] Environment Configuration shows correct URLs
- [ ] Health Check section visible
- [ ] Live Telemetry Monitor section visible
- [ ] MQTT Topic Test section visible
- [ ] Auto-refresh working (check connection timestamps updating every 5s)

## Expected Behavior

### On Page Load
- Connection statuses automatically check all services
- Backend health check runs automatically
- All sections render immediately

### With Backend Running
- HTTP API status should show "CONNECTED" (green)
- Backend Health should show "✅ Healthy"
- Latency should display (e.g., "45ms")

### With Backend Not Running
- HTTP API status shows "ERROR" (orange/red)
- Backend Health shows "❌ Unable to fetch health status"
- Retry button appears

### When Monitoring Telemetry
- Click "Start Monitor"
- Button changes to "⏸️ Monitoring..."
- Messages start appearing if telemetry is being published
- Each message shows timestamp, source, topic, and payload

## Troubleshooting

### Problem: Component not found error
**Solution**: Verify the file exists at:
```
IOT-Frontend-main/src/app/features/debug/debug.component.ts
```

### Problem: Import errors
**Solution**: The component is standalone and self-contained. It only depends on:
- `@angular/core`
- `@angular/common`
- `@angular/forms`
- `rxjs`

These should already be in your `package.json`. If not:
```bash
npm install @angular/core @angular/common @angular/forms rxjs
```

### Problem: Services not found
**Solution**: Verify your core services exist at:
- `src/app/core/services/environment.service.ts`
- `src/app/core/services/api.service.ts`
- `src/app/core/services/websocket.service.ts`
- `src/app/core/services/mqtt.service.ts`

(These were created in the previous "Frontend Services" phase)

### Problem: Signals not working
**Solution**: Signals are available in Angular 16+. If you're on an older version:
1. Upgrade Angular: `ng update @angular/core @angular/cli`
2. Or replace signals with BehaviorSubjects (requires code changes)

## Advanced Configuration

### Custom Site ID

The debug dashboard uses `'site001'` as the default site ID for MQTT subscriptions.

To change this, edit the component:

```typescript
// In debug.component.ts, find:
const siteId = 'site001';

// Change to your site ID:
const siteId = 'site_custom_123';
```

Or better, read from environment service:

```typescript
const siteId = this.environmentService.defaultSiteId || 'site001';
```

### Custom Auto-Refresh Interval

The dashboard auto-refreshes connection statuses every 5 seconds.

To change this, edit:

```typescript
// In debug.component.ts, find:
interval(5000)

// Change to your desired interval (milliseconds):
interval(10000)  // 10 seconds
```

### Adjust Message Retention

The telemetry monitor keeps the last 50 messages.

To change this:

```typescript
// In debug.component.ts, in addTelemetryMessage():
return newMessages.slice(0, 50);

// Change to your desired count:
return newMessages.slice(0, 100);  // Keep 100 messages
```

## Production Considerations

### Security

The debug dashboard is intended for development and ops monitoring. Consider:

1. **Restrict Access** in production:
   ```typescript
   {
     path: 'debug',
     canActivate: [AdminGuard],  // Add guard
     loadComponent: () => import('./features/debug/debug.component').then(m => m.DebugComponent)
   }
   ```

2. **Remove in Production** (not recommended, useful for ops):
   ```typescript
   const debugRoute = environment.production ? [] : [{
     path: 'debug',
     loadComponent: () => import('./features/debug/debug.component').then(m => m.DebugComponent)
   }];
   
   export const routes: Routes = [
     // ... your routes ...
     ...debugRoute
   ];
   ```

### Performance

The debug dashboard:
- Auto-refreshes every 5 seconds (minimal impact)
- Only monitors when "Start Monitor" is clicked
- Keeps max 50 messages (bounded memory)
- Properly cleans up subscriptions on destroy

Should have minimal performance impact even in production.

## Features to Add Later

Potential enhancements you might want to add:

1. **Export Logs**:
   ```typescript
   exportLogs() {
     const logs = JSON.stringify(this.telemetryMessages(), null, 2);
     const blob = new Blob([logs], { type: 'application/json' });
     const url = URL.createObjectURL(blob);
     const a = document.createElement('a');
     a.href = url;
     a.download = `telemetry-${Date.now()}.json`;
     a.click();
   }
   ```

2. **Filter Messages**:
   ```html
   <input [(ngModel)]="filterTopic" placeholder="Filter by topic...">
   ```
   
   ```typescript
   get filteredMessages() {
     if (!this.filterTopic) return this.telemetryMessages();
     return this.telemetryMessages().filter(m => 
       m.topic.includes(this.filterTopic)
     );
   }
   ```

3. **Chart Latency Over Time**:
   Use a library like Chart.js or ng2-charts to plot API latency.

4. **Alert Notifications**:
   Use browser Notification API to alert on disconnections.

## Complete Example

Here's a complete minimal `app.routes.ts` with the debug dashboard:

```typescript
import { Routes } from '@angular/router';

export const routes: Routes = [
  {
    path: '',
    redirectTo: '/dashboard',
    pathMatch: 'full'
  },
  {
    path: 'dashboard',
    loadComponent: () => 
      import('./features/dashboard/dashboard.component').then(m => m.DashboardComponent)
  },
  {
    path: 'lights',
    loadComponent: () => 
      import('./features/lights/lights.component').then(m => m.LightsComponent)
  },
  {
    path: 'zones',
    loadComponent: () => 
      import('./features/zones/zones.component').then(m => m.ZonesComponent)
  },
  {
    path: 'debug',
    loadComponent: () => 
      import('./features/debug/debug.component').then(m => m.DebugComponent)
  },
  {
    path: '**',
    redirectTo: '/dashboard'
  }
];
```

## Support

For issues or questions:
1. Check `IOT-Frontend-main/docs/DEBUG_DASHBOARD.md` for full documentation
2. Review component code - it's well-commented
3. Check browser console for errors
4. Verify all services are properly initialized

---

**Status**: ✅ Ready to Integrate  
**Estimated Integration Time**: 5 minutes  
**Required Angular Version**: 16+ (for signals)
