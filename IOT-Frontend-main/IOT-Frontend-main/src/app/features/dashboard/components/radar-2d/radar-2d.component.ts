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
      background: transparent;
      overflow: visible;
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
  private waveRadius = 0; // Wave emanating from center
  private waveOpacity = 1; // Fade out as wave expands
  
  // Smoothing & filtering
  private smoothedTargets: Map<number, {x: number[], y: number[], distance: number[]}> = new Map();
  private readonly SMOOTHING_WINDOW = 5; // Average over last 5 frames
  private lastUpdateTime = 0;
  private readonly UPDATE_THROTTLE_MS = 50; // Only update every 50ms (20Hz max)

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
        // Throttle updates to avoid overwhelming the UI
        const now = Date.now();
        if (now - this.lastUpdateTime < this.UPDATE_THROTTLE_MS) {
          return;
        }
        this.lastUpdateTime = now;
        
        const frame = this.normalizeFrame(payload);
        
        // Apply smoothing to targets
        if (frame.targets.length > 0) {
          frame.targets = this.smoothTargets(frame.targets);
        }
        
        this.latestFrame = frame;
        this.presenceDetected = this.latestFrame.presence;
        this.targetCount = this.latestFrame.targets.length;
      }
    });
  }
  
  private smoothTargets(targets: MmwaveTarget[]): MmwaveTarget[] {
    return targets.map(target => {
      const id = target.id;
      
      // Get or create smoothing buffer for this target
      if (!this.smoothedTargets.has(id)) {
        this.smoothedTargets.set(id, { x: [], y: [], distance: [] });
      }
      
      const buffer = this.smoothedTargets.get(id)!;
      
      // Add new values
      buffer.x.push(target.position_x_mm);
      buffer.y.push(target.position_y_mm);
      buffer.distance.push(target.distance_mm);
      
      // Keep only last N frames
      if (buffer.x.length > this.SMOOTHING_WINDOW) {
        buffer.x.shift();
        buffer.y.shift();
        buffer.distance.shift();
      }
      
      // Calculate moving average
      const avgX = buffer.x.reduce((a, b) => a + b, 0) / buffer.x.length;
      const avgY = buffer.y.reduce((a, b) => a + b, 0) / buffer.y.length;
      const avgDist = buffer.distance.reduce((a, b) => a + b, 0) / buffer.distance.length;
      
      return {
        ...target,
        position_x_mm: avgX,
        position_y_mm: avgY,
        distance_mm: avgDist
      };
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
    
    // Draw detection zone (2m-3m)
    this.drawDetectionZone(cx, cy, scale);
    
    // Draw radial lines
    this.drawRadialLines(cx, cy, scale);
    
    // Draw wave (animated)
    this.drawWave(cx, cy, scale);
    
    // Draw radar sensor
    this.drawRadarSensor(cx, cy);
    
    // Draw targets
    if (this.latestFrame && this.latestFrame.targets.length > 0) {
      this.drawTargets(cx, cy, scale, this.latestFrame.targets);
    }
    
    // Update wave animation (slower, smoother)
    this.waveRadius += 1.2;
    const maxRadius = this.DETECTION_RANGE_M * scale;
    if (this.waveRadius > maxRadius) {
      this.waveRadius = 0;
      this.waveOpacity = 1;
    } else {
      // Smoother fade with quadratic easing
      const progress = this.waveRadius / maxRadius;
      this.waveOpacity = 1 - (progress * progress);
    }
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

  private drawDetectionZone(cx: number, cy: number, scale: number): void {
    // Detection zone: 2m to 3m, ±1m width (total 2m width at center)
    const innerRadius = 2 * scale; // 2 meters
    const outerRadius = 3 * scale; // 3 meters
    const halfAngle = (this.DETECTION_ANGLE_DEG / 2) * Math.PI / 180;
    
    // Calculate the angular width for ±1m at the center of the zone (2.5m)
    const zoneCenter = 2.5 * scale;
    const halfWidth = 1 * scale; // ±1m = 2m total width
    const angularWidth = Math.atan(halfWidth / zoneCenter); // angle in radians
    
    // Draw filled zone with semi-transparent green
    const gradient = this.ctx.createRadialGradient(cx, cy, innerRadius, cx, cy, outerRadius);
    gradient.addColorStop(0, 'rgba(34, 197, 94, 0.15)'); // green-500 with transparency
    gradient.addColorStop(0.5, 'rgba(34, 197, 94, 0.2)');
    gradient.addColorStop(1, 'rgba(34, 197, 94, 0.15)');
    
    this.ctx.fillStyle = gradient;
    this.ctx.beginPath();
    this.ctx.arc(cx, cy, outerRadius, -Math.PI/2 - angularWidth, -Math.PI/2 + angularWidth);
    this.ctx.arc(cx, cy, innerRadius, -Math.PI/2 + angularWidth, -Math.PI/2 - angularWidth, true);
    this.ctx.closePath();
    this.ctx.fill();
    
    // Draw borders of the zone
    this.ctx.strokeStyle = 'rgba(34, 197, 94, 0.5)'; // green-500
    this.ctx.lineWidth = 2;
    
    // Inner arc
    this.ctx.beginPath();
    this.ctx.arc(cx, cy, innerRadius, -Math.PI/2 - angularWidth, -Math.PI/2 + angularWidth);
    this.ctx.stroke();
    
    // Outer arc
    this.ctx.beginPath();
    this.ctx.arc(cx, cy, outerRadius, -Math.PI/2 - angularWidth, -Math.PI/2 + angularWidth);
    this.ctx.stroke();
    
    // Side lines
    this.ctx.beginPath();
    this.ctx.moveTo(cx + innerRadius * Math.sin(-angularWidth), cy - innerRadius * Math.cos(-angularWidth));
    this.ctx.lineTo(cx + outerRadius * Math.sin(-angularWidth), cy - outerRadius * Math.cos(-angularWidth));
    this.ctx.stroke();
    
    this.ctx.beginPath();
    this.ctx.moveTo(cx + innerRadius * Math.sin(angularWidth), cy - innerRadius * Math.cos(angularWidth));
    this.ctx.lineTo(cx + outerRadius * Math.sin(angularWidth), cy - outerRadius * Math.cos(angularWidth));
    this.ctx.stroke();
    
    // Add label
    this.ctx.fillStyle = 'rgba(34, 197, 94, 0.8)';
    this.ctx.font = 'bold 12px -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif';
    this.ctx.textAlign = 'center';
    this.ctx.fillText('DETECTION ZONE', cx, cy - zoneCenter - 15);
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

  private drawWave(cx: number, cy: number, scale: number): void {
    if (this.waveRadius <= 0) return;

    const halfAngle = (this.DETECTION_ANGLE_DEG / 2) * Math.PI / 180;
    
    // Outer glow (widest, most subtle)
    this.ctx.strokeStyle = `rgba(255, 255, 255, ${this.waveOpacity * 0.08})`;
    this.ctx.lineWidth = 20;
    this.ctx.beginPath();
    this.ctx.arc(cx, cy, this.waveRadius, -Math.PI/2 - halfAngle, -Math.PI/2 + halfAngle);
    this.ctx.stroke();

    // Middle glow
    this.ctx.strokeStyle = `rgba(255, 255, 255, ${this.waveOpacity * 0.15})`;
    this.ctx.lineWidth = 10;
    this.ctx.beginPath();
    this.ctx.arc(cx, cy, this.waveRadius, -Math.PI/2 - halfAngle, -Math.PI/2 + halfAngle);
    this.ctx.stroke();
    
    // Inner bright wave line
    this.ctx.strokeStyle = `rgba(255, 255, 255, ${this.waveOpacity * 0.4})`;
    this.ctx.lineWidth = 2;
    this.ctx.beginPath();
    this.ctx.arc(cx, cy, this.waveRadius, -Math.PI/2 - halfAngle, -Math.PI/2 + halfAngle);
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
