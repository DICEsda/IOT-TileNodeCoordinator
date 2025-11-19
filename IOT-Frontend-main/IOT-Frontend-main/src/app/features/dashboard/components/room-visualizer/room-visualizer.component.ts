import { CommonModule, DatePipe, DecimalPipe } from '@angular/common';
import { Component, ElementRef, EventEmitter, Input, OnDestroy, OnInit, Output, ViewChild } from '@angular/core';
import { Subscription, firstValueFrom } from 'rxjs';
import { MmwaveFrame, MmwaveTarget } from '../../../../core/models/api.models';
import { ApiService } from '../../../../core/services/api.service';
import { MqttService } from '../../../../core/services/mqtt.service';

interface LightNodeState {
  nodeId: number;
  totalBulbs: number;
  activeBulbs: number;
}

interface TargetMarker {
  id: number;
  x: number;
  y: number;
  strength: number;
}

@Component({
  selector: 'app-room-visualizer',
  imports: [CommonModule, DatePipe, DecimalPipe],
  templateUrl: './room-visualizer.component.html',
  styleUrl: './room-visualizer.component.scss'
})
export class RoomVisualizerComponent implements OnInit, OnDestroy {
  private static readonly NODE_BUCKETS = 6;
  private static readonly LIGHTS_PER_NODE = 6;

  @ViewChild('radarCanvas', { static: true }) canvasRef!: ElementRef<HTMLCanvasElement>;
  @Input() siteId: string = 'site001';
  @Input() coordinatorId: string = 'coord001';
  @Output() lightStateChanged = new EventEmitter<{
    totalActive: number;
    nodeStates: LightNodeState[];
  }>();

  presenceDetected = false;
  targetCount = 0;
  lastUpdated: Date | null = null;
  latestFrame: MmwaveFrame | null = null;
  frames: MmwaveFrame[] = [];

  private ctx: CanvasRenderingContext2D | null = null;
  private animationId: number = 0;
  private sweepAngle = 0;
  private targetTrail: TargetMarker[] = [];
  private maxRangeMm = 6000;
  private pixelRatio = window.devicePixelRatio || 1;
  private mmwaveSubscription?: Subscription;
  private mmwaveTopic?: string;
  private resizeObserver?: ResizeObserver;
  private boundWindowResize?: () => void;

  constructor(
    private readonly api: ApiService,
    private readonly mqtt: MqttService
  ) {}

  ngOnInit() {
    const canvas = this.canvasRef.nativeElement;
    this.ctx = canvas.getContext('2d');

    if (!this.ctx) {
      console.error('[RoomVisualizer] Unable to acquire canvas context');
      return;
    }

    this.setupCanvasSizing();
    this.emitLightState([]);
    this.loadHistory();
    this.subscribeToRealtime();
    this.animationId = requestAnimationFrame(this.renderRadar);
  }

  ngOnDestroy() {
    if (this.animationId) {
      cancelAnimationFrame(this.animationId);
    }

    if (this.mmwaveSubscription) {
      this.mmwaveSubscription.unsubscribe();
    }

    if (this.mmwaveTopic) {
      this.mqtt.unsubscribe(this.mmwaveTopic);
    }

    if (this.resizeObserver) {
      this.resizeObserver.disconnect();
    } else if (this.boundWindowResize) {
      window.removeEventListener('resize', this.boundWindowResize);
    }
  }

  private async loadHistory(): Promise<void> {
    try {
      const frames = await firstValueFrom(
        this.api.getMmwaveHistory({
          siteId: this.siteId,
          coordinatorId: this.coordinatorId,
          limit: 120
        })
      );

      const normalized = frames
        .map(frame => this.normalizeFrame(frame))
        .sort((a, b) => b.timestamp.getTime() - a.timestamp.getTime());

      this.frames = normalized;

      if (normalized.length > 0) {
        this.applyFrame(normalized[0]);
      }
    } catch (error) {
      console.error('[RoomVisualizer] Failed to load mmWave history', error);
    }
  }

