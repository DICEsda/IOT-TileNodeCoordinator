import { CommonModule } from '@angular/common';
import { Component, signal, OnInit, OnDestroy } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { ApiService } from '../../../../core/services/api.service';
import { MqttService } from '../../../../core/services/mqtt.service';
import { Subscription, firstValueFrom } from 'rxjs';

interface NodeStatus {
  node_id: string;
  name?: string;
  zone_id?: string;
  status: string;
  brightness?: number;
  last_seen?: Date;
}

@Component({
  selector: 'app-settings',
  imports: [CommonModule, FormsModule],
  templateUrl: './settings.component.html',
  styleUrl: './settings.component.scss'
})
export class SettingsComponent implements OnInit, OnDestroy {
  // Coordinator
  coordinatorId = ''; // Will be discovered from MQTT telemetry
  siteId = 'site001';
  coordinatorOnline = signal(false);
  firmwareVersion = '1.0.0';
  pairingMode = signal(false);
  pairingTimeLeft = 0;
  private pairingTimer?: any;
  private coordinatorDiscovered = false;

  // Node (single node)
  node = signal<NodeStatus | null>(null);
  nodeZone = '';
  nodeName = '';
  nodeBrightness = 80;
  testColor = '#00FFBF';

  // Zones
  zones: string[] = ['Living Room', 'Bedroom', 'Kitchen', 'Bathroom', 'Office', 'Hallway'];

  // WiFi
  wifiSSID = '';
  wifiPassword = '';

  // MQTT
  mqttBroker = 'tcp://mosquitto:1883';
  mqttUsername = 'user1';
  mqttPassword = '';

  // Google Home
  googleHomeEnabled = false;
  googleHomeConnected = false;
  googleProjectId = '';
  googleClientId = '';
  googleClientSecret = '';
  googleApiKey = '';

  // System
  autoMode = signal(true);
  motionSensitivity = signal(50);
  lightIntensity = signal(80);
  delayBeforeOff = signal(30);

  // Notifications & advanced toggles
  emailNotifications = signal(false);
  pushNotifications = signal(false);
  batteryAlerts = signal(true);
  motionAlerts = signal(true);
  dataLogging = signal(false);
  cloudSync = signal(false);
  energySavingMode = signal(false);
  debugMode = signal(false);

  private subscriptions: Subscription[] = [];

  constructor(
    private api: ApiService,
    private mqtt: MqttService
  ) {}

  ngOnInit() {
    this.loadSettings();
    // Note: loadNode() will be called after coordinator ID is discovered
    
    // Subscribe to coordinator telemetry to discover coordinator ID
    const coordSub = this.mqtt.subscribeCoordinatorTelemetry(this.siteId, '+').subscribe({
      next: (data: any) => {
        // Extract coordinator ID from MQTT topic or payload
        if (!this.coordinatorDiscovered && data.coord_id) {
          this.coordinatorId = data.coord_id;
          this.coordinatorDiscovered = true;
          console.log('[Settings] Discovered coordinator ID:', this.coordinatorId);
          // Now check coordinator status and load nodes
          this.checkCoordinatorStatus();
          this.loadNode();
        }
        // Update online status
        if (data.coord_id === this.coordinatorId) {
          this.coordinatorOnline.set(true);
        }
      },
      error: (err: any) => console.error('Coordinator telemetry error:', err)
    });
    this.subscriptions.push(coordSub);
    
    // Subscribe to node updates
    const nodeSub = this.mqtt.subscribeNodeTelemetry(this.siteId, '+').subscribe({
      next: (data: any) => {
        if (data.node_id) {
          this.node.set({
            node_id: data.node_id,
            name: data.name,
            zone_id: data.zone_id,
            status: data.status || 'online',
            brightness: data.brightness,
            last_seen: new Date()
          });
        }
      },
      error: (err: any) => console.error('Node telemetry error:', err)
    });
    this.subscriptions.push(nodeSub);
    
    // Fallback: try checking status after 2 seconds if no telemetry received
    setTimeout(() => {
      if (!this.coordinatorDiscovered) {
        console.warn('[Settings] No coordinator telemetry received, attempting direct query');
        // Try with default coord001 or MAC address format
        this.coordinatorId = 'coord001';
        this.checkCoordinatorStatus();
        this.loadNode();
      }
    }, 2000);
  }

  ngOnDestroy() {
    this.subscriptions.forEach(sub => sub.unsubscribe());
    if (this.pairingTimer) {
      clearInterval(this.pairingTimer);
    }
  }

  pairingProgress(): number {
    return (this.pairingTimeLeft / 60) * 100;
  }

