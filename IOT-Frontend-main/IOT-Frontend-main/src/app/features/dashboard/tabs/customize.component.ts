import { Component, OnInit, OnDestroy, signal, computed, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { Subject, takeUntil, firstValueFrom } from 'rxjs';
import { DataService } from '../../../core/services/data.service';
import { ApiService } from '../../../core/services/api.service';
import { MqttService } from '../../../core/services/mqtt.service';

/**
 * Customize Tab Component
 * 
 * Provides UI for customizing device parameters:
 * - HLK-LD2450 mmWave Radar: detection range, FOV, sensitivity, modes, reporting rate
 * - TSL2561 Light Sensor: integration time, gain, auto-range, thresholds
 * - SK6812B LED Strips: color, brightness, effects, speed, segments, power limits
 * 
 * Connects to backend REST API routes and synchronizes via MQTT + WebSocket
 */
@Component({
  selector: 'app-customize',
  standalone: true,
  imports: [CommonModule, FormsModule],
  templateUrl: './customize.component.html',
  styleUrl: './customize.component.scss'
})
export class CustomizeComponent implements OnInit, OnDestroy {
  private readonly destroy$ = new Subject<void>();
  private readonly dataService = inject(DataService);
  private readonly apiService = inject(ApiService);
  private readonly mqttService = inject(MqttService);

  // Device Selection
  selectedDeviceType = signal<'coordinator' | 'node' | null>(null);
  selectedDeviceId = signal<string | null>(null);
  selectedDevice = computed(() => {
    const type = this.selectedDeviceType();
    const id = this.selectedDeviceId();
    if (!type || !id) return null;
    
    if (type === 'coordinator') {
      return this.dataService.coordinators().get(id);
    } else {
      return this.dataService.nodes().get(id);
    }
  });

  // Device Lists
  coordinators = computed(() => Array.from(this.dataService.coordinators().values()));
  nodes = computed(() => Array.from(this.dataService.nodes().values()));

  // Active Section
  activeSection = signal<'radar' | 'light' | 'led'>('radar');

  // === Coordinator Settings (from ConfigManager) ===
  presenceDebounceMs = signal(150);
  occupancyHoldMs = signal(5000);
  fadeInMs = signal(150);
  fadeOutMs = signal(1000);
  pairingWindowS = signal(120);

  // === Node Settings (from ConfigManager) ===
  pwmFreqHz = signal(1000);
  pwmResBits = signal(12);
  telemetryS = signal(5);
  rxWindowMs = signal(20);
  rxPeriodMs = signal(100);
  derateStartC = signal(70.0);
  derateMinDutyPct = signal(30);
  retryCount = signal(3);
  cmdTtlMs = signal(1500);

  // === Status (read-only) ===
  radarEnabled = signal(true);
  radarOnline = signal(false);
  lightSensorEnabled = signal(true);

  // === LED Configuration (nodes only - 4 pixels SK6812B) ===
  ledEnabled = signal(true);
  ledBrightness = signal(80);
  ledColor = signal('#00FFBF');

  // Status
  loading = signal(false);
  saving = signal(false);
  saveSuccess = signal(false);
  errorMessage = signal('');

  ngOnInit(): void {
    // Auto-select first coordinator if available
    const coords = this.coordinators();
    if (coords.length > 0) {
      this.selectDevice('coordinator', coords[0].coord_id);
    }

    // Listen for configuration updates via MQTT
    this.subscribeToConfigUpdates();
  }

  ngOnDestroy(): void {
    this.destroy$.next();
    this.destroy$.complete();
  }

  /**
   * Select a device to customize
   */
  selectDevice(type: 'coordinator' | 'node', deviceId: string): void {
    this.selectedDeviceType.set(type);
    this.selectedDeviceId.set(deviceId);
    this.loadDeviceConfiguration();
  }

  /**
   * Set active customization section
   */
  setActiveSection(section: 'radar' | 'light' | 'led'): void {
    this.activeSection.set(section);
  }

  /**
   * Load device configuration from backend
   */
  private async loadDeviceConfiguration(): Promise<void> {
    const type = this.selectedDeviceType();
    const id = this.selectedDeviceId();
    
    if (!type || !id) return;

    this.loading.set(true);
    this.errorMessage.set('');

    try {
      const siteId = this.dataService.activeSiteId() || 'site001';
      
      // Load configuration from backend API
      const config = await firstValueFrom(
        this.apiService.get<any>(`/api/v1/${type}/${id}/customize`)
      );

      if (config) {
        this.applyConfiguration(config);
      }
    } catch (error) {
      console.error('[CustomizeComponent] Failed to load configuration:', error);
      this.errorMessage.set('Failed to load device configuration');
    } finally {
      this.loading.set(false);
    }
  }

  /**
   * Apply configuration to UI
   */
  private applyConfiguration(config: any): void {
    // Coordinator configuration (from ConfigManager)
    if (config.coordinator) {
      this.presenceDebounceMs.set(config.coordinator.presence_debounce_ms ?? 150);
      this.occupancyHoldMs.set(config.coordinator.occupancy_hold_ms ?? 5000);
      this.fadeInMs.set(config.coordinator.fade_in_ms ?? 150);
      this.fadeOutMs.set(config.coordinator.fade_out_ms ?? 1000);
      this.pairingWindowS.set(config.coordinator.pairing_window_s ?? 120);
    }

    // Node configuration (from ConfigManager)
    if (config.node) {
      this.pwmFreqHz.set(config.node.pwm_freq_hz ?? 1000);
      this.pwmResBits.set(config.node.pwm_res_bits ?? 12);
      this.telemetryS.set(config.node.telemetry_s ?? 5);
      this.rxWindowMs.set(config.node.rx_window_ms ?? 20);
      this.rxPeriodMs.set(config.node.rx_period_ms ?? 100);
      this.derateStartC.set(config.node.derate_start_c ?? 70.0);
      this.derateMinDutyPct.set(config.node.derate_min_duty_pct ?? 30);
      this.retryCount.set(config.node.retry_count ?? 3);
      this.cmdTtlMs.set(config.node.cmd_ttl_ms ?? 1500);
    }

    // Radar status (read-only)
    if (config.radar) {
      this.radarEnabled.set(config.radar.enabled ?? true);
      this.radarOnline.set(config.radar.online ?? false);
    }

    // Light sensor status (read-only)
    if (config.light) {
      this.lightSensorEnabled.set(config.light.enabled ?? true);
    }

    // LED configuration (nodes only)
    if (config.led) {
      this.ledEnabled.set(config.led.enabled ?? true);
      this.ledBrightness.set(config.led.brightness ?? 80);
      this.ledColor.set(config.led.color ?? '#00FFBF');
    }
  }

  /**
   * Subscribe to configuration updates via MQTT
   */
  private subscribeToConfigUpdates(): void {
    // Subscribe to device-specific configuration topics
    // Example: site/{siteId}/coord/{coordId}/customize
    // Will be implemented based on MQTT topic structure
  }

  /**
   * Save coordinator/node configuration
   */
  async saveConfig(): Promise<void> {
    const type = this.selectedDeviceType();
    const id = this.selectedDeviceId();
    
    if (!type || !id) return;

    this.saving.set(true);
    this.errorMessage.set('');
    this.saveSuccess.set(false);

    try {
      const config: any = {};

      if (type === 'coordinator') {
        config.presence_debounce_ms = this.presenceDebounceMs();
        config.occupancy_hold_ms = this.occupancyHoldMs();
        config.fade_in_ms = this.fadeInMs();
        config.fade_out_ms = this.fadeOutMs();
        config.pairing_window_s = this.pairingWindowS();
      } else if (type === 'node') {
        config.pwm_freq_hz = this.pwmFreqHz();
        config.pwm_res_bits = this.pwmResBits();
        config.telemetry_s = this.telemetryS();
        config.rx_window_ms = this.rxWindowMs();
        config.rx_period_ms = this.rxPeriodMs();
        config.derate_start_c = this.derateStartC();
        config.derate_min_duty_pct = this.derateMinDutyPct();
        config.retry_count = this.retryCount();
        config.cmd_ttl_ms = this.cmdTtlMs();
      }

      const siteId = this.dataService.activeSiteId() || 'site001';
      
      await firstValueFrom(
        this.apiService.put(`/api/v1/${type}/${id}/customize/config`, { siteId, config })
      );

      // Publish to MQTT for real-time update
      this.publishConfigToMqtt('config', config);

      this.saveSuccess.set(true);
      setTimeout(() => this.saveSuccess.set(false), 3000);
    } catch (error) {
      console.error('[CustomizeComponent] Failed to save configuration:', error);
      this.errorMessage.set('Failed to save configuration');
    } finally {
      this.saving.set(false);
    }
  }

  /**
   * Save LED configuration (nodes only)
   */
  async saveLedConfig(): Promise<void> {
    const type = this.selectedDeviceType();
    const id = this.selectedDeviceId();
    
    if (!type || !id || type !== 'node') return;

    this.saving.set(true);
    this.errorMessage.set('');
    this.saveSuccess.set(false);

    try {
      const config = {
        enabled: this.ledEnabled(),
        brightness: this.ledBrightness(),
        color: this.ledColor()
      };

      const siteId = this.dataService.activeSiteId() || 'site001';
      
      await firstValueFrom(
        this.apiService.put(`/api/v1/${type}/${id}/customize/led`, { siteId, config })
      );

      // Publish to MQTT for real-time update
      this.publishConfigToMqtt('led', config);

      this.saveSuccess.set(true);
      setTimeout(() => this.saveSuccess.set(false), 3000);
    } catch (error) {
      console.error('[CustomizeComponent] Failed to save LED configuration:', error);
      this.errorMessage.set('Failed to save LED configuration');
    } finally {
      this.saving.set(false);
    }
  }

  /**
   * Publish configuration to MQTT for real-time synchronization
   */
  private publishConfigToMqtt(section: string, config: any): void {
    const type = this.selectedDeviceType();
    const id = this.selectedDeviceId();
    const siteId = this.dataService.activeSiteId() || 'site001';
    
    if (!type || !id) return;

    // Publish to device-specific customization topic
    const topic = `site/${siteId}/${type}/${id}/customize/${section}`;
    const payload = JSON.stringify(config);
    
    this.mqttService.publish(topic, payload);
  }

  /**
   * Reset configuration to factory defaults
   */
  async resetToDefaults(): Promise<void> {
    const section = this.activeSection();
    const confirmed = confirm(`Reset ${section} configuration to factory defaults?`);
    
    if (!confirmed) return;

    const type = this.selectedDeviceType();
    const id = this.selectedDeviceId();
    
    if (!type || !id) return;

    this.loading.set(true);
    this.errorMessage.set('');

    try {
      const siteId = this.dataService.activeSiteId() || 'site001';
      
      await firstValueFrom(
        this.apiService.post(`/api/v1/${type}/${id}/customize/reset`, { 
          siteId, 
          section 
        })
      );

      // Reload configuration
      await this.loadDeviceConfiguration();

      this.saveSuccess.set(true);
      setTimeout(() => this.saveSuccess.set(false), 3000);
    } catch (error) {
      console.error('[CustomizeComponent] Failed to reset configuration:', error);
      this.errorMessage.set('Failed to reset configuration');
    } finally {
      this.loading.set(false);
    }
  }

  /**
   * Test LED preview
   */
  async testLedPreview(): Promise<void> {
    const type = this.selectedDeviceType();
    const id = this.selectedDeviceId();
    const siteId = this.dataService.activeSiteId() || 'site001';
    
    if (!type || !id) return;

    try {
      await firstValueFrom(
        this.apiService.post(`/api/v1/${type}/${id}/led/preview`, {
          siteId,
          color: this.ledColor(),
          brightness: this.ledBrightness(),
          duration: 5000 // 5 second preview
        })
      );
    } catch (error) {
      console.error('[CustomizeComponent] LED preview failed:', error);
    }
  }
}
