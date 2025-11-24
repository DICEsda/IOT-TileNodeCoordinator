import { CommonModule } from '@angular/common';
import { Component, OnDestroy, OnInit, signal, inject, computed } from '@angular/core';
import { DataService } from '../../core/services/data.service';
import { LightMonitorComponent } from './components/light-monitor/light-monitor.component';
import { RoomVisualizerComponent } from './components/room-visualizer/room-visualizer.component';
import { CalibrateComponent } from './tabs/calibrate/calibrate.component';
import { DevicesComponent } from './tabs/devices/devices.component';
import { LogsComponent } from './tabs/logs/logs.component';
import { SettingsNewComponent } from './tabs/settings/settings-new.component';
import { CustomizeComponent } from './tabs/customize.component';
import { Node } from '../../core/models/api.models';

import { NetworkGraphComponent } from './components/network-graph.component';

interface LightNodeState {
  nodeId: string; // Changed to string to match Node ID
  totalBulbs: number;
  activeBulbs: number;
}

@Component({
  selector: 'app-dashboard',
  imports: [
    CommonModule,
    RoomVisualizerComponent,
    LightMonitorComponent,
    NetworkGraphComponent,
    LogsComponent,
    DevicesComponent,
    SettingsNewComponent,
    CustomizeComponent,
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
  
  // Computed signal for registered nodes array
  registeredNodes = computed(() => Array.from(this.data.nodes().values()));
  registeredCoordinators = computed(() => Array.from(this.data.coordinators().values()));

  private timeInterval: any;

  ngOnInit() {
    this.updateTime();
    this.timeInterval = setInterval(() => {
      this.updateTime();
    }, 1000);
    
    // Load initial data
    this.data.loadSites().then(() => {
      const sites = this.data.sites();
      if (sites.length > 0) {
        this.data.loadSite(sites[0]._id);
      } else {
        // Fallback for fresh install: use default site ID
        console.warn('[Dashboard] No sites found, defaulting to site001');
        this.data.loadSite('site001');
      }
    });
  }

  ngOnDestroy() {
    if (this.timeInterval) {
      clearInterval(this.timeInterval);
    }
  }

  setActiveTab(tab: string) {
    this.activeTab.set(tab);
  }

  onLightStateChanged(state: { totalActive: number; nodeStates: any[] }) {
    this.totalActiveLights.set(state.totalActive);
    
    // Map the numeric IDs from visualizer to real node IDs if available
    const nodes = this.registeredNodes();
    const mappedStates: LightNodeState[] = state.nodeStates.map((s, index) => {
      // If we have a real node at this index, use its ID
      // Otherwise use the numeric ID as string or a placeholder
      const realNodeId = nodes[index]?.node_id || `node-${s.nodeId}`;
      return {
        nodeId: realNodeId,
        totalBulbs: s.totalBulbs,
        activeBulbs: s.activeBulbs
      };
    });

    this.lightNodeStates.set(mappedStates);
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

