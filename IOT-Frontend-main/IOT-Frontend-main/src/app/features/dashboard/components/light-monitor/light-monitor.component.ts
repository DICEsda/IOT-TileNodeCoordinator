import { CommonModule } from '@angular/common';
import { Component, Input, OnChanges, signal, SimpleChanges } from '@angular/core';
import { Node } from '../../../../core/models/api.models';

interface Bulb {
  id: number;
  isOn: boolean;
}

interface LightNode {
  id: string; // Changed to string to match Node.node_id
  name: string;
  bulbs: Bulb[];
  totalOn: number;
  status: 'active' | 'idle' | 'offline';
  tempC?: number; // Temperature in Celsius
}

interface LightNodeState {
  nodeId: string; // Changed to string
  totalBulbs: number;
  activeBulbs: number;
}

@Component({
  selector: 'app-light-monitor',
  imports: [CommonModule],
  templateUrl: './light-monitor.component.html',
  styleUrl: './light-monitor.component.scss'
})
export class LightMonitorComponent implements OnChanges {
  @Input() totalActiveLights: number = 0;
  @Input() lightNodeStates: LightNodeState[] = [];
  @Input() registeredNodes: Node[] = []; // New input for real nodes

  nodes = signal<LightNode[]>([]);
  totalLights = signal(0);
  activeLights = signal(0);

  get activeNodesCount(): number {
    return this.nodes().filter(n => n.status === 'active').length;
  }

  ngOnChanges(changes: SimpleChanges): void {
    if (changes['registeredNodes']) {
      this.initializeNodes();
    }
    
    if (changes['lightNodeStates'] || changes['totalActiveLights']) {
      this.updateFromLightNodeStates();
    }
  }

  private initializeNodes(): void {
    let initialNodes: LightNode[] = [];

    if (this.registeredNodes && this.registeredNodes.length > 0) {
      // Use registered nodes
      initialNodes = this.registeredNodes.map(node => ({
        id: node.node_id,
        name: node.name || `Node ${node.node_id.substring(0, 4)}`,
        bulbs: Array.from({ length: 6 }, (_, i) => ({ id: i + 1, isOn: false })),
        totalOn: 0,
        status: node.status === 'online' ? 'idle' : 'offline',
        tempC: node.temperature
      }));
    } else {
      // Fallback to single node for development if no nodes registered
      initialNodes = [{
        id: 'node-default',
        name: 'Single Node (Dev)',
        bulbs: Array.from({ length: 6 }, (_, i) => ({ id: i + 1, isOn: false })),
        totalOn: 0,
        status: 'idle'
      }];
    }

    this.nodes.set(initialNodes);
    this.totalLights.set(initialNodes.length * 6);
  }

  private updateFromLightNodeStates(): void {
    if (this.nodes().length === 0) {
        this.initializeNodes();
    }

    const updatedNodes = this.nodes().map(node => {
      const nodeState = this.lightNodeStates.find(s => s.nodeId === node.id);
      
      // If no state update, keep current state but ensure status reflects connectivity
      if (!nodeState) return node;

      // Simulate bulb states based on active count
      const updatedBulbs = node.bulbs.map((bulb, index) => ({
        ...bulb,
        isOn: index < nodeState.activeBulbs
      }));

      // Get updated temperature from registeredNodes
      const registeredNode = this.registeredNodes.find(n => n.node_id === node.id);
      
      return {
        ...node,
        bulbs: updatedBulbs,
        totalOn: nodeState.activeBulbs,
        tempC: registeredNode?.temperature ?? node.tempC,
        status: nodeState.activeBulbs > 0 ? ('active' as const) : ('idle' as const)
      };
    });

    this.nodes.set(updatedNodes);
    this.activeLights.set(this.totalActiveLights);
  }

  toggleNode(nodeId: string) {
    const updatedNodes = this.nodes().map(node => {
      if (node.id === nodeId) {
        const allOn = node.bulbs.every(b => b.isOn);
        const updatedBulbs = node.bulbs.map(bulb => ({
          ...bulb,
          isOn: !allOn
        }));

        return {
          ...node,
          bulbs: updatedBulbs,
          totalOn: allOn ? 0 : 6,
          status: allOn ? ('idle' as const) : ('active' as const)
        };
      }
      return node;
    });

    this.nodes.set(updatedNodes);
  }

  toggleBulb(nodeId: string, bulbId: number) {
    const updatedNodes = this.nodes().map(node => {
      if (node.id === nodeId) {
        const updatedBulbs = node.bulbs.map(bulb => {
          if (bulb.id === bulbId) {
            return { ...bulb, isOn: !bulb.isOn };
          }
          return bulb;
        });

        const totalOn = updatedBulbs.filter(b => b.isOn).length;

        return {
          ...node,
          bulbs: updatedBulbs,
          totalOn,
          status: totalOn > 0 ? ('active' as const) : ('idle' as const)
        };
      }
      return node;
    });

    this.nodes.set(updatedNodes);
  }
}

