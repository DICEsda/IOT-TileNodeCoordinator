import { Injectable } from '@angular/core';
import * as THREE from 'three';
import { AvatarData, InputState, RoomDimensions } from '../room-visualizer.models';

@Injectable({
  providedIn: 'root'
})
export class AvatarManagerService {
  private avatar!: AvatarData;
  private roomDimensions!: RoomDimensions;

  get Avatar() { return this.avatar; }

  createAvatar(scene: THREE.Scene, roomDimensions: RoomDimensions): void {
    console.log('Creating avatar...');
    this.roomDimensions = roomDimensions;

    const avatarGroup = new THREE.Group();
    avatarGroup.position.set(0, 0, 0);

    // Head
    const headGeometry = new THREE.SphereGeometry(0.3, 16, 16);
    const headMaterial = new THREE.MeshBasicMaterial({ color: 0xffdbac });
    const head = new THREE.Mesh(headGeometry, headMaterial);
    head.position.y = 1.6;
    avatarGroup.add(head);

    // Torso
    const torsoGeometry = new THREE.BoxGeometry(0.4, 1.0, 0.25);
    const torsoMaterial = new THREE.MeshBasicMaterial({ color: 0x4a90e2 });
    const torso = new THREE.Mesh(torsoGeometry, torsoMaterial);
    torso.position.y = 0.9;
    avatarGroup.add(torso);

    // Left arm
    const armGeometry = new THREE.CylinderGeometry(0.1, 0.1, 0.7, 8);
    const armMaterial = new THREE.MeshBasicMaterial({ color: 0xffdbac });
    const leftArm = new THREE.Mesh(armGeometry, armMaterial);
    leftArm.position.set(-0.35, 1.2, 0);
    leftArm.rotation.z = Math.PI / 2.5;
    avatarGroup.add(leftArm);

    // Right arm
    const rightArm = new THREE.Mesh(armGeometry, armMaterial);
    rightArm.position.set(0.35, 1.2, 0);
    rightArm.rotation.z = -Math.PI / 2.5;
    avatarGroup.add(rightArm);

    // Left leg
    const legGeometry = new THREE.CylinderGeometry(0.12, 0.12, 0.8, 8);
    const legMaterial = new THREE.MeshBasicMaterial({ color: 0x2c2c2c });
    const leftLeg = new THREE.Mesh(legGeometry, legMaterial);
    leftLeg.position.set(-0.15, 0.3, 0);
    avatarGroup.add(leftLeg);

    // Right leg
    const rightLeg = new THREE.Mesh(legGeometry, legMaterial);
    rightLeg.position.set(0.15, 0.3, 0);
    avatarGroup.add(rightLeg);

    scene.add(avatarGroup);
    console.log('Avatar added to scene');

    this.avatar = {
      group: avatarGroup,
      position: new THREE.Vector3(0, 0, 0),
      direction: new THREE.Vector3(0, 0, -1),
      velocity: new THREE.Vector3(0, 0, 0),
      speed: 0,
      maxSpeed: 0.055,
      acceleration: 0.008
    };
    console.log('Avatar created successfully');
  }

  updateMovement(input: InputState): void {
    // Calculate input direction
    const inputDir = new THREE.Vector3(0, 0, 0);

    if (input.forward) inputDir.z -= 1;
    if (input.backward) inputDir.z += 1;
    if (input.left) inputDir.x -= 1;
    if (input.right) inputDir.x += 1;

    // Normalize and apply relative to avatar direction
    if (inputDir.length() > 0) {
      inputDir.normalize();
      this.avatar.velocity.copy(inputDir).multiplyScalar(this.avatar.maxSpeed);
      this.avatar.speed = this.avatar.maxSpeed;
    } else {
      // Deceleration
      this.avatar.velocity.multiplyScalar(0.9);
      this.avatar.speed *= 0.9;
    }

    // Calculate new position
    const newPosition = this.avatar.position.clone().add(this.avatar.velocity);

    // Apply boundary constraints
    const margin = 0.5;
    const halfWidth = this.roomDimensions.width / 2 - margin;
    const halfDepth = this.roomDimensions.depth / 2 - margin;
    const minY = 0.5;
    const maxY = this.roomDimensions.height - 1;

    newPosition.x = Math.max(-halfWidth, Math.min(halfWidth, newPosition.x));
    newPosition.z = Math.max(-halfDepth, Math.min(halfDepth, newPosition.z));
    newPosition.y = Math.max(minY, Math.min(maxY, newPosition.y));

    // Update avatar position
    this.avatar.position.copy(newPosition);
    this.avatar.group.position.copy(newPosition);
  }

  rotateAvatar(deltaX: number): void {
    const rotationSpeed = 0.005;
    const currentYaw = Math.atan2(this.avatar.direction.x, this.avatar.direction.z);
    const newYaw = currentYaw + deltaX * rotationSpeed;

    this.avatar.direction.x = Math.sin(newYaw);
    this.avatar.direction.z = Math.cos(newYaw);
  }
}
