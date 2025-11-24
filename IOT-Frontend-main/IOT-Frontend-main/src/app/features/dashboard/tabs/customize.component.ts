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
  styleUrls: ['./customize.component.scss']
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

  // === HLK-LD2450 mmWave Radar Configuration ===
  radarEnabled = signal(true);
  radarMode = signal<'standard' | 'energy-saving' | 'multi-target'>('standard');
  radarMaxDistance = signal(600); // cm (0-600)
  radarMinDistance = signal(0); // cm
  radarFieldOfView = signal(120); // degrees (60-150)
  radarSensitivity = signal(50); // 0-100
  radarReportRate = signal(10); // Hz (1-20)
  radarFilterEnabled = signal(true);
  radarStaticDetection = signal(true);
  radarMovingDetection = signal(true);
  radarZone1Enabled = signal(true);
  radarZone1MinX = signal(-150); // mm
  radarZone1MaxX = signal(150); // mm
  radarZone1MinY = signal(0); // mm
  radarZone1MaxY = signal(600); // mm
  radarZone2Enabled = signal(false);
  radarZone2MinX = signal(-300); // mm
  radarZone2MaxX = signal(-150); // mm
  radarZone2MinY = signal(0); // mm
  radarZone2MaxY = signal(600); // mm
  radarZone3Enabled = signal(false);
  radarZone3MinX = signal(150); // mm
  radarZone3MaxX = signal(300); // mm
  radarZone3MinY = signal(0); // mm
  radarZone3MaxY = signal(600); // mm

  // === TSL2561 Light Sensor Configuration ===
  lightSensorEnabled = signal(true);
  lightIntegrationTime = signal<13 | 101 | 402>(101); // ms
  lightGain = signal<1 | 16>(1); // 1x or 16x
  lightAutoRange = signal(true);
  lightLowThreshold = signal(10); // lux
  lightHighThreshold = signal(1000); // lux
  lightSamplingInterval = signal(1000); // ms
  lightCalibrationFactor = signal(1.0);
  lightPackageType = signal<'CS' | 'T-FN-CL'>('CS'); // CS or T-FN-CL package

  // === SK6812B LED Configuration ===
  ledEnabled = signal(true);
  ledCount = signal(60); // Total LEDs per strip
  ledBrightness = signal(80); // 0-100%
  ledColor = signal('#00FFBF'); // Default color
  ledEffect = signal<'solid' | 'fade' | 'pulse' | 'rainbow' | 'chase' | 'twinkle'>('solid');
  ledEffectSpeed = signal(50); // 0-100
  ledSegments = signal(1); // Number of independent segments
  ledSegment1Start = signal(0);
  ledSegment1End = signal(20);
  ledSegment1Color = signal('#00FFBF');
  ledSegment2Start = signal(20);
  ledSegment2End = signal(40);
  ledSegment2Color = signal('#FF00FF');
  ledSegment3Start = signal(40);
  ledSegment3End = signal(60);
  ledSegment3Color = signal('#FFFF00');
  ledPowerLimit = signal(30); // Watts
  ledMaxCurrent = signal(1000); // mA per LED
  ledTemperatureCompensation = signal(true);
  ledGammaCorrection = signal(2.2);
  ledColorOrder = signal<'GRB' | 'RGB' | 'RGBW'>('GRB'); // SK6812B uses GRB

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
    // Radar configuration
    if (config.radar) {
      this.radarEnabled.set(config.radar.enabled ?? true);
      this.radarMode.set(config.radar.mode ?? 'standard');
      this.radarMaxDistance.set(config.radar.maxDistance ?? 600);
      this.radarMinDistance.set(config.radar.minDistance ?? 0);
      this.radarFieldOfView.set(config.radar.fieldOfView ?? 120);
      this.radarSensitivity.set(config.radar.sensitivity ?? 50);
      this.radarReportRate.set(config.radar.reportRate ?? 10);
      this.radarFilterEnabled.set(config.radar.filterEnabled ?? true);
      this.radarStaticDetection.set(config.radar.staticDetection ?? true);
      this.radarMovingDetection.set(config.radar.movingDetection ?? true);

      if (config.radar.zones) {
        const zone1 = config.radar.zones[0];
        if (zone1) {
          this.radarZone1Enabled.set(zone1.enabled ?? true);
          this.radarZone1MinX.set(zone1.minX ?? -150);
          this.radarZone1MaxX.set(zone1.maxX ?? 150);
          this.radarZone1MinY.set(zone1.minY ?? 0);
          this.radarZone1MaxY.set(zone1.maxY ?? 600);
        }
        const zone2 = config.radar.zones[1];
        if (zone2) {
          this.radarZone2Enabled.set(zone2.enabled ?? false);
          this.radarZone2MinX.set(zone2.minX ?? -300);
          this.radarZone2MaxX.set(zone2.maxX ?? -150);
          this.radarZone2MinY.set(zone2.minY ?? 0);
          this.radarZone2MaxY.set(zone2.maxY ?? 600);
        }
        const zone3 = config.radar.zones[2];
        if (zone3) {
          this.radarZone3Enabled.set(zone3.enabled ?? false);
          this.radarZone3MinX.set(zone3.minX ?? 150);
          this.radarZone3MaxX.set(zone3.maxX ?? 300);
          this.radarZone3MinY.set(zone3.minY ?? 0);
          this.radarZone3MaxY.set(zone3.maxY ?? 600);
        }
      }
    }

    // Light sensor configuration
    if (config.light) {
      this.lightSensorEnabled.set(config.light.enabled ?? true);
      this.lightIntegrationTime.set(config.light.integrationTime ?? 101);
      this.lightGain.set(config.light.gain ?? 1);
      this.lightAutoRange.set(config.light.autoRange ?? true);
      this.lightLowThreshold.set(config.light.lowThreshold ?? 10);
      this.lightHighThreshold.set(config.light.highThreshold ?? 1000);
      this.lightSamplingInterval.set(config.light.samplingInterval ?? 1000);
      this.lightCalibrationFactor.set(config.light.calibrationFactor ?? 1.0);
      this.lightPackageType.set(config.light.packageType ?? 'CS');
    }

    // LED configuration
    if (config.led) {
      this.ledEnabled.set(config.led.enabled ?? true);
      this.ledCount.set(config.led.count ?? 60);
      this.ledBrightness.set(config.led.brightness ?? 80);
      this.ledColor.set(config.led.color ?? '#00FFBF');
      this.ledEffect.set(config.led.effect ?? 'solid');
      this.ledEffectSpeed.set(config.led.effectSpeed ?? 50);
      this.ledSegments.set(config.led.segments ?? 1);
      
      if (config.led.segmentConfig) {
        const seg1 = config.led.segmentConfig[0];
        if (seg1) {
          this.ledSegment1Start.set(seg1.start ?? 0);
          this.ledSegment1End.set(seg1.end ?? 20);
          this.ledSegment1Color.set(seg1.color ?? '#00FFBF');
        }
        const seg2 = config.led.segmentConfig[1];
        if (seg2) {
          this.ledSegment2Start.set(seg2.start ?? 20);
          this.ledSegment2End.set(seg2.end ?? 40);
          this.ledSegment2Color.set(seg2.color ?? '#FF00FF');
        }
        const seg3 = config.led.segmentConfig[2];
        if (seg3) {
          this.ledSegment3Start.set(seg3.start ?? 40);
          this.ledSegment3End.set(seg3.end ?? 60);
          this.ledSegment3Color.set(seg3.color ?? '#FFFF00');
        }
      }

      this.ledPowerLimit.set(config.led.powerLimit ?? 30);
      this.ledMaxCurrent.set(config.led.maxCurrent ?? 1000);
      this.ledTemperatureCompensation.set(config.led.temperatureCompensation ?? true);
      this.ledGammaCorrection.set(config.led.gammaCorrection ?? 2.2);
      this.ledColorOrder.set(config.led.colorOrder ?? 'GRB');
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
   * Save radar configuration
   */
  async saveRadarConfig(): Promise<void> {
    const type = this.selectedDeviceType();
    const id = this.selectedDeviceId();
    
    if (!type || !id) return;

    this.saving.set(true);
    this.errorMessage.set('');
    this.saveSuccess.set(false);

    try {
      const config = {
        enabled: this.radarEnabled(),
        mode: this.radarMode(),
        maxDistance: this.radarMaxDistance(),
        minDistance: this.radarMinDistance(),
        fieldOfView: this.radarFieldOfView(),
        sensitivity: this.radarSensitivity(),
        reportRate: this.radarReportRate(),
        filterEnabled: this.radarFilterEnabled(),
        staticDetection: this.radarStaticDetection(),
        movingDetection: this.radarMovingDetection(),
        zones: [
          {
            enabled: this.radarZone1Enabled(),
            minX: this.radarZone1MinX(),
            maxX: this.radarZone1MaxX(),
            minY: this.radarZone1MinY(),
            maxY: this.radarZone1MaxY()
          },
          {
            enabled: this.radarZone2Enabled(),
            minX: this.radarZone2MinX(),
            maxX: this.radarZone2MaxX(),
            minY: this.radarZone2MinY(),
            maxY: this.radarZone2MaxY()
          },
          {
            enabled: this.radarZone3Enabled(),
            minX: this.radarZone3MinX(),
            maxX: this.radarZone3MaxX(),
            minY: this.radarZone3MinY(),
            maxY: this.radarZone3MaxY()
          }
        ]
      };

      const siteId = this.dataService.activeSiteId() || 'site001';
      
      await firstValueFrom(
        this.apiService.put(`/api/v1/${type}/${id}/customize/radar`, { siteId, config })
      );

      // Publish to MQTT for real-time update
      this.publishConfigToMqtt('radar', config);

      this.saveSuccess.set(true);
      setTimeout(() => this.saveSuccess.set(false), 3000);
    } catch (error) {
      console.error('[CustomizeComponent] Failed to save radar configuration:', error);
      this.errorMessage.set('Failed to save radar configuration');
    } finally {
      this.saving.set(false);
    }
  }

  /**
   * Save light sensor configuration
   */
  async saveLightConfig(): Promise<void> {
    const type = this.selectedDeviceType();
    const id = this.selectedDeviceId();
    
    if (!type || !id) return;

    this.saving.set(true);
    this.errorMessage.set('');
    this.saveSuccess.set(false);

    try {
      const config = {
        enabled: this.lightSensorEnabled(),
        integrationTime: this.lightIntegrationTime(),
        gain: this.lightGain(),
        autoRange: this.lightAutoRange(),
        lowThreshold: this.lightLowThreshold(),
        highThreshold: this.lightHighThreshold(),
        samplingInterval: this.lightSamplingInterval(),
        calibrationFactor: this.lightCalibrationFactor(),
        packageType: this.lightPackageType()
      };

      const siteId = this.dataService.activeSiteId() || 'site001';
      
      await firstValueFrom(
        this.apiService.put(`/api/v1/${type}/${id}/customize/light`, { siteId, config })
      );

      // Publish to MQTT for real-time update
      this.publishConfigToMqtt('light', config);

      this.saveSuccess.set(true);
      setTimeout(() => this.saveSuccess.set(false), 3000);
    } catch (error) {
      console.error('[CustomizeComponent] Failed to save light sensor configuration:', error);
      this.errorMessage.set('Failed to save light sensor configuration');
    } finally {
      this.saving.set(false);
    }
  }

  /**
   * Save LED configuration
   */
  async saveLedConfig(): Promise<void> {
    const type = this.selectedDeviceType();
    const id = this.selectedDeviceId();
    
    if (!type || !id) return;

    this.saving.set(true);
    this.errorMessage.set('');
    this.saveSuccess.set(false);

    try {
      const config = {
        enabled: this.ledEnabled(),
        count: this.ledCount(),
        brightness: this.ledBrightness(),
        color: this.ledColor(),
        effect: this.ledEffect(),
        effectSpeed: this.ledEffectSpeed(),
        segments: this.ledSegments(),
        segmentConfig: [
          {
            start: this.ledSegment1Start(),
            end: this.ledSegment1End(),
            color: this.ledSegment1Color()
          },
          {
            start: this.ledSegment2Start(),
            end: this.ledSegment2End(),
            color: this.ledSegment2Color()
          },
          {
            start: this.ledSegment3Start(),
            end: this.ledSegment3End(),
            color: this.ledSegment3Color()
          }
        ],
        powerLimit: this.ledPowerLimit(),
        maxCurrent: this.ledMaxCurrent(),
        temperatureCompensation: this.ledTemperatureCompensation(),
        gammaCorrection: this.ledGammaCorrection(),
        colorOrder: this.ledColorOrder()
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
          effect: this.ledEffect(),
          duration: 5000 // 5 second preview
        })
      );
    } catch (error) {
      console.error('[CustomizeComponent] LED preview failed:', error);
    }
  }

  /**
   * Helper method to set light integration time with proper type
   */
  setLightIntegrationTime(value: number): void {
    if (value === 13 || value === 101 || value === 402) {
      this.lightIntegrationTime.set(value as 13 | 101 | 402);
    }
  }

  /**
   * Helper method to set light gain with proper type
   */
  setLightGain(value: number): void {
    if (value === 1 || value === 16) {
      this.lightGain.set(value as 1 | 16);
    }
  }
}
