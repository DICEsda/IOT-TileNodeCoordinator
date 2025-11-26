import { Component, ElementRef, OnInit, OnDestroy, ViewChild, input } from '@angular/core';
import { CommonModule } from '@angular/common';
import * as BABYLON from '@babylonjs/core';
import '@babylonjs/core/Meshes/Builders/polygonBuilder';
import * as earcut from 'earcut';
import { Subscription } from 'rxjs';
import { MqttService } from '../../../../core/services/mqtt.service';
import { MmwaveFrame, MmwaveTarget } from '../../../../core/models/api.models';

interface TrailParticle {
  position: BABYLON.Vector3;
  createdAt: number;
  targetId: number;
}

@Component({
  selector: 'app-radar-3d',
  standalone: true,
  imports: [CommonModule],
  templateUrl: './radar-3d.component.html',
  styleUrls: ['./radar-3d.component.scss']
})
export class Radar3DComponent implements OnInit, OnDestroy {
  @ViewChild('renderCanvas', { static: true }) canvasRef!: ElementRef<HTMLCanvasElement>;
  
  siteId = input.required<string>();
  coordinatorId = input.required<string>();

  // Babylon.js core
  private engine!: BABYLON.Engine;
  private scene!: BABYLON.Scene;
  private camera!: BABYLON.ArcRotateCamera;
  
  // Radar elements
  private targetMeshes = new Map<number, BABYLON.Mesh>();
  private trailParticles: TrailParticle[] = [];
  private sweepLine!: BABYLON.LinesMesh;
  private sweepAngle = 0;
  
  // Data
  latestFrame: MmwaveFrame | null = null;
  presenceDetected = false;
  targetCount = 0;
  lastUpdated: Date | null = null;
  
  private mqttSubscription?: Subscription;
  private readonly MAX_TRAIL_AGE_MS = 3000;
  private readonly DETECTION_RANGE_M = 6;

  constructor(private mqtt: MqttService) {
    console.log('[Radar3D] Constructor called');
    // Register earcut with BabylonJS - must be set before creating polygon
    if (typeof (BABYLON as any).PolygonMeshBuilder !== 'undefined') {
      (BABYLON as any).PolygonMeshBuilder.prototype.bjsEarcut = earcut.default || earcut;
    }
  }

  ngOnInit(): void {
    console.log('[Radar3D] Initializing with siteId:', this.siteId(), 'coordinatorId:', this.coordinatorId());
    this.initBabylon();
    this.subscribeMqtt();
  }

  ngOnDestroy(): void {
    if (this.mqttSubscription) {
      this.mqttSubscription.unsubscribe();
    }
    if (this.engine) {
      this.engine.dispose();
    }
  }

  private initBabylon(): void {
    const canvas = this.canvasRef.nativeElement;
    this.engine = new BABYLON.Engine(canvas, true, { preserveDrawingBuffer: true, stencil: true });
    this.scene = new BABYLON.Scene(this.engine);
    
    // Black background with subtle gradient
    this.scene.clearColor = new BABYLON.Color4(0.05, 0.05, 0.05, 1);
    
    // Camera setup - angled 3D perspective
    this.camera = new BABYLON.ArcRotateCamera(
      'camera',
      Math.PI / 4,  // alpha (horizontal rotation)
      Math.PI / 3,  // beta (vertical angle)
      12,           // radius
      BABYLON.Vector3.Zero(),
      this.scene
    );
    this.camera.attachControl(canvas, true);
    this.camera.lowerRadiusLimit = 5;
    this.camera.upperRadiusLimit = 20;
    this.camera.lowerBetaLimit = 0.1;
    this.camera.upperBetaLimit = Math.PI / 2;
    
    // Lighting
    const ambientLight = new BABYLON.HemisphericLight('ambient', new BABYLON.Vector3(0, 1, 0), this.scene);
    ambientLight.intensity = 0.5;
    
    const dirLight = new BABYLON.DirectionalLight('dir', new BABYLON.Vector3(-1, -2, -1), this.scene);
    dirLight.intensity = 0.7;
    
    // Build radar environment
    this.createRadarGrid();
    this.createSweepLine();
    
    // Render loop
    this.engine.runRenderLoop(() => {
      this.updateSweep();
      this.updateTrails();
      this.scene.render();
    });
    
    // Handle resize
    window.addEventListener('resize', () => {
      this.engine.resize();
    });
  }