  private subscribeToRealtime(): void {
    const topic = `site/${this.siteId}/coord/${this.coordinatorId}/mmwave`;
    this.mmwaveTopic = topic;
    const subject = this.mqtt.subscribeCoordinatorMmwave(this.siteId, this.coordinatorId);
    this.mmwaveSubscription = subject.subscribe({
      next: payload => {
        const frame = this.normalizeLiveFrame(payload);
        this.frames = [frame, ...this.frames].slice(0, 240);
        this.applyFrame(frame);
      },
      error: err => console.error('[RoomVisualizer] MQTT mmWave error', err)
    });
  }

  private setupCanvasSizing(): void {
    const canvas = this.canvasRef.nativeElement;
    const parent = canvas.parentElement ?? canvas;

    const resize = () => {
      const bounds = parent.getBoundingClientRect();
      const size = Math.min(bounds.width, bounds.height || bounds.width || 420);
      this.pixelRatio = typeof window !== 'undefined' ? window.devicePixelRatio || 1 : 1;

      canvas.width = size * this.pixelRatio;
      canvas.height = size * this.pixelRatio;
      canvas.style.width = `${size}px`;
      canvas.style.height = `${size}px`;

      const context = this.ctx;
      if (context) {
        context.setTransform(1, 0, 0, 1, 0, 0);
        context.scale(this.pixelRatio, this.pixelRatio);
      }
    };

    resize();

    if (typeof window !== 'undefined' && 'ResizeObserver' in window) {
      this.resizeObserver = new ResizeObserver(() => resize());
      this.resizeObserver.observe(parent);
    } else {
      const win = typeof window !== 'undefined' ? (window as Window) : undefined;
      if (win) {
        this.boundWindowResize = resize;
        win.addEventListener('resize', resize as EventListener);
      }
    }
  }

  private normalizeFrame(frame: MmwaveFrame): MmwaveFrame {
    const rawTimestamp = (frame as any).timestamp;
    const timestamp = rawTimestamp instanceof Date
      ? rawTimestamp
      : rawTimestamp
        ? new Date(rawTimestamp)
        : new Date();

    return {
      ...frame,
      timestamp,
      targets: (frame.targets ?? []).map(target => {
        const positionX = this.safeNumber(target.position_x_mm);
        const positionY = this.safeNumber(target.position_y_mm);
        const velocityX = this.safeNumber(target.velocity_x_m_s);
        const velocityY = this.safeNumber(target.velocity_y_m_s);
        const distance = target.distance_mm !== undefined
          ? this.safeNumber(target.distance_mm)
          : Math.sqrt(positionX * positionX + positionY * positionY);

        return {
          ...target,
          distance_mm: distance,
          position_x_mm: positionX,
          position_y_mm: positionY,
          velocity_x_m_s: velocityX,
          velocity_y_m_s: velocityY
        };
      })
    };
  }

  private normalizeLiveFrame(payload: any): MmwaveFrame {
    const ts = typeof payload?.timestamp === 'string'
      ? new Date(payload.timestamp)
      : new Date(((payload?.ts ?? Date.now() / 1000) as number) * 1000);

    const rawTargets = Array.isArray(payload?.targets) ? payload.targets : [];

    const targets: MmwaveTarget[] = rawTargets.map((target: any) => {
      const position = target.position_mm || {};
      const velocity = target.velocity_m_s || {};

      const positionX = this.safeNumber(target.position_x_mm ?? position.x);
      const positionY = this.safeNumber(target.position_y_mm ?? position.y);

      const velocityX = this.safeNumber(target.velocity_x_m_s ?? velocity.x);
      const velocityY = this.safeNumber(target.velocity_y_m_s ?? velocity.y);

      const defaultDistance = Math.sqrt(positionX ** 2 + positionY ** 2);

      return {
        id: Math.trunc(this.safeNumber(target.id, 0)),
        distance_mm: target.distance_mm !== undefined ? this.safeNumber(target.distance_mm) : defaultDistance,
        speed_cm_s: target.speed_cm_s !== undefined
          ? this.safeNumber(target.speed_cm_s)
          : Math.hypot(velocityX, velocityY) * 100,
        resolution_mm: this.safeNumber(target.resolution_mm),
        position_x_mm: positionX,
        position_y_mm: positionY,
        velocity_x_m_s: velocityX,
        velocity_y_m_s: velocityY
      };
    });

    return {
      site_id: payload?.site_id ?? this.siteId,
      coordinator_id: payload?.coord_id ?? payload?.coordinator_id ?? this.coordinatorId,
      sensor_id: payload?.sensor_id ?? 'mmwave',
      presence: Boolean(payload?.presence),
      confidence: Number(payload?.confidence ?? 0),
      targets,
      timestamp: ts
    };
  }

