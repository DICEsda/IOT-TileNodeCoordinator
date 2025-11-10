import { Injectable } from '@angular/core';
import * as THREE from 'three';
import { AvatarData, LightNode } from '../room-visualizer.models';

export interface LightNodeState {
  nodeId: number;
  totalBulbs: number;
  activeBulbs: number;
}

@Injectable({
  providedIn: 'root'
})
export class LightingManagerService {
  private lightNodes: LightNode[] = [];
  private detectionRadius: number = 3;

  get LightNodes() { return this.lightNodes; }

  createLightNodes(scene: THREE.Scene): void {
    console.log('Creating light nodes...');
    const nodePositions = [
      new THREE.Vector3(-6, 6, 5),      // Node 1 - Front Left
      new THREE.Vector3(0, 6, 5),       // Node 2 - Front Center
      new THREE.Vector3(6, 6, 5),       // Node 3 - Front Right
      new THREE.Vector3(-6, 6, -5),     // Node 4 - Back Left
      new THREE.Vector3(0, 6, -5),      // Node 5 - Back Center
      new THREE.Vector3(6, 6, -5)       // Node 6 - Back Right
    ];

    nodePositions.forEach((position, nodeId) => {
      const node: LightNode = {
        position,
        bulbs: [],
        nodeId: nodeId + 1
      };

      // Create 6 bulbs per node in a strip configuration
      for (let i = 0; i < 6; i++) {
        const bulbOffset = new THREE.Vector3((i - 2.5) * 0.4, 0, 0);
        const bulbPosition = position.clone().add(bulbOffset);
        const bulb = this.createBulb(bulbPosition);
        node.bulbs.push(bulb);
        scene.add(bulb.mesh);
        scene.add(bulb.light);
      }

      this.createMountingStrip(scene, position);
      this.lightNodes.push(node);
    });
    console.log('Light nodes created successfully, total bulbs:', this.lightNodes.length * 6);
  }

  private createBulb(position: THREE.Vector3) {
    const bulbGeometry = new THREE.SphereGeometry(0.15, 16, 16);
    const bulbMaterial = new THREE.MeshBasicMaterial({ color: 0x444444 });

    const bulbMesh = new THREE.Mesh(bulbGeometry, bulbMaterial);
    bulbMesh.position.copy(position);

    const pointLight = new THREE.PointLight(0xffffff, 0, 5);
    pointLight.position.copy(position);

    return {
      mesh: bulbMesh,
      light: pointLight,
      isOn: false
    };
  }

  private createMountingStrip(scene: THREE.Scene, position: THREE.Vector3): void {
    const stripGeometry = new THREE.BoxGeometry(2.8, 0.1, 0.2);
    const stripMaterial = new THREE.MeshBasicMaterial({ color: 0x2a2f3a });

    const strip = new THREE.Mesh(stripGeometry, stripMaterial);
    strip.position.copy(position);
    strip.position.y += 0.2;
    scene.add(strip);
  }

  updateLightDetection(avatar: AvatarData): void {
    const avatarPos = avatar.position;

    this.lightNodes.forEach(node => {
      node.bulbs.forEach(bulb => {
        const dx = bulb.mesh.position.x - avatarPos.x;
        const dz = bulb.mesh.position.z - avatarPos.z;
        const distance = Math.sqrt(dx * dx + dz * dz);

        const shouldBeOn = distance <= this.detectionRadius;
        this.toggleBulb(bulb, shouldBeOn);
      });
    });
  }

  private toggleBulb(bulb: any, isOn: boolean): void {
    bulb.isOn = isOn;
    const material = bulb.mesh.material as THREE.MeshBasicMaterial;

    if (isOn) {
      material.color.setHex(0xffff00);
      bulb.light.intensity = 2;
    } else {
      material.color.setHex(0x444444);
      bulb.light.intensity = 0;
    }
  }

  getLightNodeStates(): LightNodeState[] {
    return this.lightNodes.map(node => ({
      nodeId: node.nodeId,
      totalBulbs: node.bulbs.length,
      activeBulbs: node.bulbs.filter(bulb => bulb.isOn).length
    }));
  }

  getTotalActiveLights(): number {
    return this.lightNodes.reduce((total, node) => {
      return total + node.bulbs.filter(bulb => bulb.isOn).length;
    }, 0);
  }
}