  private createRadarGrid(): void {
    // HLK-LD2450 specs: 120° detection angle, 6m range
    const detectionAngle = 120 * (Math.PI / 180); // 120 degrees in radians
    const halfAngle = detectionAngle / 2;
    
    // Ground plane using disc instead of polygon to avoid earcut dependency
    const groundMat = new BABYLON.StandardMaterial('groundMat', this.scene);
    groundMat.diffuseColor = new BABYLON.Color3(0.05, 0.05, 0.05);
    groundMat.specularColor = new BABYLON.Color3(0, 0, 0);
    groundMat.emissiveColor = new BABYLON.Color3(0.02, 0.02, 0.02);
    groundMat.alpha = 0.3;
    
    // Create a disc for the detection area (simpler than polygon)
    const cone = BABYLON.MeshBuilder.CreateDisc('detectionCone', { 
      radius: this.DETECTION_RANGE_M, 
      tessellation: 32,
      arc: 120 / 360  // 120 degree arc
    }, this.scene);
    cone.rotation.x = Math.PI / 2; // Rotate to lie flat
    cone.position.y = 0.01;
    cone.material = groundMat;
    
    // Concentric arc rings (on XZ plane)
    const ringMat = new BABYLON.StandardMaterial('ringMat', this.scene);
    ringMat.emissiveColor = new BABYLON.Color3(0.3, 0.3, 0.3);
    ringMat.disableLighting = true;
    ringMat.alpha = 0.5;
    
    for (let i = 1; i <= this.DETECTION_RANGE_M; i++) {
      const arcPoints = [];
      const arcSegments = 32;
      for (let j = 0; j <= arcSegments; j++) {
        const angle = -halfAngle + (detectionAngle * j / arcSegments);
        const x = i * Math.sin(angle);
        const z = i * Math.cos(angle);
        arcPoints.push(new BABYLON.Vector3(x, 0.02, z));
      }
      
      const arc = BABYLON.MeshBuilder.CreateLines(`ring${i}`, { points: arcPoints }, this.scene);
      arc.color = new BABYLON.Color3(0.3, 0.3, 0.3);
    }
    
    // Radial grid lines from center
    const radialMat = new BABYLON.StandardMaterial('radialMat', this.scene);
    radialMat.emissiveColor = new BABYLON.Color3(0.2, 0.2, 0.2);
    radialMat.disableLighting = true;
    radialMat.alpha = 0.3;
    
    const radialAngles = [-60, -45, -30, 0, 30, 45, 60]; // Degrees
    radialAngles.forEach(deg => {
      const angle = deg * (Math.PI / 180);
      const points = [
        new BABYLON.Vector3(0, 0.02, 0),
        new BABYLON.Vector3(
          this.DETECTION_RANGE_M * Math.sin(angle),
          0.02,
          this.DETECTION_RANGE_M * Math.cos(angle)
        )
      ];
      const line = BABYLON.MeshBuilder.CreateLines(`radial${deg}`, { points }, this.scene);
      line.color = new BABYLON.Color3(0.2, 0.2, 0.2);
    });
    
    // Radar sensor indicator at origin (small glowing sphere)
    const radarSensor = BABYLON.MeshBuilder.CreateSphere('radarSensor', { diameter: 0.2 }, this.scene);
    const radarMat = new BABYLON.StandardMaterial('radarMat', this.scene);
    radarMat.emissiveColor = new BABYLON.Color3(0.8, 0.8, 1);
    radarMat.disableLighting = true;
    radarSensor.material = radarMat;
    radarSensor.position.set(0, 0.1, 0);
    
    // Detection cone edges (boundary lines)
    const leftEdge = [
      new BABYLON.Vector3(0, 0.02, 0),
      new BABYLON.Vector3(
        this.DETECTION_RANGE_M * Math.sin(-halfAngle),
        0.02,
        this.DETECTION_RANGE_M * Math.cos(-halfAngle)
      )
    ];
    const rightEdge = [
      new BABYLON.Vector3(0, 0.02, 0),
      new BABYLON.Vector3(
        this.DETECTION_RANGE_M * Math.sin(halfAngle),
        0.02,
        this.DETECTION_RANGE_M * Math.cos(halfAngle)
      )
    ];
    
    const leftLine = BABYLON.MeshBuilder.CreateLines('leftEdge', { points: leftEdge }, this.scene);
    leftLine.color = new BABYLON.Color3(1, 1, 1);
    const rightLine = BABYLON.MeshBuilder.CreateLines('rightEdge', { points: rightEdge }, this.scene);
    rightLine.color = new BABYLON.Color3(1, 1, 1);
  }

  private createSweepLine(): void {
    // Create sweep line from origin
    const points = [
      new BABYLON.Vector3(0, 0.05, 0),
      new BABYLON.Vector3(0, 0.05, this.DETECTION_RANGE_M)
    ];
    
    const sweepLine = BABYLON.MeshBuilder.CreateLines('sweep', { points }, this.scene) as BABYLON.LinesMesh;
    sweepLine.color = new BABYLON.Color3(1, 1, 1);
    sweepLine.alpha = 0.3;
    this.sweepLine = sweepLine;
  }

  private updateSweep(): void {
    // Sweep within 120° cone (-60° to +60°)
    const maxAngle = 60 * (Math.PI / 180);
    const sweepSpeed = 0.015;
    
    this.sweepAngle += sweepSpeed;
    if (this.sweepAngle > maxAngle) {
      this.sweepAngle = -maxAngle;
    }
    
    this.sweepLine.rotation.y = this.sweepAngle;
  }

