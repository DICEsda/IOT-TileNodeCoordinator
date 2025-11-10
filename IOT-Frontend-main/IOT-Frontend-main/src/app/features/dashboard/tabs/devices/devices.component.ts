import { CommonModule } from '@angular/common';
import { Component, signal } from '@angular/core';

interface Sensor {
  id: string;
  type: string;
  value: string;
  status: 'active' | 'inactive';
}

interface Device {
  id: number;
  name: string;
  location: string;
  status: 'online' | 'offline' | 'warning';
  battery: number;
  lastSeen: Date;
  sensors: Sensor[];
  activeLights: number;
  totalLights: number;
}

@Component({
  selector: 'app-devices',
  imports: [CommonModule],
  templateUrl: './devices.component.html',
  styleUrl: './devices.component.scss'
})
export class DevicesComponent {
  devices = signal<Device[]>([
    {
      id: 1,
      name: 'Node 01 - Front Left',
      location: 'Front Left Corner',
      status: 'online',
      battery: 87,
      lastSeen: new Date(),
      sensors: [
        { id: 'motion-1', type: 'Motion Sensor', value: 'Active', status: 'active' },
        { id: 'light-1', type: 'Light Sensor', value: '450 lux', status: 'active' },
        { id: 'temp-1', type: 'Temperature', value: '22°C', status: 'active' }
      ],
      activeLights: 4,
      totalLights: 6
    },
    {
      id: 2,
      name: 'Node 02 - Front Center',
      location: 'Front Center',
      status: 'online',
      battery: 92,
      lastSeen: new Date(),
      sensors: [
        { id: 'motion-2', type: 'Motion Sensor', value: 'Inactive', status: 'inactive' },
        { id: 'light-2', type: 'Light Sensor', value: '320 lux', status: 'active' },
        { id: 'temp-2', type: 'Temperature', value: '21°C', status: 'active' }
      ],
      activeLights: 0,
      totalLights: 6
    },
    {
      id: 3,
      name: 'Node 03 - Front Right',
      location: 'Front Right Corner',
      status: 'online',
      battery: 78,
      lastSeen: new Date(),
      sensors: [
        { id: 'motion-3', type: 'Motion Sensor', value: 'Active', status: 'active' },
        { id: 'light-3', type: 'Light Sensor', value: '280 lux', status: 'active' },
        { id: 'temp-3', type: 'Temperature', value: '23°C', status: 'active' }
      ],
      activeLights: 6,
      totalLights: 6
    },
    {
      id: 4,
      name: 'Node 04 - Back Left',
      location: 'Back Left Corner',
      status: 'warning',
      battery: 15,
      lastSeen: new Date(Date.now() - 1000 * 60 * 5),
      sensors: [
        { id: 'motion-4', type: 'Motion Sensor', value: 'Inactive', status: 'inactive' },
        { id: 'light-4', type: 'Light Sensor', value: '150 lux', status: 'active' },
        { id: 'temp-4', type: 'Temperature', value: '20°C', status: 'active' }
      ],
      activeLights: 2,
      totalLights: 6
    },
    {
      id: 5,
      name: 'Node 05 - Back Center',
      location: 'Back Center',
      status: 'online',
      battery: 95,
      lastSeen: new Date(),
      sensors: [
        { id: 'motion-5', type: 'Motion Sensor', value: 'Active', status: 'active' },
        { id: 'light-5', type: 'Light Sensor', value: '410 lux', status: 'active' },
        { id: 'temp-5', type: 'Temperature', value: '22°C', status: 'active' }
      ],
      activeLights: 5,
      totalLights: 6
    },
    {
      id: 6,
      name: 'Node 06 - Back Right',
      location: 'Back Right Corner',
      status: 'online',
      battery: 68,
      lastSeen: new Date(),
      sensors: [
        { id: 'motion-6', type: 'Motion Sensor', value: 'Inactive', status: 'inactive' },
        { id: 'light-6', type: 'Light Sensor', value: '200 lux', status: 'active' },
        { id: 'temp-6', type: 'Temperature', value: '21°C', status: 'active' }
      ],
      activeLights: 1,
      totalLights: 6
    }
  ]);

  get onlineDevicesCount(): number {
    return this.devices().filter(d => d.status === 'online').length;
  }

  get warningDevicesCount(): number {
    return this.devices().filter(d => d.status === 'warning').length;
  }

  get offlineDevicesCount(): number {
    return this.devices().filter(d => d.status === 'offline').length;
  }

  formatLastSeen(date: Date): string {
    const now = new Date();
    const diff = now.getTime() - date.getTime();
    const minutes = Math.floor(diff / 60000);

    if (minutes < 1) return 'Just now';
    if (minutes === 1) return '1 minute ago';
    if (minutes < 60) return `${minutes} minutes ago`;

    const hours = Math.floor(minutes / 60);
    if (hours === 1) return '1 hour ago';
    return `${hours} hours ago`;
  }

  getBatteryClass(battery: number): string {
    if (battery > 70) return 'good';
    if (battery > 30) return 'medium';
    return 'low';
  }
}

