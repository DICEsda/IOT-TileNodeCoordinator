import { CommonModule } from '@angular/common';
import { Component, Input, OnChanges, OnInit, OnDestroy, ViewChild, ElementRef, AfterViewInit, signal, SimpleChanges, inject, effect } from '@angular/core';
import { Node } from '../../../../core/models/api.models';
import { DataService } from '../../../../core/services/data.service';
import { Chart, registerables } from 'chart.js';

Chart.register(...registerables);

interface Bulb {
  id: number;
  isOn: boolean;
  r: number;  // Red value 0-255
  g: number;  // Green value 0-255
  b: number;  // Blue value 0-255
  w: number;  // White value 0-255
}

interface LightNode {
  id: string; // Changed to string to match Node.node_id
  name: string;
  bulbs: Bulb[];
  totalOn: number;
  status: 'active' | 'idle' | 'offline';
  tempC?: number; // Temperature in Celsius
}

interface LightNodeState {
  nodeId: string; // Changed to string
  totalBulbs: number;
  activeBulbs: number;
}

interface SensorDataPoint {
  timestamp: Date;
  temperature: number;
  lux: number;
}

@Component({
  selector: 'app-light-monitor',
  standalone: true,
  imports: [CommonModule],
  templateUrl: './light-monitor.component.html',
  styleUrl: './light-monitor.component.scss'
})
export class LightMonitorComponent implements OnChanges, AfterViewInit, OnDestroy {
  @ViewChild('sensorChart') chartCanvas!: ElementRef<HTMLCanvasElement>;
  
  private dataService = inject(DataService);
  private chart?: Chart;
  private sensorData: SensorDataPoint[] = [];
  private readonly MAX_DATA_POINTS = 30;
  private pollInterval?: any;
  @Input() totalActiveLights: number = 0;
  @Input() lightNodeStates: LightNodeState[] = [];
  @Input() registeredNodes: Node[] = []; // New input for real nodes

  nodes = signal<LightNode[]>([]);
  totalLights = signal(0);
  activeLights = signal(0);
  
  // Track manual overrides to prevent telemetry from resetting state
  private manualOverrideUntil: Map<string, number> = new Map();
  private readonly OVERRIDE_DURATION_MS = 5000; // 5 seconds

  constructor() {
    // Watch for node temperature changes from DataService
    effect(() => {
      const dataServiceNodes = this.dataService.nodes();
      if (dataServiceNodes.size === 0) return;

      // Update node temperatures in real-time
      const updatedNodes = this.nodes().map(node => {
        const liveNode = dataServiceNodes.get(node.id);
        if (liveNode && liveNode.temperature !== undefined) {
          return { ...node, tempC: liveNode.temperature };
        }
        return node;
      });

      // Only update if temperatures actually changed
      const hasChanges = updatedNodes.some((node, i) => 
        node.tempC !== this.nodes()[i]?.tempC
      );
      
      if (hasChanges) {
        this.nodes.set(updatedNodes);
      }
    });
  }

  get activeNodesCount(): number {
    return this.nodes().filter(n => n.status === 'active').length;
  }

  ngOnChanges(changes: SimpleChanges): void {
    if (changes['registeredNodes']) {
      this.initializeNodes();
    }
    
    if (changes['lightNodeStates'] || changes['totalActiveLights']) {
      this.updateFromLightNodeStates();
    }
  }

  private initializeNodes(): void {
    const PIXELS_PER_NODE = 4; // SK6812B pixels per node
    
    // Preserve current bulb states if nodes already exist
    const existingNodes = this.nodes();
    const existingBulbStates = new Map<string, Map<number, boolean>>();
    existingNodes.forEach(node => {
      const bulbMap = new Map<number, boolean>();
      node.bulbs.forEach(bulb => bulbMap.set(bulb.id, bulb.isOn));
      existingBulbStates.set(node.id, bulbMap);
    });

    let initialNodes: LightNode[] = [];
    
    if (this.registeredNodes && this.registeredNodes.length > 0) {
      // Use registered nodes
      initialNodes = this.registeredNodes.map(node => {
        const existingBulbs = existingBulbStates.get(node.node_id);
        return {
          id: node.node_id,
          name: node.name || `Node ${node.node_id.substring(0, 4)}`,
          bulbs: Array.from({ length: PIXELS_PER_NODE }, (_, i) => ({ 
            id: i + 1, 
            // Preserve existing bulb state if available
            isOn: existingBulbs?.get(i + 1) ?? false,
            r: node.rgbw?.r || 0,
            g: node.rgbw?.g || 0,
            b: node.rgbw?.b || 0,
            w: node.rgbw?.w || 0
          })),
          totalOn: existingBulbs ? Array.from(existingBulbs.values()).filter(v => v).length : 0,
          status: node.status === 'online' ? 'active' : 'offline',
          tempC: node.temperature
        };
      });
    } else {
      // Fallback to single node for development if no nodes registered
      initialNodes = [{
        id: 'node-default',
        name: 'Single Node (Dev)',
        bulbs: Array.from({ length: PIXELS_PER_NODE }, (_, i) => ({ 
          id: i + 1, 
          isOn: false,
          r: 0,
          g: 0,
          b: 0,
          w: 0
        })),
        totalOn: 0,
        status: 'idle'
      }];
    }

    this.nodes.set(initialNodes);
    this.totalLights.set(initialNodes.length * PIXELS_PER_NODE);
  }