  private subscribeMqtt(): void {
    const topic = `site/${this.siteId()}/coord/${this.coordinatorId()}/mmwave`;
    console.log('[Radar3D] Subscribing to topic:', topic);
    console.log('[Radar3D] MQTT connected:', this.mqtt.connected());
    
    const subject = this.mqtt.subscribeCoordinatorMmwave(this.siteId(), this.coordinatorId());
    this.mqttSubscription = subject.subscribe({
      next: (payload) => {
        console.log('[Radar3D] Raw MQTT payload:', payload);
        const frame = this.normalizeFrame(payload);
        console.log('[Radar3D] Normalized frame:', frame);
        this.applyFrame(frame);
      },
      error: (err) => console.error('[Radar3D] MQTT error', err),
      complete: () => console.log('[Radar3D] MQTT subscription completed')
    });
    
    console.log('[Radar3D] Subscription created');
  }

  private normalizeFrame(payload: any): MmwaveFrame {
    // Targets are in payload.targets array, not in events
    const rawTargets = Array.isArray(payload?.targets) ? payload.targets : [];
    
    const targets: MmwaveTarget[] = rawTargets.map((t: any, idx: number) => {
      return {
        id: t.id ?? idx,
        distance_mm: t.distance_mm ?? t.dist_mm ?? 0,
        position_x_mm: t.position_mm?.x ?? t.position_x_mm ?? t.x_mm ?? 0,
        position_y_mm: t.position_mm?.y ?? t.position_y_mm ?? t.y_mm ?? 0,
        velocity_x_m_s: t.velocity_m_s?.x ?? t.velocity_x_m_s ?? t.vx ?? 0,
        velocity_y_m_s: t.velocity_m_s?.y ?? t.velocity_y_m_s ?? t.vy ?? 0,
        resolution_mm: t.resolution_mm ?? 0
      };
    });

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

  private applyFrame(frame: MmwaveFrame): void {
    this.latestFrame = frame;
    this.targetCount = frame.targets.length;
    this.presenceDetected = frame.targets.length > 0;
    this.lastUpdated = new Date();
    
    // Update target meshes
    this.updateTargets(frame.targets);
  }

  private updateTargets(targets: MmwaveTarget[]): void {
    const now = Date.now();
    const activeIds = new Set<number>();
    
    targets.forEach((target, index) => {
      if (index >= 3) return; // Max 3 targets
      
      activeIds.add(target.id);
      
      // Convert mm to meters - X is horizontal, Y is depth
      const x = target.position_x_mm / 1000;
      const z = target.position_y_mm / 1000; // Forward/back distance
      const position = new BABYLON.Vector3(x, 0.3, z); // Elevated for visibility
      
      // Get or create target mesh (single dot per target)
      let mesh = this.targetMeshes.get(target.id);
      if (!mesh) {
        mesh = BABYLON.MeshBuilder.CreateSphere(`target${target.id}`, { diameter: 0.2 }, this.scene);
        const mat = new BABYLON.StandardMaterial(`targetMat${target.id}`, this.scene);
        
        // Brightness variation based on target index
        const brightness = 1 - (index * 0.2);
        mat.emissiveColor = new BABYLON.Color3(brightness, brightness, brightness);
        mat.disableLighting = true;
        
        // Add glow
        const glow = new BABYLON.GlowLayer('glow', this.scene);
        glow.intensity = 0.8;
        
        mesh.material = mat;
        this.targetMeshes.set(target.id, mesh);
      }
      
      // Smooth position update
      if (mesh.position) {
        mesh.position = BABYLON.Vector3.Lerp(mesh.position, position, 0.3);
      } else {
        mesh.position = position;
      }
      
      // Add trail particle
      this.trailParticles.push({
        position: mesh.position.clone(),
        createdAt: now,
        targetId: target.id
      });
    });
    
    // Remove inactive targets
    this.targetMeshes.forEach((mesh, id) => {
      if (!activeIds.has(id)) {
        mesh.dispose();
        this.targetMeshes.delete(id);
      }
    });
  }

  private updateTrails(): void {
    const now = Date.now();
    
    // Remove old particles and render active trails
    this.trailParticles = this.trailParticles.filter(particle => {
      const age = now - particle.createdAt;
      if (age > this.MAX_TRAIL_AGE_MS) {
        return false;
      }
      
      // Render fading particle
      const alpha = 1 - (age / this.MAX_TRAIL_AGE_MS);
      this.renderTrailParticle(particle, alpha);
      
      return true;
    });
  }

  private renderTrailParticle(particle: TrailParticle, alpha: number): void {
    // Create ephemeral particle visualization
    const particleMesh = BABYLON.MeshBuilder.CreateSphere('trail', { diameter: 0.1 }, this.scene);
    const mat = new BABYLON.StandardMaterial('trailMat', this.scene);
    mat.emissiveColor = new BABYLON.Color3(1, 1, 1);
    mat.alpha = alpha * 0.5;
    mat.disableLighting = true;
    particleMesh.material = mat;
    particleMesh.position = particle.position;
    
    // Dispose immediately (will be recreated next frame if still active)
    setTimeout(() => particleMesh.dispose(), 50);
  }
}
