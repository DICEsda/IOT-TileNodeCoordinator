import { CommonModule } from '@angular/common';
import { Component, signal, OnInit, OnDestroy } from '@angular/core';
import { FormsModule } from '@angular/forms';
import { ApiService } from '../../../../core/services/api.service';
import { MqttService } from '../../../../core/services/mqtt.service';
import { Subscription } from 'rxjs';
import { SystemSettings, SettingsUpdateMessage } from '../../../../core/models/settings.models';

@Component({
  selector: 'app-settings-new',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './settings-new.component.html',
  styleUrls: ['./settings.component.scss', './settings-new-additional.scss']
})
export class SettingsNewComponent implements OnInit, OnDestroy {
  // Authentication
  isAuthenticated = signal(false);
  passwordInput = '';
  passwordError = '';
  
  // Developer mode toggle
  developerModeEnabled = signal(false);
  
  // Settings sections
  settings = signal<SystemSettings | null>(null);
  
  // Loading & saving states
  isLoading = signal(false);
  isSaving = signal(false);
  saveMessage = signal<{ type: 'success' | 'error', text: string } | null>(null);
  
  // Subscriptions
  private subscriptions: Subscription[] = [];
  
  // Active section for mobile/accordion view
  activeSection = signal<string | null>(null);
  
  // WiFi properties
  wifiSSID = '';
  wifiPassword = '';
  
  // MQTT properties
  mqttBroker = '';
  mqttUsername = '';
  mqttPassword = '';
  
  // Google Home integration
  googleHomeEnabled = false;
  googleHomeConnected = false;
  googleProjectId = '';
  googleClientId = '';
  googleClientSecret = '';
  googleApiKey = '';
  
  // System configuration properties
  autoMode = false;
  motionSensitivity = 50;
  lightIntensity = 80;
  autoOffDelay = 30;
  
  // Node properties
  nodeBrightness = 100;
  nodeZone = '';
  nodeName = '';
  testColor = '#ffffff';
  
  // Zones
  zones: string[] = ['Living Room', 'Bedroom', 'Kitchen'];
  
  // Coordinator status
  coordinatorId = 'COORD-001';
  coordinatorOnline = signal(false);
  firmwareVersion = 'v1.0.0';
  pairingMode = signal(false);
  pairingTimeLeft = 0;
  pairingProgress = signal(0);
  
  // Node management
  node = signal<any>(null);

  constructor(
    private api: ApiService,
    private mqtt: MqttService
  ) {}

  ngOnInit() {
    // Subscribe to real-time settings updates via WebSocket
    const settingsSub = this.mqtt.messages$.subscribe((msg) => {
      if (msg.payload?.type === 'settings_update') {
        this.handleSettingsUpdate(msg.payload as SettingsUpdateMessage);
      }
    });
    this.subscriptions.push(settingsSub);
  }

  ngOnDestroy() {
    this.subscriptions.forEach(sub => sub.unsubscribe());
  }

  /**
   * Validate password and load settings
   */
  async authenticate() {
    if (!this.passwordInput) {
      this.passwordError = 'Please enter a password';
      return;
    }

    try {
      const response = await this.api.validateSettingsPassword(this.passwordInput).toPromise();
      
      if (response?.valid) {
        this.isAuthenticated.set(true);
        this.passwordError = '';
        this.passwordInput = '';
        await this.loadSettings();
      } else {
        this.passwordError = 'Invalid password';
      }
    } catch (error) {
      console.error('Authentication error:', error);
      this.passwordError = 'Authentication failed';
    }
  }

  /**
   * Load all system settings
   */
  async loadSettings() {
    this.isLoading.set(true);
    
    try {
      const data = await this.api.getSystemSettings().toPromise();
      if (data?.data) {
        this.settings.set(data.data);
      }
    } catch (error) {
      console.error('Failed to load settings:', error);
      this.showMessage('error', 'Failed to load settings');
    } finally {
      this.isLoading.set(false);
    }
  }

  /**
   * Save a specific section of settings
   */
  async saveSection(section: keyof SystemSettings) {
    if (!this.settings()) return;
    
    this.isSaving.set(true);
    
    try {
      const sectionData = this.settings()![section];
      await this.api.updateSystemSettings(section, sectionData).toPromise();
      
      this.showMessage('success', `${this.getSectionTitle(section)} saved successfully`);
    } catch (error) {
      console.error(`Failed to save ${section}:`, error);
      this.showMessage('error', `Failed to save ${this.getSectionTitle(section)}`);
    } finally {
      this.isSaving.set(false);
    }
  }

  /**
   * Save all settings at once
   */
  async saveAllSettings() {
    if (!this.settings()) return;
    
    this.isSaving.set(true);
    
    const sections: (keyof SystemSettings)[] = [
      'system_identity',
      'network',
      'mqtt',
      'coordinator',
      'node',
      'zones',
      'automation',
      'telemetry'
    ];
    
    if (this.developerModeEnabled()) {
      sections.push('developer');
    }
    
    try {
      for (const section of sections) {
        const sectionData = this.settings()![section];
        await this.api.updateSystemSettings(section, sectionData).toPromise();
      }
      
      this.showMessage('success', 'All settings saved successfully');
    } catch (error) {
      console.error('Failed to save all settings:', error);
      this.showMessage('error', 'Failed to save some settings');
    } finally {
      this.isSaving.set(false);
    }
  }

