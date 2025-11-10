import { CommonModule } from '@angular/common';
import { Component, ElementRef, EventEmitter, OnDestroy, OnInit, Output, ViewChild } from '@angular/core';
import { RoomDimensions } from './room-visualizer.models';
import { AvatarManagerService } from './services/avatar-manager.service';
import { InputManagerService } from './services/input-manager.service';
import { LightingManagerService, LightNodeState } from './services/lighting-manager.service';
import { SceneManagerService } from './services/scene-manager.service';

@Component({
  selector: 'app-room-visualizer',
  imports: [CommonModule],
  templateUrl: './room-visualizer.component.html',
  styleUrl: './room-visualizer.component.scss'
})
export class RoomVisualizerComponent implements OnInit, OnDestroy {
  @ViewChild('canvas', { static: true }) canvasRef!: ElementRef<HTMLCanvasElement>;
  @Output() lightStateChanged = new EventEmitter<{
    totalActive: number;
    nodeStates: LightNodeState[];
  }>();

  legendMinimized: boolean = false;
  private animationId: number = 0;
  private orbitControlsEnabled: boolean = true;

  private roomDimensions: RoomDimensions = { width: 20, height: 8, depth: 15 };
  private boundWindowResize = this.onWindowResize.bind(this);

  constructor(
    private sceneManager: SceneManagerService,
    private avatarManager: AvatarManagerService,
    private lightingManager: LightingManagerService,
    private inputManager: InputManagerService
  ) {}

  ngOnInit() {
    setTimeout(() => {
      this.initializeScene();
      this.setupInputHandlers();
      this.animate();
    }, 100);
  }

  ngOnDestroy() {
    this.cleanup();
  }

  private initializeScene(): void {
    const canvas = this.canvasRef.nativeElement;

    // Initialize scene and create room
    this.sceneManager.initScene(canvas);
    this.sceneManager.createRoom(this.roomDimensions);

    // Create avatar and lights
    this.avatarManager.createAvatar(this.sceneManager.Scene, this.roomDimensions);
    this.lightingManager.createLightNodes(this.sceneManager.Scene);

    // Setup window resize listener
    window.addEventListener('resize', this.boundWindowResize);
  }

  private setupInputHandlers(): void {
    const canvas = this.canvasRef.nativeElement;
    this.inputManager.setupInputHandlers(canvas);
  }

  private animate = () => {
    this.animationId = requestAnimationFrame(this.animate);

    // Update avatar movement and detection
    this.avatarManager.updateMovement(this.inputManager.input);
    this.lightingManager.updateLightDetection(this.avatarManager.Avatar);

    // Emit light state changes
    const totalActive = this.lightingManager.getTotalActiveLights();
    const nodeStates = this.lightingManager.getLightNodeStates();
    this.lightStateChanged.emit({ totalActive, nodeStates });

    // Handle mouse-based avatar rotation
    const isMoving = this.inputManager.input.forward || this.inputManager.input.backward ||
                     this.inputManager.input.left || this.inputManager.input.right;

    if (!this.orbitControlsEnabled && isMoving) {
      const delta = this.inputManager.getMouseDelta();
      this.avatarManager.rotateAvatar(delta.x);
    }

    // Update orbit controls if enabled
    if (this.orbitControlsEnabled) {
      this.sceneManager.Controls.update();
    }

    this.sceneManager.Renderer.render(this.sceneManager.Scene, this.sceneManager.Camera);
  };

  private onWindowResize(): void {
    const canvas = this.canvasRef.nativeElement;
    this.sceneManager.onWindowResize(canvas);
  }

  private cleanup(): void {
    if (this.animationId) {
      cancelAnimationFrame(this.animationId);
    }
    window.removeEventListener('resize', this.boundWindowResize);
    const canvas = this.canvasRef.nativeElement;
    this.inputManager.removeInputHandlers(canvas);
    this.sceneManager.dispose();
  }

  toggleLegend(): void {
    this.legendMinimized = !this.legendMinimized;
  }
}