  async loadSettings() {
    try {
      const settings = await firstValueFrom(this.api.getSettings(this.siteId));
      if (!settings) {
        return;
      }

      this.autoMode.set(settings.auto_mode ?? true);
      this.motionSensitivity.set(settings.motion_sensitivity ?? 50);
      this.lightIntensity.set(settings.light_intensity ?? 80);
      this.delayBeforeOff.set(settings.auto_off_delay ?? 30);

      if (Array.isArray(settings.zones) && settings.zones.length) {
        this.zones = [...settings.zones];
      }

      this.mqttBroker = settings.mqtt_broker ?? this.mqttBroker;
      this.mqttUsername = settings.mqtt_username ?? this.mqttUsername;
      this.googleHomeEnabled = settings.google_home_enabled ?? false;
      this.googleProjectId = settings.google_project_id ?? '';
      this.googleClientId = settings.google_client_id ?? '';
      this.googleClientSecret = settings.google_client_secret ?? '';
      this.googleApiKey = settings.google_api_key ?? '';
    } catch (error) {
      console.error('Failed to load settings:', error);
    }
  }

  async loadNode() {
    try {
      const nodes = await this.api.getNodes(this.siteId, this.coordinatorId).toPromise();
      if (nodes && nodes.length > 0) {
        const nodeData = nodes[0];
        this.node.set(nodeData);
        this.nodeName = nodeData.name || nodeData.node_id;
        this.nodeZone = nodeData.zone_id || '';
        this.nodeBrightness = nodeData.brightness || 80;
      }
    } catch (error) {
      console.error('Failed to load node:', error);
    }
  }

  async checkCoordinatorStatus() {
    if (!this.coordinatorId) {
      console.warn('[Settings] Cannot check coordinator status: ID not yet discovered');
      return;
    }
    
    try {
      const coordinator = await this.api.getCoordinator(this.siteId, this.coordinatorId).toPromise();
      if (coordinator) {
        // Coordinator exists in database, consider it online
        this.coordinatorOnline.set(true);
        this.firmwareVersion = coordinator.fw_version || '1.0.0';
        console.log('[Settings] Coordinator found via API:', coordinator);
      }
    } catch (error: any) {
      const errorMsg = error?.message || error?.toString() || '';
      if (errorMsg.includes('404')) {
        console.warn(`[Settings] Coordinator ${this.coordinatorId} not found in database yet (waiting for telemetry)`);
      } else {
        console.error('Failed to check coordinator status:', error);
      }
      // Don't set offline if 404 - coordinator may just not be in DB yet but is publishing MQTT
      if (!errorMsg.includes('404')) {
        this.coordinatorOnline.set(false);
      }
    }
  }

  enterPairingMode() {
    if (!this.coordinatorOnline() || this.pairingMode()) return;

    this.api.startPairing(this.siteId, this.coordinatorId, 60).subscribe({
      next: () => {
        this.pairingMode.set(true);
        this.pairingTimeLeft = 60;
        
        this.pairingTimer = setInterval(() => {
          this.pairingTimeLeft--;
          if (this.pairingTimeLeft <= 0) {
            this.pairingMode.set(false);
            clearInterval(this.pairingTimer);
          }
        }, 1000);
      },
      error: (err) => {
        console.error('Failed to enter pairing mode:', err);
        alert('Failed to enter pairing mode');
      }
    });
  }

  restartCoordinator() {
    if (!this.coordinatorOnline()) return;
    
    if (confirm('Are you sure you want to restart the coordinator?')) {
      this.api.restartCoordinator(this.siteId, this.coordinatorId).subscribe({
        next: () => {
          alert('Coordinator restart initiated');
          this.coordinatorOnline.set(false);
          setTimeout(() => this.checkCoordinatorStatus(), 10000);
        },
        error: (err) => {
          console.error('Failed to restart coordinator:', err);
          alert('Failed to restart coordinator');
        }
      });
    }
  }

  deleteNode() {
    if (!this.node()) return;
    
    if (confirm('Are you sure you want to remove this node? This cannot be undone.')) {
      this.api.deleteNode(this.siteId, this.coordinatorId, this.node()!.node_id).subscribe({
        next: () => {
          alert('Node removed successfully');
          this.node.set(null);
          this.nodeName = '';
          this.nodeZone = '';
        },
        error: (err) => {
          console.error('Failed to delete node:', err);
          alert('Failed to delete node');
        }
      });
    }
  }

  updateNodeZone() {
    if (!this.node()) return;
    
    this.api.updateNodeZone(this.siteId, this.coordinatorId, this.node()!.node_id, this.nodeZone).subscribe({
      next: () => {
        console.log('Node zone updated');
      },
      error: (err) => {
        console.error('Failed to update node zone:', err);
        alert('Failed to update zone');
      }
    });
  }

  updateNodeName() {
    if (!this.node()) return;
    
    this.api.updateNodeName(this.siteId, this.coordinatorId, this.node()!.node_id, this.nodeName).subscribe({
      next: () => {
        console.log('Node name updated');
      },
      error: (err) => {
        console.error('Failed to update node name:', err);
      }
    });
  }