  private safeNumber(value: any, fallback = 0): number {
    const parsed = Number(value);
    return Number.isFinite(parsed) ? parsed : fallback;
  }

  private applyFrame(frame: MmwaveFrame): void {
    this.latestFrame = frame;
    this.presenceDetected = frame.presence;
    this.targetCount = frame.targets.length;
    this.lastUpdated = frame.timestamp;

    this.updateMaxRange(frame.targets);
    this.updateTargetTrail(frame.targets);
    this.emitLightState(frame.targets);
  }

  private updateMaxRange(targets: MmwaveTarget[]): void {
    if (!targets.length) {
      return;
    }

    const maxDistance = targets.reduce((acc, target) => {
      const distance = target.distance_mm ?? Math.sqrt(target.position_x_mm ** 2 + target.position_y_mm ** 2);
      return Math.max(acc, distance);
    }, 0);

    const padded = maxDistance * 1.15;
    this.maxRangeMm = Math.min(Math.max(padded, 2000), 12000);
  }

  private updateTargetTrail(targets: MmwaveTarget[]): void {
    targets.forEach(target => {
      const existing = this.targetTrail.find(marker => marker.id === target.id);
      const x = target.position_x_mm ?? 0;
      const y = target.position_y_mm ?? 0;

      if (existing) {
        existing.x = x;
        existing.y = y;
        existing.strength = 1;
      } else {
        this.targetTrail.push({
          id: target.id,
          x,
          y,
          strength: 1
        });
      }
    });
  }

  private emitLightState(targets: MmwaveTarget[]): void {
    const buckets = Array.from({ length: RoomVisualizerComponent.NODE_BUCKETS }, (_, index) => ({
      nodeId: index + 1,
      totalBulbs: RoomVisualizerComponent.LIGHTS_PER_NODE,
      activeBulbs: 0
    }));

    const segmentSize = (Math.PI * 2) / RoomVisualizerComponent.NODE_BUCKETS;

    targets.forEach(target => {
      const angle = Math.atan2(target.position_y_mm ?? 0, target.position_x_mm ?? 0);
      const normalized = (angle + Math.PI * 2) % (Math.PI * 2);
      const bucketIndex = Math.min(
        RoomVisualizerComponent.NODE_BUCKETS - 1,
        Math.floor(normalized / segmentSize)
      );

      buckets[bucketIndex].activeBulbs = Math.min(
        RoomVisualizerComponent.LIGHTS_PER_NODE,
        buckets[bucketIndex].activeBulbs + 1
      );
    });

    this.lightStateChanged.emit({
      totalActive: targets.length,
      nodeStates: buckets
    });
  }

  private renderRadar = () => {
    this.animationId = requestAnimationFrame(this.renderRadar);

    const ctx = this.ctx;
    if (!ctx) {
      return;
    }

    const canvas = this.canvasRef.nativeElement;
  const width = canvas.width / this.pixelRatio;
  const height = canvas.height / this.pixelRatio;
    const radius = Math.min(width, height) / 2 - 20;

    ctx.save();
    ctx.clearRect(0, 0, width, height);
    this.drawBackground(width, height);
    ctx.translate(width / 2, height / 2);
    this.drawGrid(radius);
    this.drawSweep(radius);
    this.decayTrail();
    this.drawTargets(radius);
    ctx.restore();
  };

