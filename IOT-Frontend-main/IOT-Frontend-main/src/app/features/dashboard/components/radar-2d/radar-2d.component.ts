import { Component, OnInit, OnDestroy, input, ViewChild, ElementRef, AfterViewInit } from '@angular/core';
import { CommonModule } from '@angular/common';
import { MqttService } from '../../../../core/services/mqtt.service';
import { Subscription } from 'rxjs';
import { MmwaveFrame, MmwaveTarget } from '../../../../core/models/api.models';

@Component({
  selector: 'app-radar-2d',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="radar-container">
      <canvas #radarCanvas class="radar-canvas"></canvas>
      <div class="radar-info">
        <div class="status" [class.active]="presenceDetected">
          {{ presenceDetected ? '● PRESENCE DETECTED' : '○ No Presence' }}
        </div>
        <div class="targets">Targets: {{ targetCount }}</div>
      </div>
    </div>
  `,
  styles: [`
    .radar-container {
      position: relative;
      width: 100%;
      height: 100%;
      display: flex;
      align-items: center;
      justify-content: center;
      background: var(--bg-primary);
      border-radius: 8px;
      overflow: hidden;
    }
    
    .radar-canvas {
      max-width: 100%;
      max-height: 100%;
    }
    
    .radar-info {
      position: absolute;
      top: 20px;
      left: 20px;
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    
    .status {
      display: inline-flex;
      align-items: center;
      gap: 8px;
      padding: 6px 12px;
      background: rgba(255, 255, 255, 0.05);
      border-radius: 6px;
      font-size: 12px;
      font-weight: 500;
      color: var(--text-muted);
      backdrop-filter: blur(10px);
    }
    
    .status.active {
      background: rgba(74, 222, 128, 0.15);
      color: #4ade80;
    }
    
    .targets {
      padding: 6px 12px;
      background: rgba(255, 255, 255, 0.05);
      border-radius: 6px;
      font-size: 11px;
      color: var(--text-muted);
      backdrop-filter: blur(10px);
    }
  `]
})
export class Radar2DComponent implements OnInit, OnDestroy, AfterViewInit {
  @ViewChild('radarCanvas', { static: false }) canvasRef!: ElementRef<HTMLCanvasElement>;
  
  siteId = input.required<string>();
  coordinatorId = input.required<string>();
  
  presenceDetected = false;
  targetCount = 0;
  
  private canvas!: HTMLCanvasElement;
  private ctx!: CanvasRenderingContext2D;
  private mqttSubscription?: Subscription;
  private animationFrame?: number;
  
  private readonly DETECTION_RANGE_M = 6;
  private readonly DETECTION_ANGLE_DEG = 120;
  private readonly WIDTH = 600;
  private readonly HEIGHT = 600;
  
  private latestFrame: MmwaveFrame | null = null;
  private sweepAngle = -60; // Start at -60 degrees

  constructor(private mqtt: MqttService) {}

  ngOnInit(): void {
    this.subscribeMqtt();
  }

  ngAfterViewInit(): void {
    this.canvas = this.canvasRef.nativeElement;
    this.canvas.width = this.WIDTH;
    this.canvas.height = this.HEIGHT;
    this.ctx = this.canvas.getContext('2d')!;
    this.startAnimation();
  }

  ngOnDestroy(): void {
    if (this.mqttSubscription) {
      this.mqttSubscription.unsubscribe();
    }
    if (this.animationFrame) {
      cancelAnimationFrame(this.animationFrame);
    }
  }

  private subscribeMqtt(): void {
    const subject = this.mqtt.subscribeCoordinatorMmwave(this.siteId(), this.coordinatorId());
    this.mqttSubscription = subject.subscribe({
      next: (payload) => {
        this.latestFrame = this.normalizeFrame(payload);
        this.presenceDetected = this.latestFrame.presence;
        this.targetCount = this.latestFrame.targets.length;
      }
    });
  }

  private normalizeFrame(payload: any): MmwaveFrame {
    const rawTargets = Array.isArray(payload?.targets) ? payload.targets : [];
    
    const targets: MmwaveTarget[] = rawTargets.map((t: any, idx: number) => ({
      id: t.id ?? idx,
      distance_mm: t.distance_mm ?? 0,
      position_x_mm: t.position_mm?.x ?? t.position_x_mm ?? 0,
      position_y_mm: t.position_mm?.y ?? t.position_y_mm ?? 0,
      velocity_x_m_s: t.velocity_m_s?.x ?? t.velocity_x_m_s ?? 0,
      velocity_y_m_s: t.velocity_m_s?.y ?? t.velocity_y_m_s ?? 0,
      resolution_mm: t.resolution_mm ?? 0
    }));

    return {
      site_id: payload?.site_id ?? this.siteId(),
      coordinator_id: payload?.coordinator_id ?? payload?.coord_id ?? this.coordinatorId(),
      sensor_id: payload?.sensor_id ?? 'mmwave',
      timestamp: new Date(payload?.timestamp ?? new Date().toISOString()),
      targets,
      presence: payload?.presence ?? targets.length > 0,
      confidence: payload?.confidence ?? 0.8
    };
  }

  private startAnimation(): void {
    const animate = () => {
      this.draw();
      this.animationFrame = requestAnimationFrame(animate);
    };
    animate();
  }

  private draw(): void {
    const cx = this.WIDTH / 2;
    const cy = this.HEIGHT - 50;
    const scale = (this.HEIGHT - 100) / this.DETECTION_RANGE_M;

    // Clear canvas with UI background color
    this.ctx.fillStyle = 'rgba(13, 13, 13, 1)';
    this.ctx.fillRect(0, 0, this.WIDTH, this.HEIGHT);

    // Draw detection cone
    this.drawDetectionCone(cx, cy, scale);
    
    // Draw range circles
    this.drawRangeCircles(cx, cy, scale);
    
    // Draw radial lines
    this.drawRadialLines(cx, cy, scale);
    
    // Draw sweep line (animated)
    this.drawSweepLine(cx, cy, scale);
    
    // Draw radar sensor
    this.drawRadarSensor(cx, cy);
    
    // Draw targets
    if (this.latestFrame && this.latestFrame.targets.length > 0) {
      this.drawTargets(cx, cy, scale, this.latestFrame.targets);
    }
    
    // Update sweep angle
    this.sweepAngle += 2;
    if (this.sweepAngle > 60) this.sweepAngle = -60;
  }

  private drawDetectionCone(cx: number, cy: number, scale: number): void {
    const halfAngle = (this.DETECTION_ANGLE_DEG / 2) * Math.PI / 180;
    const radius = this.DETECTION_RANGE_M * scale;

    // Fill cone with subtle gradient
    const gradient = this.ctx.createRadialGradient(cx, cy, 0, cx, cy, radius);
    gradient.addColorStop(0, 'rgba(255, 255, 255, 0.05)');
    gradient.addColorStop(1, 'rgba(255, 255, 255, 0.02)');
    
    this.ctx.fillStyle = gradient;
    this.ctx.beginPath();
    this.ctx.moveTo(cx, cy);
    this.ctx.arc(cx, cy, radius, -Math.PI/2 - halfAngle, -Math.PI/2 + halfAngle);
    this.ctx.closePath();
    this.ctx.fill();

    // Cone edges with UI color scheme
    this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.1)';
    this.ctx.lineWidth = 1.5;
    this.ctx.beginPath();
    this.ctx.moveTo(cx, cy);
    this.ctx.lineTo(cx + radius * Math.sin(-halfAngle), cy + radius * Math.cos(-halfAngle));
    this.ctx.stroke();
    
    this.ctx.beginPath();
    this.ctx.moveTo(cx, cy);
    this.ctx.lineTo(cx + radius * Math.sin(halfAngle), cy + radius * Math.cos(halfAngle));
    this.ctx.stroke();
  }

  private drawRangeCircles(cx: number, cy: number, scale: number): void {
    const halfAngle = (this.DETECTION_ANGLE_DEG / 2) * Math.PI / 180;
    
    this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
    this.ctx.lineWidth = 1;

    for (let i = 1; i <= this.DETECTION_RANGE_M; i++) {
      const radius = i * scale;
      this.ctx.beginPath();
      this.ctx.arc(cx, cy, radius, -Math.PI/2 - halfAngle, -Math.PI/2 + halfAngle);
      this.ctx.stroke();

      // Label
      if (i % 2 === 0) {
        this.ctx.fillStyle = 'rgba(255, 255, 255, 0.3)';
        this.ctx.font = '11px -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif';
        this.ctx.textAlign = 'center';
        this.ctx.fillText(`${i}m`, cx, cy - radius - 5);
      }
    }
  }

  private drawRadialLines(cx: number, cy: number, scale: number): void {
    const angles = [-60, -45, -30, 0, 30, 45, 60];
    const radius = this.DETECTION_RANGE_M * scale;

    this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
    this.ctx.lineWidth = 1;

    angles.forEach(deg => {
      const angle = (deg - 90) * Math.PI / 180;
      this.ctx.beginPath();
      this.ctx.moveTo(cx, cy);
      this.ctx.lineTo(cx + radius * Math.cos(angle), cy + radius * Math.sin(angle));
      this.ctx.stroke();
    });
  }

  private drawSweepLine(cx: number, cy: number, scale: number): void {
    const angle = (this.sweepAngle - 90) * Math.PI / 180;
    const radius = this.DETECTION_RANGE_M * scale;

    // Gradient sweep line
    const gradient = this.ctx.createLinearGradient(
      cx, cy,
      cx + radius * Math.cos(angle), cy + radius * Math.sin(angle)
    );
    gradient.addColorStop(0, 'rgba(255, 255, 255, 0.3)');
    gradient.addColorStop(1, 'rgba(255, 255, 255, 0)');

    this.ctx.strokeStyle = gradient;
    this.ctx.lineWidth = 2;
    this.ctx.beginPath();
    this.ctx.moveTo(cx, cy);
    this.ctx.lineTo(cx + radius * Math.cos(angle), cy + radius * Math.sin(angle));
    this.ctx.stroke();
  }

  private drawRadarSensor(cx: number, cy: number): void {
    // Outer glow
    const outerGradient = this.ctx.createRadialGradient(cx, cy, 0, cx, cy, 20);
    outerGradient.addColorStop(0, 'rgba(255, 255, 255, 0.15)');
    outerGradient.addColorStop(1, 'rgba(255, 255, 255, 0)');
    this.ctx.fillStyle = outerGradient;
    this.ctx.beginPath();
    this.ctx.arc(cx, cy, 20, 0, Math.PI * 2);
    this.ctx.fill();

    // Inner glow
    const innerGradient = this.ctx.createRadialGradient(cx, cy, 0, cx, cy, 10);
    innerGradient.addColorStop(0, 'rgba(255, 255, 255, 0.8)');
    innerGradient.addColorStop(1, 'rgba(255, 255, 255, 0.3)');
    this.ctx.fillStyle = innerGradient;
    this.ctx.beginPath();
    this.ctx.arc(cx, cy, 10, 0, Math.PI * 2);
    this.ctx.fill();

    // Sensor dot
    this.ctx.fillStyle = '#ffffff';
    this.ctx.beginPath();
    this.ctx.arc(cx, cy, 4, 0, Math.PI * 2);
    this.ctx.fill();
  }

  private drawTargets(cx: number, cy: number, scale: number, targets: MmwaveTarget[]): void {
    targets.slice(0, 3).forEach((target, index) => {
      const x = (target.position_x_mm / 1000.0) * scale;
      const y = (target.position_y_mm / 1000.0) * scale;
      
      const targetX = cx + x;
      const targetY = cy - y;

      // Outer glow with white theme
      const gradient = this.ctx.createRadialGradient(targetX, targetY, 0, targetX, targetY, 25);
      gradient.addColorStop(0, 'rgba(255, 255, 255, 0.4)');
      gradient.addColorStop(0.5, 'rgba(255, 255, 255, 0.15)');
      gradient.addColorStop(1, 'rgba(255, 255, 255, 0)');
      this.ctx.fillStyle = gradient;
      this.ctx.beginPath();
      this.ctx.arc(targetX, targetY, 25, 0, Math.PI * 2);
      this.ctx.fill();

      // Target ring
      this.ctx.strokeStyle = 'rgba(255, 255, 255, 0.6)';
      this.ctx.lineWidth = 2;
      this.ctx.beginPath();
      this.ctx.arc(targetX, targetY, 10, 0, Math.PI * 2);
      this.ctx.stroke();

      // Target center dot
      this.ctx.fillStyle = '#ffffff';
      this.ctx.beginPath();
      this.ctx.arc(targetX, targetY, 4, 0, Math.PI * 2);
      this.ctx.fill();

      // Target ID with better font
      this.ctx.fillStyle = 'rgba(255, 255, 255, 0.9)';
      this.ctx.font = '11px -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif';
      this.ctx.textAlign = 'center';
      this.ctx.fillText(`T${target.id}`, targetX, targetY - 20);
    });
  }
}
