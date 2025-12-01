import { Component, ElementRef, Input, OnChanges, OnInit, ViewChild, OnDestroy } from '@angular/core';
import { CommonModule } from '@angular/common';
import * as d3 from 'd3';
import { Node, Coordinator } from '../../../core/models/api.models';

interface GraphNode extends d3.SimulationNodeDatum {
  id: string;
  type: 'coordinator' | 'node' | 'backend' | 'frontend' | 'mongodb';
  name: string;
  status: string;
  data?: any;
  x?: number;
  y?: number;
  fx?: number | null;
  fy?: number | null;
}

interface GraphLink extends d3.SimulationLinkDatum<GraphNode> {
  source: string | GraphNode;
  target: string | GraphNode;
  value: number;
}

@Component({
  selector: 'app-network-graph',
  standalone: true,
  imports: [CommonModule],
  template: `
    <div class="graph-container" #graphContainer>
      <div class="graph-overlay">
        <h3>Network Topology</h3>
        <div class="legend">
          <span class="item"><span class="dot coord"></span> Coordinator</span>
          <span class="item"><span class="dot node"></span> Node</span>
          <span class="item"><span class="dot backend"></span> Backend</span>
          <span class="item"><span class="dot frontend"></span> Frontend</span>
          <span class="item"><span class="dot mongodb"></span> MongoDB</span>
        </div>
      </div>
    </div>
  `,
  styles: [`
    .graph-container {
      width: 100%;
      height: 500px;
      background: #1e1e1e;
      border-radius: 12px;
      position: relative;
      overflow: visible;
      border: 1px solid rgba(255, 255, 255, 0.1);
    }
    .graph-overlay {
      position: absolute;
      top: 16px;
      left: 16px;
      pointer-events: none;
      
      h3 {
        margin: 0 0 8px 0;
        font-size: 14px;
        font-weight: 500;
        color: rgba(255, 255, 255, 0.7);
      }
      
      .legend {
        display: flex;
        gap: 12px;
        
        .item {
          display: flex;
          align-items: center;
          gap: 6px;
          font-size: 12px;
          color: rgba(255, 255, 255, 0.5);
          
          .dot {
            width: 8px;
            height: 8px;
            border-radius: 50%;
            
            &.coord { background: #4caf50; }
            &.node { background: #2196f3; }
            &.backend { background: #ff9800; }
            &.frontend { background: #9c27b0; }
            &.mongodb { background: #00897b; }
          }
        }
      }
    }
  `]
})
export class NetworkGraphComponent implements OnInit, OnChanges, OnDestroy {
  @Input() nodes: Node[] = [];
  @Input() coordinators: Coordinator[] = [];
  @Input() backendHealthy: boolean = false;
  @Input() dbHealthy: boolean = false;
  @Input() frontendHealthy: boolean = true;

  @ViewChild('graphContainer', { static: true }) container!: ElementRef;

  private svg!: d3.Selection<SVGSVGElement, unknown, null, undefined>;
  private width = 0;
  private height = 0;

  ngOnInit() {
    this.initGraph();
  }

  ngOnChanges() {
    if (this.svg) {
      this.updateGraph();
    }
  }

  ngOnDestroy() {
    // No simulation to clean up
  }

  private initGraph() {
    const element = this.container.nativeElement;
    this.width = element.clientWidth;
    this.height = element.clientHeight;

    this.svg = d3.select(element)
      .append('svg')
      .attr('width', '100%')
      .attr('height', '100%')
      .attr('viewBox', [0, 0, this.width, this.height]);

    new ResizeObserver(() => {
      this.width = element.clientWidth;
      this.height = element.clientHeight;
      this.updateGraph();
    }).observe(element);

    // No force simulation - using fixed hierarchical layout
    this.updateGraph();
  }

