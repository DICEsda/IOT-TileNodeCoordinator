import { CommonModule } from '@angular/common';
import { Component, OnDestroy, OnInit, signal, inject } from '@angular/core';
import { DataService } from '../../core/services/data.service';
import { LightMonitorComponent } from './components/light-monitor/light-monitor.component';
import { RoomVisualizerComponent } from './components/room-visualizer/room-visualizer.component';
import { CalibrateComponent } from './tabs/calibrate/calibrate.component';
import { DevicesComponent } from './tabs/devices/devices.component';
import { LogsComponent } from './tabs/logs/logs.component';
import { SettingsComponent } from './tabs/settings/settings.component';

interface LightNodeState {
  nodeId: number;
  totalBulbs: number;
  activeBulbs: number;
}

@Component({
  selector: 'app-dashboard',
  imports: [
    CommonModule,
    RoomVisualizerComponent,
    LightMonitorComponent,
    LogsComponent,
    DevicesComponent,
    SettingsComponent,
    CalibrateComponent
  ],
  templateUrl: './dashboard.component.html',
  styleUrl: './dashboard.component.scss'
})
export class DashboardComponent implements OnInit, OnDestroy {
  // Inject DataService to reflect real connection state
  data = inject(DataService);
  activeTab = signal<string>('monitor');
  currentTime = signal<string>('');
  totalActiveLights = signal<number>(0);
  lightNodeStates = signal<LightNodeState[]>([]);
  private timeInterval: any;

  ngOnInit() {
    this.updateTime();
    this.timeInterval = setInterval(() => {
      this.updateTime();
    }, 1000);
  }

  ngOnDestroy() {
    if (this.timeInterval) {
      clearInterval(this.timeInterval);
    }
  }

  setActiveTab(tab: string) {
    this.activeTab.set(tab);
  }

  onLightStateChanged(state: { totalActive: number; nodeStates: LightNodeState[] }) {
    this.totalActiveLights.set(state.totalActive);
    this.lightNodeStates.set(state.nodeStates);
  }

  private updateTime() {
    const now = new Date();
    this.currentTime.set(now.toLocaleString('en-DK', {
      timeZone: 'Europe/Copenhagen',
      month: 'short',
      day: 'numeric',
      year: 'numeric',
      hour: '2-digit',
      minute: '2-digit',
      second: '2-digit',
      hour12: false
    }));
  }

  getCurrentTime(): string {
    return this.currentTime();
  }

  isHealthy(): boolean {
    return this.data.getSystemHealth().overall;
  }
}

