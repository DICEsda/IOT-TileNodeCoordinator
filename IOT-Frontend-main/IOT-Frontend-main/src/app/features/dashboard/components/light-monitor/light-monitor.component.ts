import { CommonModule } from '@angular/common';
import { Component, Input, OnChanges, signal, SimpleChanges } from '@angular/core';

interface Bulb {
  id: number;
  isOn: boolean;
}

interface LightNode {
  id: number;
  name: string;
  bulbs: Bulb[];
  totalOn: number;
  status: 'active' | 'idle' | 'offline';
}

interface LightNodeState {
  nodeId: number;
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

  private nodeNames = [
    'Node 01 - Front Left',
    'Node 02 - Front Center',
    'Node 03 - Front Right',
    'Node 04 - Back Left',
    'Node 05 - Back Center',
    'Node 06 - Back Right'
  ];

  nodes = signal<LightNode[]>([
    {
      id: 1,
      name: 'Node 01 - Front Left',
      bulbs: Array.from({ length: 6 }, (_, i) => ({ id: i + 1, isOn: false })),
      totalOn: 0,
      status: 'active'
    },
    {
      id: 2,
      name: 'Node 02 - Front Center',
      bulbs: Array.from({ length: 6 }, (_, i) => ({ id: i + 1, isOn: false })),
      totalOn: 0,
      status: 'active'
    },
    {
      id: 3,
      name: 'Node 03 - Front Right',
      bulbs: Array.from({ length: 6 }, (_, i) => ({ id: i + 1, isOn: false })),
      totalOn: 0,
      status: 'active'
    },
    {
      id: 4,
      name: 'Node 04 - Back Left',
      bulbs: Array.from({ length: 6 }, (_, i) => ({ id: i + 1, isOn: false })),
      totalOn: 0,
      status: 'active'
    },
    {
      id: 5,
      name: 'Node 05 - Back Center',
      bulbs: Array.from({ length: 6 }, (_, i) => ({ id: i + 1, isOn: false })),
      totalOn: 0,
      status: 'active'
    },
    {
      id: 6,
      name: 'Node 06 - Back Right',
      bulbs: Array.from({ length: 6 }, (_, i) => ({ id: i + 1, isOn: false })),
      totalOn: 0,
      status: 'active'
    }
  ]);

  totalLights = signal(36);
  activeLights = signal(0);

  get activeNodesCount(): number {
    return this.nodes().filter(n => n.status === 'active').length;
  }

  ngOnChanges(changes: SimpleChanges): void {
    if (changes['lightNodeStates'] || changes['totalActiveLights']) {
      this.updateFromLightNodeStates();
    }
  }

  private updateFromLightNodeStates(): void {
    if (this.lightNodeStates.length === 0) return;

    const updatedNodes = this.nodes().map(node => {
      const nodeState = this.lightNodeStates.find(s => s.nodeId === node.id);
      if (!nodeState) return node;

      // Simulate bulb states based on active count
      const updatedBulbs = node.bulbs.map((bulb, index) => ({
        ...bulb,
        isOn: index < nodeState.activeBulbs
      }));

      return {
        ...node,
        bulbs: updatedBulbs,
        totalOn: nodeState.activeBulbs,
        status: nodeState.activeBulbs > 0 ? ('active' as const) : ('idle' as const)
      };
    });

    this.nodes.set(updatedNodes);
    this.activeLights.set(this.totalActiveLights);
  }

  toggleNode(nodeId: number) {
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

  toggleBulb(nodeId: number, bulbId: number) {
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

