import { CommonModule } from '@angular/common';
import { Component, signal } from '@angular/core';
import { FormsModule } from '@angular/forms';

@Component({
  selector: 'app-settings',
  imports: [CommonModule, FormsModule],
  templateUrl: './settings.component.html',
  styleUrl: './settings.component.scss'
})
export class SettingsComponent {
  // System Settings
  autoMode = signal(true);
  motionSensitivity = signal(75);
  lightIntensity = signal(100);
  delayBeforeOff = signal(30);

  // Notification Settings
  emailNotifications = signal(true);
  pushNotifications = signal(false);
  batteryAlerts = signal(true);
  motionAlerts = signal(false);

  // Advanced Settings
  dataLogging = signal(true);
  cloudSync = signal(true);
  energySavingMode = signal(false);
  debugMode = signal(false);

  saveSettings() {
    console.log('Settings saved:', {
      autoMode: this.autoMode(),
      motionSensitivity: this.motionSensitivity(),
      lightIntensity: this.lightIntensity(),
      delayBeforeOff: this.delayBeforeOff(),
      emailNotifications: this.emailNotifications(),
      pushNotifications: this.pushNotifications(),
      batteryAlerts: this.batteryAlerts(),
      motionAlerts: this.motionAlerts(),
      dataLogging: this.dataLogging(),
      cloudSync: this.cloudSync(),
      energySavingMode: this.energySavingMode(),
      debugMode: this.debugMode()
    });

    // Show success message
    alert('Settings saved successfully!');
  }

  resetToDefaults() {
    if (confirm('Are you sure you want to reset all settings to default values?')) {
      this.autoMode.set(true);
      this.motionSensitivity.set(75);
      this.lightIntensity.set(100);
      this.delayBeforeOff.set(30);
      this.emailNotifications.set(true);
      this.pushNotifications.set(false);
      this.batteryAlerts.set(true);
      this.motionAlerts.set(false);
      this.dataLogging.set(true);
      this.cloudSync.set(true);
      this.energySavingMode.set(false);
      this.debugMode.set(false);
    }
  }
}

