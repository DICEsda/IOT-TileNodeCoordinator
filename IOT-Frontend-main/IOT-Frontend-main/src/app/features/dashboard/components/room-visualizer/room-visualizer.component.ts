import { CommonModule, DatePipe, DecimalPipe } from '@angular/common';
import { Component, ElementRef, EventEmitter, Input, OnChanges, OnDestroy, OnInit, Output, SimpleChanges, ViewChild, AfterViewInit, NgZone } from '@angular/core';
import { Subscription, firstValueFrom } from 'rxjs';
import { MmwaveFrame, MmwaveTarget, Node } from '../../../../core/models/api.models';
import { ApiService } from '../../../../core/services/api.service';
import { MqttService } from '../../../../core/services/mqtt.service';
import { Radar2DComponent } from '../radar-2d/radar-2d.component';

interface LightNodeState {
  nodeId: string;
  totalBulbs: number;
  activeBulbs: number;
}

@Component({
  selector: 'app-room-visualizer',
  imports: [CommonModule, DatePipe, DecimalPipe, Radar2DComponent],
  templateUrl: './room-visualizer.component.html',
  styleUrl: './room-visualizer.component.scss'
})
export class RoomVisualizerComponent implements OnInit, OnDestroy, AfterViewInit, OnChanges {
  private static readonly DEFAULT_BUCKETS = 6;
  private static readonly LIGHTS_PER_NODE = 6;

  @ViewChild('radarCanvas', { static: false }) canvasRef!: ElementRef<HTMLCanvasElement>;
  @Input() siteId: string = 'site001';
  @Input() coordinatorId: string = 'coord001';
  @Input() registeredNodes: Node[] = [];
  
  @Output() lightStateChanged = new EventEmitter<{
    totalActive: number;
    nodeStates: LightNodeState[];
  }>();

  presenceDetected = false;
  targetCount = 0;
  lastUpdated: Date | null = null;
  latestFrame: MmwaveFrame | null = null;
  frames: MmwaveFrame[] = [];

  private ctx!: CanvasRenderingContext2D;
  private animationFrameId: number | null = null;
  private mmwaveSubscription?: Subscription;
  private mmwaveTopic?: string;
  private bucketCount = RoomVisualizerComponent.DEFAULT_BUCKETS;

  constructor(
    private readonly api: ApiService,
    private readonly mqtt: MqttService,
    private readonly ngZone: NgZone
  ) {
    console.log('[RoomVisualizer] Constructor - siteId:', this.siteId, 'coordinatorId:', this.coordinatorId);
  }

  ngOnInit() {
    console.log('[RoomVisualizer] ngOnInit - siteId:', this.siteId, 'coordinatorId:', this.coordinatorId);
    this.updateBucketCount();
    this.emitLightState([]);
    this.loadHistory();
    this.subscribeToRealtime();
  }

  ngOnChanges(changes: SimpleChanges) {
    if (changes['registeredNodes']) {
      this.updateBucketCount();
    }
  }

  private updateBucketCount() {
    if (this.registeredNodes && this.registeredNodes.length > 0) {
      this.bucketCount = this.registeredNodes.length;
    } else {
      this.bucketCount = RoomVisualizerComponent.DEFAULT_BUCKETS;
    }
  }

  ngAfterViewInit() {
    // Canvas initialization disabled - using Radar3D component instead
    // this.initCanvas();
    // this.animate();
  }

  ngOnDestroy() {
    if (this.animationFrameId !== null) {
      cancelAnimationFrame(this.animationFrameId);
    }

    if (this.mmwaveSubscription) {
      this.mmwaveSubscription.unsubscribe();
    }

    if (this.mmwaveTopic) {
      this.mqtt.unsubscribe(this.mmwaveTopic);
    }
  }

  private initCanvas() {
    const canvas = this.canvasRef.nativeElement;
    const parent = canvas.parentElement;
    if (parent) {
      canvas.width = parent.clientWidth;
      canvas.height = parent.clientHeight;
    }
    this.ctx = canvas.getContext('2d')!;
    
    // Handle resize
    window.addEventListener('resize', () => this.onWindowResize());
  }

