import { CommonModule } from '@angular/common';
import { Component, OnInit, effect, inject, signal } from '@angular/core';
import { Coordinator, Node } from '../../../../core/models/api.models';
import { DataService } from '../../../../core/services/data.service';

type DeviceStatus = 'online' | 'offline' | 'warning' | 'error' | 'pairing';

interface SensorRow {
  label: string;
  value: string | null;
}

@Component({
  selector: 'app-devices',
  imports: [CommonModule],
  templateUrl: './devices.component.html',
  styleUrl: './devices.component.scss'
})
export class DevicesComponent implements OnInit {
  private readonly data = inject(DataService);

  loading = signal<boolean>(true);
  coordinator = signal<Coordinator | null>(null);
  nodes = signal<Node[]>([]);

  constructor() {
    effect(() => {
      const coordinators = Array.from(this.data.coordinators().values());
      this.coordinator.set(coordinators.length ? coordinators[0] : null);
    });

    effect(() => {
      const nodeList = Array.from(this.data.nodes().values());
      this.nodes.set(nodeList);
    });
  }

  async ngOnInit(): Promise<void> {
    try {
      if (!this.data.activeSiteId()) {
        if (this.data.sites().length === 0) {
          await this.data.loadSites();
        }
        const defaultSite = this.data.sites()[0];
        if (defaultSite) {
          await this.data.loadSite(defaultSite._id);
        }
      } else {
        await this.data.loadSite(this.data.activeSiteId()!);
      }
    } catch (error) {
      console.error('[DevicesComponent] failed to initialize site data', error);
    } finally {
      this.loading.set(false);
    }
  }

  getNodesList(): Node[] {
    const byStatus = (node: Node): number => {
      const statusPriority: Record<DeviceStatus, number> = {
        online: 0,
        pairing: 1,
        warning: 2,
        error: 3,
        offline: 4
      } as const;
      return statusPriority[(node.status as DeviceStatus) ?? 'offline'] ?? 4;
    };

    return [...this.nodes()].sort((a, b) => {
      const statusDiff = byStatus(a) - byStatus(b);
      if (statusDiff !== 0) return statusDiff;
      return (a.node_id || '').localeCompare(b.node_id || '');
    });
  }

  get onlineDevicesCount(): number {
    return this.nodes().filter((node: Node) => node.status === 'online').length;
  }

  get warningDevicesCount(): number {
    return this.nodes().filter((node: Node) => this.isWarningNode(node)).length;
  }

  get offlineDevicesCount(): number {
    return this.nodes().filter((node: Node) => node.status === 'offline').length;
  }

  private isWarningNode(node: Node): boolean {
    const battery = this.getBatteryPercent(node);
    if (battery !== null && battery <= 30) {
      return true;
    }
    return node.status === 'error' || node.status === 'pairing';
  }

  coordinatorStatus(): DeviceStatus {
    return (this.coordinator()?.status as DeviceStatus) ?? 'offline';
  }

  getConnectionSubtitle(): string {
    return this.coordinatorStatus() === 'online'
      ? 'Paired & connected to nodes'
      : 'Link offline';
  }

  formatLastSeen(value?: Date | string): string {
    if (!value) return 'No data';
    const date = typeof value === 'string' ? new Date(value) : value;
    if (Number.isNaN(date.getTime())) return 'No data';

    const diffMinutes = Math.floor((Date.now() - date.getTime()) / 60000);
    if (diffMinutes < 1) return 'Just now';
    if (diffMinutes === 1) return '1 minute ago';
    if (diffMinutes < 60) return `${diffMinutes} minutes ago`;

    const hours = Math.floor(diffMinutes / 60);
    if (hours === 1) return '1 hour ago';
    if (hours < 24) return `${hours} hours ago`;

    const days = Math.floor(hours / 24);
    return days === 1 ? '1 day ago' : `${days} days ago`;
  }

  getBatteryPercent(node: Node): number | null {
    if (typeof node.battery_percent === 'number') {
      return Math.max(0, Math.min(100, node.battery_percent));
    }
    return null;
  }

  getBatteryClass(percent: number | null): string {
    if (percent === null) return 'empty';
    if (percent > 70) return 'good';
    if (percent > 30) return 'medium';
    return 'low';
  }

  getSensorRows(node: Node): SensorRow[] {
    return [
      {
        label: 'Temperature',
        value: typeof node.temperature === 'number' ? `${node.temperature.toFixed(1)}°C` : null
      },
      {
        label: 'Battery Voltage',
        value: typeof node.battery_voltage === 'number' ? `${node.battery_voltage.toFixed(2)}V` : null
      },
      {
        label: 'Zone',
        value: node.zone_id ?? null
      }
    ];
  }

  formatUptime(seconds?: number): string {
    if (!seconds || seconds <= 0) return 'No data';
    const hours = Math.floor(seconds / 3600);
    const minutes = Math.floor((seconds % 3600) / 60);
    return `${hours}h ${minutes}m`;
  }
}