  /**
   * Reset all settings to defaults
   */
  async resetToDefaults() {
    if (!confirm('Are you sure you want to reset all settings to their default values? This cannot be undone.')) {
      return;
    }
    
    this.isSaving.set(true);
    
    try {
      await this.api.resetSettingsToDefaults().toPromise();
      await this.loadSettings();
      
      this.showMessage('success', 'Settings reset to defaults');
    } catch (error) {
      console.error('Failed to reset settings:', error);
      this.showMessage('error', 'Failed to reset settings');
    } finally {
      this.isSaving.set(false);
    }
  }

  /**
   * Handle real-time settings updates from WebSocket
   */
  private handleSettingsUpdate(update: SettingsUpdateMessage) {
    const current = this.settings();
    if (!current) return;
    
    const updated = { ...current };
    updated[update.section] = { ...current[update.section], ...update.settings };
    
    this.settings.set(updated);
    console.log(`Settings updated: ${update.section}`, update.settings);
  }

  /**
   * Add a new zone to the zones list
   */
  addZone() {
    const settings = this.settings();
    if (!settings) return;
    
    const zoneName = prompt('Enter zone name:');
    if (!zoneName) return;
    
    const updated = { ...settings };
    updated.zones.default_zones = [...updated.zones.default_zones, zoneName];
    this.settings.set(updated);
  }

  /**
   * Remove a zone from the zones list
   */
  removeZone(index: number) {
    const settings = this.settings();
    if (!settings) return;
    
    if (!confirm('Remove this zone?')) return;
    
    const updated = { ...settings };
    updated.zones.default_zones = updated.zones.default_zones.filter((_, i) => i !== index);
    this.settings.set(updated);
  }

  /**
   * Update a zone name
   */
  updateZoneName(index: number, newName: string) {
    const settings = this.settings();
    if (!settings) return;
    
    const updated = { ...settings };
    updated.zones.default_zones[index] = newName;
    this.settings.set(updated);
  }

  /**
   * Toggle section expansion
   */
  toggleSection(section: string) {
    this.activeSection.set(this.activeSection() === section ? null : section);
  }

  /**
   * Check if section is expanded
   */
  isSectionExpanded(section: string): boolean {
    return this.activeSection() === section;
  }

  /**
   * Get human-readable section title
   */
  getSectionTitle(section: string): string {
    const titles: Record<string, string> = {
      system_identity: 'System Identity',
      network: 'Network & Connectivity',
      mqtt: 'MQTT Behavior',
      coordinator: 'Coordinator Configuration',
      node: 'Node Defaults',
      zones: 'Zones',
      automation: 'Energy & Automation',
      telemetry: 'Telemetry & Logging',
      developer: 'Developer Tools'
    };
    return titles[section] || section;
  }

  /**
   * Show temporary message
   */
  private showMessage(type: 'success' | 'error', text: string) {
    this.saveMessage.set({ type, text });
    setTimeout(() => this.saveMessage.set(null), 3000);
  }

  /**
   * Logout from settings
   */
  logout() {
    this.isAuthenticated.set(false);
    this.settings.set(null);
    this.developerModeEnabled.set(false);
  }
  
  /**
   * Connect to Google Home
   */
  connectGoogleHome() {
    // TODO: Implement Google Home OAuth flow
    console.log('Google Home connection requested');
    this.googleHomeConnected = true;
  }
  
  /**
   * Disconnect from Google Home
   */
  disconnectGoogleHome() {
    if (confirm('Are you sure you want to disconnect Google Home?')) {
      this.googleHomeConnected = false;
      console.log('Google Home disconnected');
    }
  }
  
  /**
   * Save settings (called from toolbar)
   */
  saveSettings() {
    console.log('Saving settings...');
    this.saveAllSettings();
  }
  
  /**
   * Enter pairing mode
   */
  enterPairingMode() {
    console.log('Entering pairing mode...');
    // TODO: Send MQTT command to coordinator
  }
  
  /**
   * Restart coordinator
   */
  restartCoordinator() {
    if (confirm('Are you sure you want to restart the coordinator?')) {
      console.log('Restarting coordinator...');
      // TODO: Send restart command
    }
  }
  
  /**
   * Delete a node
   */
  deleteNode() {
    if (confirm('Are you sure you want to delete this node?')) {
      console.log('Deleting node...');
      // TODO: Send delete command
    }
  }
  
  /**
   * Update node zone assignment
   */
  updateNodeZone() {
    console.log('Updating node zone:', this.nodeZone);
    // TODO: Send zone update command
  }
  
  /**
   * Send test color to node
   */
  sendTestColor() {
    console.log('Sending test color:', this.testColor);
    // TODO: Send color test command
  }
  
  /**
   * Turn off LEDs
   */
  turnOffLEDs() {
    console.log('Turning off LEDs');
    // TODO: Send off command
  }
  
  /**
   * Update node brightness
   */
  updateBrightness() {
    console.log('Updating brightness:', this.nodeBrightness);
    // TODO: Send brightness update
  }
  
  /**
   * Update WiFi configuration
   */
  updateWiFiConfig() {
    console.log('Updating WiFi config:', this.wifiSSID);
    // TODO: Send WiFi config update
  }
  
  /**
   * Update node name
   */
  updateNodeName() {
    console.log('Updating node name:', this.nodeName);
    // TODO: Send node name update
  }
}