  private onWindowResize() {
    if (!this.canvasRef) return;
    const canvas = this.canvasRef.nativeElement;
    const parent = canvas.parentElement;
    if (parent) {
      canvas.width = parent.clientWidth;
      canvas.height = parent.clientHeight;
    }
  }

  private animate() {
    this.ngZone.runOutsideAngular(() => {
      const loop = () => {
        this.animationFrameId = requestAnimationFrame(loop);
        this.draw();
      };
      loop();
    });
  }

  private draw() {
    if (!this.ctx || !this.canvasRef) return;
    const canvas = this.canvasRef.nativeElement;
    const width = canvas.width;
    const height = canvas.height;
    const ctx = this.ctx;

    // Clear
    ctx.clearRect(0, 0, width, height);

    // Center point (bottom center for radar view)
    const centerX = width / 2;
    const centerY = height - 20;
    const maxRadius = Math.min(width / 2, height - 40);
    const scale = maxRadius / 6000; // 6000mm (6m) max range

    // Draw Grid
    ctx.strokeStyle = 'rgba(0, 255, 191, 0.2)';
    ctx.lineWidth = 1;

    // Arcs (1m, 2m, 3m, 4m, 5m, 6m)
    for (let i = 1; i <= 6; i++) {
      ctx.beginPath();
      ctx.arc(centerX, centerY, i * 1000 * scale, Math.PI, 2 * Math.PI);
      ctx.stroke();
    }

    // Radial lines (-60, -30, 0, 30, 60 degrees)
    const angles = [-60, -30, 0, 30, 60];
    angles.forEach(angle => {
      const rad = (angle - 90) * (Math.PI / 180);
      ctx.beginPath();
      ctx.moveTo(centerX, centerY);
      ctx.lineTo(
        centerX + Math.cos(rad) * maxRadius,
        centerY + Math.sin(rad) * maxRadius
      );
      ctx.stroke();
    });

    // Draw Targets
    if (this.latestFrame && this.latestFrame.targets) {
      this.latestFrame.targets.forEach(target => {
        // Convert mm to canvas coords
        // X is left/right, Y is distance (depth)
        const x = target.position_x_mm || 0;
        const y = target.position_y_mm || 0;

        // Map to canvas
        // x: 0 is center. -x is left.
        const canvasX = centerX + (x * scale);
        const canvasY = centerY - (y * scale);

        // Draw dot
        ctx.beginPath();
        ctx.arc(canvasX, canvasY, 6, 0, 2 * Math.PI);
        ctx.fillStyle = '#ff0055';
        ctx.fill();
        
        // Draw ripple/pulse
        ctx.beginPath();
        ctx.arc(canvasX, canvasY, 12, 0, 2 * Math.PI);
        ctx.strokeStyle = 'rgba(255, 0, 85, 0.4)';
        ctx.stroke();
      });
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

      if (!frames || !Array.isArray(frames)) {
        return;
      }

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

    this.emitLightState(frame.targets);
  }

  private emitLightState(targets: MmwaveTarget[]): void {
    const buckets = Array.from({ length: this.bucketCount }, (_, index) => ({
      nodeId: (index + 1).toString(), // Ensure string ID
      totalBulbs: RoomVisualizerComponent.LIGHTS_PER_NODE,
      activeBulbs: 0
    }));

    // If we only have 1 node, map ALL targets to it regardless of position
    if (this.bucketCount === 1) {
      if (targets.length > 0) {
        buckets[0].activeBulbs = RoomVisualizerComponent.LIGHTS_PER_NODE;
      }
    } else {
      // Distribute targets across buckets based on angle
      const segmentSize = (Math.PI * 2) / this.bucketCount;

      targets.forEach(target => {
        const angle = Math.atan2(target.position_y_mm ?? 0, target.position_x_mm ?? 0);
        const normalized = (angle + Math.PI * 2) % (Math.PI * 2);
        const bucketIndex = Math.min(
          this.bucketCount - 1,
          Math.floor(normalized / segmentSize)
        );

        buckets[bucketIndex].activeBulbs = Math.min(
          RoomVisualizerComponent.LIGHTS_PER_NODE,
          buckets[bucketIndex].activeBulbs + 1
        );
      });
    }

    this.lightStateChanged.emit({
      totalActive: targets.length,
      nodeStates: buckets
    });
  }
}