  sendTestColor() {
    if (!this.node()) return;
    
    const hex = this.testColor.replace('#', '');
    const r = parseInt(hex.substr(0, 2), 16);
    const g = parseInt(hex.substr(2, 2), 16);
    const b = parseInt(hex.substr(4, 2), 16);
    
    this.api.sendNodeColor(this.siteId, this.coordinatorId, this.node()!.node_id, r, g, b, 0).subscribe({
      next: () => {
        console.log('Test color sent');
      },
      error: (err) => {
        console.error('Failed to send test color:', err);
        alert('Failed to send color command');
      }
    });
  }

  turnOffLEDs() {
    if (!this.node()) return;
    
    this.api.turnOffNode(this.siteId, this.coordinatorId, this.node()!.node_id).subscribe({
      next: () => {
        console.log('LEDs turned off');
      },
      error: (err) => {
        console.error('Failed to turn off LEDs:', err);
        alert('Failed to turn off LEDs');
      }
    });
  }

  updateBrightness() {
    if (!this.node()) return;
    
    const brightness = Math.round((this.nodeBrightness / 100) * 255);
    this.api.setNodeBrightness(this.siteId, this.coordinatorId, this.node()!.node_id, brightness).subscribe({
      next: () => {
        console.log('Brightness updated');
      },
      error: (err) => {
        console.error('Failed to update brightness:', err);
      }
    });
  }

  addZone() {
    this.zones.push('New Zone');
  }

  removeZone(index: number) {
    if (confirm('Remove this zone?')) {
      this.zones.splice(index, 1);
    }
  }

  updateWiFiConfig() {
    if (!this.wifiSSID || !this.wifiPassword) {
      alert('Please enter both SSID and password');
      return;
    }
    
    this.api.updateWiFiConfig(this.siteId, this.coordinatorId, this.wifiSSID, this.wifiPassword).subscribe({
      next: () => {
        alert('WiFi configuration updated. Coordinator will reconnect.');
        this.wifiPassword = '';
      },
      error: (err) => {
        console.error('Failed to update WiFi config:', err);
        alert('Failed to update WiFi configuration');
      }
    });
  }

  connectGoogleHome() {
    const authUrl = `${this.api['baseUrl']}/api/v1/google/auth`;
    window.open(authUrl, 'google-auth', 'width=600,height=700');
    
    window.addEventListener('message', (event) => {
      if (event.data.type === 'google-auth-success') {
        this.googleHomeConnected = true;
        alert('Google Home connected successfully!');
      }
    });
  }

  disconnectGoogleHome() {
    if (confirm('Disconnect Google Home?')) {
      this.api.disconnectGoogleHome(this.siteId).subscribe({
        next: () => {
          this.googleHomeConnected = false;
          alert('Google Home disconnected');
        },
        error: (err) => {
          console.error('Failed to disconnect Google Home:', err);
          alert('Failed to disconnect');
        }
      });
    }
  }

  saveSettings() {
    const settings = {
      site_id: this.siteId,
      auto_mode: this.autoMode(),
      motion_sensitivity: this.motionSensitivity(),
      light_intensity: this.lightIntensity(),
      auto_off_delay: this.delayBeforeOff(),
      zones: this.zones,
      mqtt_broker: this.mqttBroker,
      mqtt_username: this.mqttUsername,
      google_home_enabled: this.googleHomeEnabled,
      google_project_id: this.googleProjectId,
      google_client_id: this.googleClientId,
      google_client_secret: this.googleClientSecret,
      google_api_key: this.googleApiKey
    };

    this.api.saveSettings(this.siteId, settings).subscribe({
      next: () => {
        alert('Settings saved successfully!');
      },
      error: (err) => {
        console.error('Failed to save settings:', err);
        alert('Failed to save settings');
      }
    });
  }

  resetToDefaults(): void {
    this.autoMode.set(true);
    this.motionSensitivity.set(50);
    this.lightIntensity.set(80);
    this.delayBeforeOff.set(30);

    this.emailNotifications.set(false);
    this.pushNotifications.set(false);
    this.batteryAlerts.set(true);
    this.motionAlerts.set(true);
    this.dataLogging.set(false);
    this.cloudSync.set(false);
    this.energySavingMode.set(false);
    this.debugMode.set(false);

    this.zones = ['Living Room', 'Bedroom', 'Kitchen', 'Bathroom', 'Office', 'Hallway'];
    this.mqttBroker = 'tcp://mosquitto:1883';
    this.mqttUsername = 'user1';
    this.googleHomeEnabled = false;
    this.googleProjectId = '';
    this.googleClientId = '';
    this.googleClientSecret = '';
    this.googleApiKey = '';
  }
}

