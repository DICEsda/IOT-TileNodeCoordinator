import { Component, OnInit, OnDestroy, ViewChild, ElementRef, AfterViewInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { DataService } from '../../../../core/services/data.service';
import { Chart, registerables } from 'chart.js';
import { Subscription } from 'rxjs';

Chart.register(...registerables);

interface SensorDataPoint {
  timestamp: Date;
  temperature: number;
  lux: number;
}

@Component({
  selector: 'app-sensor-chart',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="sensor-chart-card card">
      <div class="chart-header">
        <h4>Sensor Data</h4>
        <div class="legend">
          <div class="legend-item">
            <span class="legend-color" style="background: #94a3b8;"></span>
            <span>Temperature (°C)</span>
          </div>
          <div class="legend-item">
            <span class="legend-color" style="background: #64748b;"></span>
            <span>Light (lux)</span>
          </div>
        </div>
      </div>
      <div class="chart-container">
        <canvas #chartCanvas></canvas>
      </div>
    </div>
  `,
  styles: [`
    .sensor-chart-card {
      background: rgba(255, 255, 255, 0.02);
      border: 1px solid rgba(255, 255, 255, 0.1);
      border-radius: 12px;
      padding: 20px;
      display: flex;
      flex-direction: column;
      gap: 16px;
      min-height: 300px;
      transition: all 0.3s ease;
    }
    
    .sensor-chart-card:hover {
      background: rgba(255, 255, 255, 0.04);
    }

    .chart-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      flex-wrap: wrap;
      gap: 12px;
    }

    .chart-header h4 {
      margin: 0;
      font-size: 18px;
      font-weight: 600;
      color: rgba(255, 255, 255, 0.9);
    }

    .legend {
      display: flex;
      gap: 16px;
      font-size: 13px;
    }

    .legend-item {
      display: flex;
      align-items: center;
      gap: 6px;
      color: rgba(255, 255, 255, 0.7);
    }

    .legend-color {
      width: 12px;
      height: 12px;
      border-radius: 2px;
    }

    .chart-container {
      flex: 1;
      position: relative;
      min-height: 250px;
    }

    canvas {
      width: 100% !important;
      height: 100% !important;
    }
  `]
})
export class SensorChartComponent implements OnInit, AfterViewInit, OnDestroy {
  @ViewChild('chartCanvas', { static: false }) canvasRef!: ElementRef<HTMLCanvasElement>;
  
  private chart?: Chart;
  private dataSubscription?: Subscription;
  private sensorData: SensorDataPoint[] = [];
  private readonly MAX_DATA_POINTS = 30; // Keep last 30 data points
  
  constructor(private dataService: DataService) {}

  ngOnInit(): void {
    // Subscribe to coordinator telemetry via DataService signal
    // We'll poll the coordinator data periodically
    setInterval(() => {
      const coords = this.dataService.coordinators();
      if (coords.size > 0) {
        const coord = Array.from(coords.values())[0];
        console.log('[SensorChart] Coordinator data:', {
          temp_c: coord.temp_c,
          light_lux: coord.light_lux,
          status: coord.status
        });
        if (coord.temp_c !== undefined && coord.light_lux !== undefined) {
          this.addDataPoint({
            timestamp: new Date(),
            temperature: coord.temp_c,
            lux: coord.light_lux
          });
          console.log('[SensorChart] Added data point - Temp:', coord.temp_c, 'Lux:', coord.light_lux);
        }
      } else {
        console.log('[SensorChart] No coordinators found');
      }
    }, 2000); // Poll every 2 seconds
  }

  ngAfterViewInit(): void {
    this.initChart();
  }

  ngOnDestroy(): void {
    if (this.dataSubscription) {
      this.dataSubscription.unsubscribe();
    }
    if (this.chart) {
      this.chart.destroy();
    }
  }

  private initChart(): void {
    const ctx = this.canvasRef.nativeElement.getContext('2d');
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
            pointHoverRadius: 4,
            yAxisID: 'y-temp',
            fill: true
          },
          {
            label: 'Light (lux)',
            data: [],
            borderColor: '#64748b',
            backgroundColor: 'rgba(100, 116, 139, 0.1)',
            borderWidth: 2,
            tension: 0.4,
            pointRadius: 0,
            pointHoverRadius: 4,
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
          legend: {
            display: false
          },
          tooltip: {
            backgroundColor: 'rgba(0, 0, 0, 0.8)',
            titleColor: 'rgba(255, 255, 255, 0.9)',
            bodyColor: 'rgba(255, 255, 255, 0.8)',
            borderColor: 'rgba(255, 255, 255, 0.1)',
            borderWidth: 1,
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
            grid: {
              color: 'rgba(255, 255, 255, 0.05)',
              drawTicks: false
            },
            ticks: {
              color: 'rgba(255, 255, 255, 0.5)',
              maxRotation: 0,
              maxTicksLimit: 8,
              callback: function(value, index) {
                // Show every 5th label
                return index % 5 === 0 ? this.getLabelForValue(value as number) : '';
              }
            },
            border: {
              display: false
            }
          },
          'y-temp': {
            type: 'linear',
            position: 'left',
            display: true,
            min: 0,
            max: 50,
            grid: {
              color: 'rgba(255, 255, 255, 0.05)',
              drawTicks: false
            },
            ticks: {
              color: '#94a3b8',
              stepSize: 10,
              callback: (value) => `${value}°C`
            },
            border: {
              display: false
            },
            title: {
              display: true,
              text: 'Temperature (°C)',
              color: '#94a3b8',
              font: {
                size: 12,
                weight: 'normal'
              }
            }
          },
          'y-lux': {
            type: 'linear',
            position: 'right',
            display: true,
            min: 0,
            max: 1000,
            grid: {
              drawOnChartArea: false,
              drawTicks: false
            },
            ticks: {
              color: '#64748b',
              stepSize: 200,
              callback: (value) => `${value}`
            },
            border: {
              display: false
            },
            title: {
              display: true,
              text: 'Light (lux)',
              color: '#64748b',
              font: {
                size: 12,
                weight: 'normal'
              }
            }
          }
        }
      }
    });
  }

  private addDataPoint(point: SensorDataPoint): void {
    this.sensorData.push(point);
    
    // Keep only last MAX_DATA_POINTS
    if (this.sensorData.length > this.MAX_DATA_POINTS) {
      this.sensorData.shift();
    }
    
    this.updateChart();
  }

  private updateChart(): void {
    if (!this.chart) return;

    // Update labels (timestamps)
    this.chart.data.labels = this.sensorData.map(d => 
      d.timestamp.toLocaleTimeString('en-US', { 
        hour: '2-digit', 
        minute: '2-digit',
        second: '2-digit',
        hour12: false 
      })
    );

    // Update temperature data
    this.chart.data.datasets[0].data = this.sensorData.map(d => d.temperature);
    
    // Update lux data
    this.chart.data.datasets[1].data = this.sensorData.map(d => d.lux);

    this.chart.update('none'); // Update without animation for smooth real-time updates
  }
}