  private updateFromLightNodeStates(): void {
    if (this.nodes().length === 0) {
        this.initializeNodes();
    }

    // Only update temperature and connectivity status from telemetry
    // Do NOT update bulb on/off states - those are controlled by manual toggles only
    const updatedNodes = this.nodes().map(node => {
      // Get updated temperature from registeredNodes
      const registeredNode = this.registeredNodes.find(n => n.node_id === node.id);
      
      if (!registeredNode) return node;
      
      // Only update temp, keep bulb states as-is
      return {
        ...node,
        tempC: registeredNode?.temperature ?? node.tempC
      };
    });

    this.nodes.set(updatedNodes);
    this.activeLights.set(this.totalActiveLights);
  }

  toggleNode(nodeId: string) {
    const node = this.nodes().find(n => n.id === nodeId);
    if (!node) return;

    const allOn = node.bulbs.every(b => b.isOn);
    const newState = !allOn; // true = turn all ON, false = turn all OFF
    const wValue = newState ? 255 : 0;

    // Send MQTT command for each bulb
    node.bulbs.forEach((bulb, index) => {
      const command = {
        cmd: 'set_light',
        pixel: index, // 0-indexed for firmware
        r: 0,
        g: 0,
        b: 0,
        w: wValue,
        fade_ms: 200
      };
      this.dataService.sendNodeCommand(nodeId, command);
      
      // Mark as manually overridden
      const overrideKey = `${nodeId}-${bulb.id}`;
      this.manualOverrideUntil.set(overrideKey, Date.now() + this.OVERRIDE_DURATION_MS);
    });

    // Optimistically update UI
    const updatedNodes = this.nodes().map(n => {
      if (n.id === nodeId) {
        const updatedBulbs = n.bulbs.map(bulb => ({
          ...bulb,
          isOn: newState,
          r: 0,
          g: 0,
          b: 0,
          w: wValue
        }));

        return {
          ...n,
          bulbs: updatedBulbs,
          totalOn: newState ? n.bulbs.length : 0
          // Keep status unchanged - node stays 'active' as long as it's connected
        };
      }
      return n;
    });

    this.nodes.set(updatedNodes);
  }

  toggleBulb(nodeId: string, bulbId: number) {
    console.log('[LightMonitor] toggleBulb called: nodeId=', nodeId, 'bulbId=', bulbId);
    const node = this.nodes().find(n => n.id === nodeId);
    if (!node) {
      console.warn('[LightMonitor] Node not found:', nodeId);
      return;
    }

    const bulb = node.bulbs.find(b => b.id === bulbId);
    if (!bulb) {
      console.warn('[LightMonitor] Bulb not found:', bulbId);
      return;
    }

    // Send MQTT command to turn on/off this specific pixel
    // Use only W channel for pure warm white (SK6812B dedicated white LED)
    const command = {
      cmd: 'set_light',
      pixel: bulbId - 1, // 0-indexed for firmware
      r: 0,
      g: 0,
      b: 0,
      w: bulb.isOn ? 0 : 255,
      fade_ms: 200
    };

    console.log('[LightMonitor] About to send command:', command);
    this.dataService.sendNodeCommand(nodeId, command);
    console.log('[LightMonitor] sendNodeCommand returned');

    // Mark this node as manually overridden - prevent telemetry from resetting
    const overrideKey = `${nodeId}-${bulbId}`;
    this.manualOverrideUntil.set(overrideKey, Date.now() + this.OVERRIDE_DURATION_MS);

    // Optimistically update UI
    const updatedNodes = this.nodes().map(node => {
      if (node.id === nodeId) {
        const updatedBulbs = node.bulbs.map(bulb => {
          if (bulb.id === bulbId) {
            const newOn = !bulb.isOn;
            return { 
              ...bulb, 
              isOn: newOn,
              r: 0,
              g: 0,
              b: 0,
              w: newOn ? 255 : 0
            };
          }
          return bulb;
        });

        const totalOn = updatedBulbs.filter(b => b.isOn).length;

        return {
          ...node,
          bulbs: updatedBulbs,
          totalOn
          // Keep status unchanged - node stays 'active' as long as it's connected
        };
      }
      return node;
    });

    this.nodes.set(updatedNodes);
  }

