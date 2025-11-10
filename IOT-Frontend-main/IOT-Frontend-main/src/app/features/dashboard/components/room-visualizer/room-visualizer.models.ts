import * as THREE from 'three';

export interface LightBulb {
  mesh: THREE.Mesh;
  light: THREE.PointLight;
  isOn: boolean;
}

export interface LightNode {
  position: THREE.Vector3;
  bulbs: LightBulb[];
  nodeId: number;
}

export interface AvatarData {
  group: THREE.Group;
  position: THREE.Vector3;
  direction: THREE.Vector3;
  velocity: THREE.Vector3;
  speed: number;
  maxSpeed: number;
  acceleration: number;
}

export interface InputState {
  forward: boolean;
  backward: boolean;
  left: boolean;
  right: boolean;
  mouseX: number;
  mouseY: number;
  lastMouseX: number;
  lastMouseY: number;
}

export interface RoomDimensions {
  width: number;
  height: number;
  depth: number;
}