  private drawBackground(width: number, height: number): void {
    const ctx = this.ctx;
    if (!ctx) return;
    const gradient = ctx.createLinearGradient(0, 0, 0, height);
    gradient.addColorStop(0, '#06121f');
    gradient.addColorStop(1, '#091a2b');

    ctx.fillStyle = gradient;
    ctx.fillRect(0, 0, width, height);
  }

  private drawGrid(radius: number): void {
    const ctx = this.ctx;
    if (!ctx) return;

    ctx.strokeStyle = 'rgba(0, 255, 191, 0.15)';
    ctx.lineWidth = 1;

    for (let i = 1; i <= 4; i++) {
      const r = (radius / 4) * i;
      ctx.beginPath();
      ctx.arc(0, 0, r, 0, Math.PI * 2);
      ctx.stroke();
    }

    ctx.beginPath();
    ctx.moveTo(-radius, 0);
    ctx.lineTo(radius, 0);
    ctx.moveTo(0, -radius);
    ctx.lineTo(0, radius);
    ctx.stroke();

    ctx.font = '12px Inter, sans-serif';
    ctx.fillStyle = 'rgba(221, 255, 244, 0.6)';
    ctx.textAlign = 'right';
    ctx.textBaseline = 'top';

    const labels = [0.25, 0.5, 0.75, 1];
    labels.forEach(multiplier => {
      const value = (this.maxRangeMm / 1000) * multiplier;
      const text = `${value.toFixed(1)} m`;
      const y = -radius * multiplier;
      ctx.fillText(text, radius - 4, y + 4);
    });
  }

  private drawSweep(radius: number): void {
    const ctx = this.ctx;
    if (!ctx) return;

    const sweepWidth = Math.PI / 36; // 5 degrees
    const startAngle = this.sweepAngle;
    const endAngle = startAngle + sweepWidth;

    const gradient = ctx.createRadialGradient(0, 0, radius * 0.1, 0, 0, radius);
    gradient.addColorStop(0, 'rgba(0, 255, 191, 0.0)');
    gradient.addColorStop(1, 'rgba(0, 255, 191, 0.35)');

    ctx.beginPath();
    ctx.moveTo(0, 0);
    ctx.arc(0, 0, radius, startAngle, endAngle);
    ctx.closePath();
    ctx.fillStyle = gradient;
    ctx.fill();

    ctx.strokeStyle = 'rgba(0, 255, 191, 0.8)';
    ctx.lineWidth = 1.5;
    ctx.beginPath();
    ctx.moveTo(0, 0);
    ctx.lineTo(radius * Math.cos(startAngle), radius * Math.sin(startAngle));
    ctx.stroke();

    this.sweepAngle = (this.sweepAngle + 0.02) % (Math.PI * 2);
  }

  private drawTargets(radius: number): void {
    const ctx = this.ctx;
    if (!ctx) return;

    const scale = radius / this.maxRangeMm;

    this.targetTrail.forEach(marker => {
      const x = marker.x * scale;
      const y = -marker.y * scale;

      const alpha = Math.min(1, Math.max(0.2, marker.strength));
      ctx.fillStyle = `rgba(0, 255, 191, ${alpha})`;
      ctx.shadowColor = `rgba(0, 255, 191, ${alpha})`;
      ctx.shadowBlur = 10;

      ctx.beginPath();
      ctx.arc(x, y, 6, 0, Math.PI * 2);
      ctx.fill();

      ctx.shadowBlur = 0;
      ctx.font = '11px Inter, sans-serif';
      ctx.fillStyle = `rgba(221, 255, 244, ${alpha})`;
      ctx.textAlign = 'center';
      ctx.textBaseline = 'bottom';
      ctx.fillText(`ID ${marker.id}`, x, y - 8);
    });
  }

  private decayTrail(): void {
    for (let i = this.targetTrail.length - 1; i >= 0; i--) {
      const marker = this.targetTrail[i];
      marker.strength = Math.max(0, marker.strength - 0.01);
      if (marker.strength <= 0.05) {
        this.targetTrail.splice(i, 1);
      }
    }
  }
}

