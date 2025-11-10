import { Injectable } from '@angular/core';
import * as THREE from 'three';
import { OrbitControls } from 'three/examples/jsm/controls/OrbitControls.js';
import { RoomDimensions } from '../room-visualizer.models';

@Injectable({
  providedIn: 'root'
})
export class SceneManagerService {
  private scene!: THREE.Scene;
  private camera!: THREE.PerspectiveCamera;
  private renderer!: THREE.WebGLRenderer;
  private controls!: OrbitControls;

  get Scene() { return this.scene; }
  get Camera() { return this.camera; }
  get Renderer() { return this.renderer; }
  get Controls() { return this.controls; }

  initScene(canvas: HTMLCanvasElement): void {
    if (canvas.clientWidth === 0 || canvas.clientHeight === 0) {
      console.warn('Canvas has no dimensions, retrying...');
      setTimeout(() => this.initScene(canvas), 100);
      return;
    }

    try {
      // Scene
      this.scene = new THREE.Scene();
      this.scene.background = new THREE.Color(0x000000);
      this.scene.fog = new THREE.Fog(0x000000, 10, 50);
      console.log('Scene created');

      // Camera
      this.camera = new THREE.PerspectiveCamera(
        60,
        canvas.clientWidth / canvas.clientHeight,
        0.1,
        1000
      );
      this.camera.position.set(15, 12, 15);
      this.camera.lookAt(0, 0, 0);
      console.log('Camera created and positioned');

      // Renderer
      this.renderer = new THREE.WebGLRenderer({
        canvas,
        antialias: true,
        alpha: false,
        powerPreference: 'high-performance'
      });
      this.renderer.setSize(canvas.clientWidth, canvas.clientHeight);
      this.renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
      this.renderer.shadowMap.enabled = false;
      console.log('Renderer created and configured');

      // Controls
      this.controls = new OrbitControls(this.camera, this.renderer.domElement);
      this.controls.enableDamping = true;
      this.controls.dampingFactor = 0.05;
      this.controls.maxPolarAngle = Math.PI / 2;
      this.controls.minDistance = 8;
      this.controls.maxDistance = 30;
      console.log('Controls initialized');

      // Ambient light
      const ambientLight = new THREE.AmbientLight(0xffffff, 1.0);
      this.scene.add(ambientLight);
      console.log('Ambient light added');

      console.log('THREE.js initialization complete');
    } catch (error) {
      console.error('Error initializing THREE.js:', error);
      throw error;
    }
  }

  createRoom(dimensions: RoomDimensions): void {
    console.log('Creating room...');
    const { width: roomWidth, height: roomHeight, depth: roomDepth } = dimensions;

    // Floor
    const floorGeometry = new THREE.PlaneGeometry(roomWidth, roomDepth);
    const floorMaterial = new THREE.MeshBasicMaterial({ color: 0x0f1419 });
    const floor = new THREE.Mesh(floorGeometry, floorMaterial);
    floor.rotation.x = -Math.PI / 2;
    this.scene.add(floor);
    console.log('Floor added');

    // Grid helper
    const gridHelper = new THREE.GridHelper(roomWidth, 20, 0x2a2f3a, 0x0f1419);
    gridHelper.position.y = 0.01;
    this.scene.add(gridHelper);
    console.log('Grid helper added');

    // Ceiling
    const ceilingGeometry = new THREE.PlaneGeometry(roomWidth, roomDepth);
    const ceilingMaterial = new THREE.MeshBasicMaterial({
      color: 0x1a1f28,
      side: THREE.DoubleSide
    });
    const ceiling = new THREE.Mesh(ceilingGeometry, ceilingMaterial);
    ceiling.rotation.x = Math.PI / 2;
    ceiling.position.y = roomHeight;
    this.scene.add(ceiling);
    console.log('Ceiling added');

    this.createWalls(roomWidth, roomHeight, roomDepth);
    console.log('Room created successfully');
  }

  private createWalls(width: number, height: number, depth: number): void {
    const wallMaterial = new THREE.MeshBasicMaterial({
      color: 0x151a23,
      side: THREE.DoubleSide
    });

    // Back wall
    const backWall = new THREE.Mesh(
      new THREE.PlaneGeometry(width, height),
      wallMaterial
    );
    backWall.position.set(0, height / 2, -depth / 2);
    this.scene.add(backWall);

    // Left wall
    const leftWall = new THREE.Mesh(
      new THREE.PlaneGeometry(depth, height),
      wallMaterial
    );
    leftWall.position.set(-width / 2, height / 2, 0);
    leftWall.rotation.y = Math.PI / 2;
    this.scene.add(leftWall);

    // Right wall
    const rightWall = new THREE.Mesh(
      new THREE.PlaneGeometry(depth, height),
      wallMaterial
    );
    rightWall.position.set(width / 2, height / 2, 0);
    rightWall.rotation.y = -Math.PI / 2;
    this.scene.add(rightWall);
  }

  onWindowResize(canvas: HTMLCanvasElement): void {
    this.camera.aspect = canvas.clientWidth / canvas.clientHeight;
    this.camera.updateProjectionMatrix();
    this.renderer.setSize(canvas.clientWidth, canvas.clientHeight);
  }

  dispose(): void {
    this.renderer?.dispose();
    this.controls?.dispose();
  }
}