  private updateGraph() {
    if (!this.svg) return;

    const graphNodes: GraphNode[] = [];
    const graphLinks: GraphLink[] = [];

    // Add infrastructure nodes (no fixed positions for draggability)
    graphNodes.push({
      id: 'mongodb',
      type: 'mongodb',
      name: 'MongoDB',
      status: this.dbHealthy ? 'online' : 'offline'
    });

    graphNodes.push({
      id: 'backend',
      type: 'backend',
      name: 'Backend',
      status: this.backendHealthy ? 'online' : 'offline'
    });

    graphNodes.push({
      id: 'frontend',
      type: 'frontend',
      name: 'Frontend',
      status: this.frontendHealthy ? 'online' : 'offline'
    });

    // Add infrastructure links
    graphLinks.push({
      source: 'mongodb',
      target: 'backend',
      value: 1
    });

    graphLinks.push({
      source: 'backend',
      target: 'frontend',
      value: 1
    });

    // Always show 4 coordinators - update status based on real data
    for (let i = 1; i <= 4; i++) {
      const coordId = `coord00${i}`;
      
      // Check if we have real coordinator data
      const realCoord = this.coordinators.find(c => c.coord_id === coordId);
      
      graphNodes.push({
        id: coordId,
        type: 'coordinator',
        name: `Coordinator ${i}`,
        status: realCoord ? realCoord.status : 'offline',
        data: realCoord
      });

      // Link coordinator to backend
      graphLinks.push({
        source: 'backend',
        target: coordId,
        value: 1
      });

      // Add 5 nodes for each coordinator
      for (let j = 1; j <= 5; j++) {
        const nodeId = `${coordId}_node${j}`;
        
        // Check if we have real node data (for first coordinator only since we don't have coordinator_id)
        const realNode = (i === 1 && this.nodes.length > 0 && j <= this.nodes.length) ? this.nodes[j - 1] : undefined;
        
        graphNodes.push({
          id: nodeId,
          type: 'node',
          name: realNode ? (realNode.name || `${i}.${j}`) : `${i}.${j}`,
          status: realNode ? realNode.status : 'offline',
          data: realNode
        });

        // Link node to its coordinator
        graphLinks.push({
          source: coordId,
          target: nodeId,
          value: 1
        });
      }
    }

    // Calculate fixed positions for hierarchical layout (well-spaced)
    graphNodes.forEach(node => {
      if (node.type === 'mongodb') {
        node.x = this.width * 0.25;
        node.y = this.height * 0.18;
      } else if (node.type === 'backend') {
        node.x = this.width * 0.42;
        node.y = this.height * 0.38;
      } else if (node.type === 'frontend') {
        node.x = this.width * 0.58;
        node.y = this.height * 0.38;
      } else if (node.type === 'coordinator') {
        const coordIndex = parseInt(node.id.replace('coord00', '')) - 1;
        node.x = this.width * (0.23 + coordIndex * 0.17);
        node.y = this.height * 0.6;
      } else if (node.type === 'node') {
        const match = node.id.match(/coord00(\d)_node(\d)/);
        if (match) {
          const coordIndex = parseInt(match[1]) - 1;
          const nodeIndex = parseInt(match[2]) - 1;
          node.x = this.width * (0.23 + coordIndex * 0.17) + (nodeIndex - 2) * 22;
          node.y = this.height * 0.82;
        }
      }
    });

    this.svg.selectAll('*').remove();

    const link = this.svg.append('g')
      .attr('stroke', '#999')
      .attr('stroke-opacity', 0.6)
      .selectAll('line')
      .data(graphLinks)
      .join('line')
      .attr('stroke-width', 1.5)
      .attr('x1', (d: GraphLink) => {
        const source = graphNodes.find(n => n.id === (typeof d.source === 'string' ? d.source : (d.source as any).id));
        return source?.x || 0;
      })
      .attr('y1', (d: GraphLink) => {
        const source = graphNodes.find(n => n.id === (typeof d.source === 'string' ? d.source : (d.source as any).id));
        return source?.y || 0;
      })
      .attr('x2', (d: GraphLink) => {
        const target = graphNodes.find(n => n.id === (typeof d.target === 'string' ? d.target : (d.target as any).id));
        return target?.x || 0;
      })
      .attr('y2', (d: GraphLink) => {
        const target = graphNodes.find(n => n.id === (typeof d.target === 'string' ? d.target : (d.target as any).id));
        return target?.y || 0;
      })
      .attr('stroke-opacity', (d: GraphLink) => {
        const source = graphNodes.find(n => n.id === (typeof d.source === 'string' ? d.source : (d.source as any).id));
        const target = graphNodes.find(n => n.id === (typeof d.target === 'string' ? d.target : (d.target as any).id));
        return (source?.status === 'online' && target?.status === 'online') ? 0.6 : 0.2;
      });

    const node = this.svg.append('g')
      .attr('stroke', '#fff')
      .attr('stroke-width', 1.5)
      .selectAll('circle')
      .data(graphNodes)
      .join('circle')
      .attr('cx', (d: GraphNode) => d.x || 0)
      .attr('cy', (d: GraphNode) => d.y || 0)
      .attr('r', (d: GraphNode) => {
        if (d.type === 'coordinator') return 10;
        if (['backend', 'frontend', 'mongodb'].includes(d.type)) return 12;
        return 6;
      })
      .attr('fill', (d: GraphNode) => {
        if (d.type === 'coordinator') return d.status === 'online' ? '#4caf50' : '#666';
        if (d.type === 'backend') return d.status === 'online' ? '#ff9800' : '#666';
        if (d.type === 'frontend') return d.status === 'online' ? '#9c27b0' : '#666';
        if (d.type === 'mongodb') return d.status === 'online' ? '#00897b' : '#666';
        return d.status === 'online' ? '#2196f3' : '#666';
      })
      .attr('opacity', (d: GraphNode) => d.status === 'online' ? 1 : 0.5);

    node.append('title')
      .text((d: GraphNode) => d.name);

    const labels = this.svg.append('g')
      .selectAll('text')
      .data(graphNodes)
      .join('text')
      .attr('x', (d: GraphNode) => d.x || 0)
      .attr('y', (d: GraphNode) => {
        const r = d.type === 'coordinator' ? 10 : (d.type === 'node' ? 6 : 12);
        return (d.y || 0) + r + 14;
      })
      .attr('text-anchor', 'middle')
      .text((d: GraphNode) => d.name)
      .attr('fill', (d: GraphNode) => d.status === 'online' ? '#ffffff' : '#999')
      .attr('font-weight', '600')
      .style('font-size', '11px')
      .style('letter-spacing', '0.5px')
      .style('pointer-events', 'none');
  }

}