  ngAfterViewInit(): void {
    setTimeout(() => this.initChart(), 500);
  }

  ngOnDestroy(): void {
    if (this.pollInterval) {
      clearInterval(this.pollInterval);
    }
    if (this.chart) {
      this.chart.destroy();
    }
  }

  private initChart(): void {
    const ctx = this.chartCanvas?.nativeElement?.getContext('2d');
    if (!ctx) return;

    this.chart = new Chart(ctx, {
      type: 'line',
      data: {
        labels: [],
        datasets: [
          {
            label: 'Temperature (°C)',
            data: [],
            borderColor: '#94a3b8',
            backgroundColor: 'rgba(148, 163, 184, 0.1)',
            borderWidth: 2,
            tension: 0.4,
            pointRadius: 0,
            yAxisID: 'y-temp',
            fill: true
          },
          {
            label: 'Light (lux)',
            data: [],
            borderColor: '#fbbf24',
            backgroundColor: 'rgba(251, 191, 36, 0.1)',
            borderWidth: 2,
            tension: 0.4,
            pointRadius: 0,
            yAxisID: 'y-lux',
            fill: true
          }
        ]
      },
      options: {
        responsive: true,
        maintainAspectRatio: false,
        interaction: {
          mode: 'index',
          intersect: false,
        },
        plugins: {
          legend: { display: false },
          tooltip: {
            backgroundColor: 'rgba(0, 0, 0, 0.8)',
            padding: 12,
            displayColors: true,
            callbacks: {
              label: (context) => {
                const label = context.dataset.label || '';
                const value = context.parsed.y;
                if (value === null || value === undefined) return '';
                if (label.includes('Temperature')) {
                  return `${label}: ${value.toFixed(1)}°C`;
                } else {
                  return `${label}: ${value.toFixed(0)} lux`;
                }
              }
            }
          }
        },
        scales: {
          x: {
            display: true,
            grid: { color: 'rgba(255, 255, 255, 0.05)' },
            ticks: {
              color: 'rgba(255, 255, 255, 0.5)',
              maxRotation: 0,
              maxTicksLimit: 8,
              callback: function(value, index) {
                return index % 5 === 0 ? this.getLabelForValue(value as number) : '';
              }
            }
          },
          'y-temp': {
            type: 'linear',
            position: 'left',
            min: 15,
            max: 35,
            grid: { color: 'rgba(255, 255, 255, 0.05)' },
            ticks: {
              color: '#94a3b8',
              stepSize: 2,
              callback: (value) => `${value}°C`
            },
            title: {
              display: true,
              text: 'Temperature (°C)',
              color: '#94a3b8'
            }
          },
          'y-lux': {
            type: 'linear',
            position: 'right',
            min: 0,
            max: 1000,
            grid: { drawOnChartArea: false },
            ticks: {
              color: '#fbbf24',
              stepSize: 200,
              callback: (value) => `${value}`
            },
            title: {
              display: true,
              text: 'Light (lux)',
              color: '#fbbf24'
            }
          }
        }
      }
    });

    // Start polling for data every 1 second
    this.pollInterval = setInterval(() => {
      const coords = this.dataService.coordinators();
      const nodes = this.dataService.nodes();
      
      // Get lux from coordinator (ambient light sensor)
      let lux: number = 0;
      if (coords.size > 0) {
        const coord = Array.from(coords.values())[0];
        lux = coord.light_lux ?? 0;
      }
      
      // Get temperature from first available node (BME280 sensor)
      let temperature: number = 0;
      if (nodes.size > 0) {
        const node = Array.from(nodes.values())[0];
        temperature = node.temperature ?? 0;
      }
      
      // Add data point if we have any data (lux or temperature)
      // This ensures the chart shows something even if one sensor is missing
      if (coords.size > 0 || nodes.size > 0) {
        this.addDataPoint({
          timestamp: new Date(),
          temperature: temperature,
          lux: lux
        });
      }
    }, 1000);
  }

  private addDataPoint(point: SensorDataPoint): void {
    this.sensorData.push(point);
    if (this.sensorData.length > this.MAX_DATA_POINTS) {
      this.sensorData.shift();
    }
    this.updateChart();
  }

  private updateChart(): void {
    if (!this.chart) {
      return;
    }

    const tempData = this.sensorData.map(d => d.temperature);
    const luxData = this.sensorData.map(d => d.lux);

    this.chart.data.labels = this.sensorData.map(d => 
      d.timestamp.toLocaleTimeString('en-US', { 
        hour: '2-digit', 
        minute: '2-digit',
        second: '2-digit',
        hour12: false 
      })
    );

    this.chart.data.datasets[0].data = tempData;
    this.chart.data.datasets[1].data = luxData;

    this.chart.update();
  }
}

