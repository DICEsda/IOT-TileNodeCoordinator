import { Component, ElementRef, Input, OnChanges, OnInit, ViewChild, OnDestroy } from '@angular/core';
import { CommonModule } from '@angular/common';
import * as d3 from 'd3';
import { Node, Coordinator } from '../../../core/models/api.models';

interface GraphNode extends d3.SimulationNodeDatum {
  id: string;
  type: 'coordinator' | 'node';
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
        </div>
      </div>
    </div>
  `,
  styles: [`
    .graph-container {
      width: 100%;
      height: 400px;
      background: #1e1e1e;
      border-radius: 12px;
      position: relative;
      overflow: hidden;
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
          }
        }
      }
    }
  `]
})
export class NetworkGraphComponent implements OnInit, OnChanges, OnDestroy {
  @Input() nodes: Node[] = [];
  @Input() coordinators: Coordinator[] = [];

  @ViewChild('graphContainer', { static: true }) container!: ElementRef;

  private simulation!: d3.Simulation<GraphNode, GraphLink>;
  private svg!: d3.Selection<SVGSVGElement, unknown, null, undefined>;
  private width = 0;
  private height = 0;

  ngOnInit() {
    this.initGraph();
  }

  ngOnChanges() {
    if (this.simulation) {
      this.updateGraph();
    }
  }

  ngOnDestroy() {
    if (this.simulation) {
      this.simulation.stop();
    }
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
      this.simulation.force('center', d3.forceCenter(this.width / 2, this.height / 2));
      this.simulation.alpha(0.3).restart();
    }).observe(element);

    this.simulation = d3.forceSimulation<GraphNode>()
      .force('link', d3.forceLink<GraphNode, GraphLink>().id((d: GraphNode) => d.id).distance(100))
      .force('charge', d3.forceManyBody().strength(-300))
      .force('center', d3.forceCenter(this.width / 2, this.height / 2))
      .force('collide', d3.forceCollide().radius(30));

    this.updateGraph();
  }

  private updateGraph() {
    if (!this.svg) return;

    const graphNodes: GraphNode[] = [];
    const graphLinks: GraphLink[] = [];

    this.coordinators.forEach(c => {
      graphNodes.push({
        id: c.coord_id,
        type: 'coordinator',
        name: 'Coordinator',
        status: c.status,
        data: c
      });
    });

    this.nodes.forEach(n => {
      graphNodes.push({
        id: n.node_id,
        type: 'node',
        name: n.name || `Node ${n.node_id}`,
        status: n.status,
        data: n
      });

      if (this.coordinators.length > 0) {
        graphLinks.push({
          source: this.coordinators[0].coord_id,
          target: n.node_id,
          value: 1
        });
      }
    });

    this.simulation.nodes(graphNodes);
    const linkForce = this.simulation.force('link') as d3.ForceLink<GraphNode, GraphLink>;
    linkForce.links(graphLinks);

    this.svg.selectAll('*').remove();

    const link = this.svg.append('g')
      .attr('stroke', '#999')
      .attr('stroke-opacity', 0.6)
      .selectAll('line')
      .data(graphLinks)
      .join('line')
      .attr('stroke-width', 1);

    const node = this.svg.append('g')
      .attr('stroke', '#fff')
      .attr('stroke-width', 1.5)
      .selectAll('circle')
      .data(graphNodes)
      .join('circle')
      .attr('r', (d: GraphNode) => d.type === 'coordinator' ? 10 : 6)
      .attr('fill', (d: GraphNode) => d.type === 'coordinator' ? '#4caf50' : (d.status === 'online' ? '#2196f3' : '#f44336'))
      .call(this.drag(this.simulation));

    node.append('title')
      .text((d: GraphNode) => d.name);

    const labels = this.svg.append('g')
      .selectAll('text')
      .data(graphNodes)
      .join('text')
      .attr('dx', 12)
      .attr('dy', 4)
      .text((d: GraphNode) => d.name)
      .attr('fill', '#ccc')
      .style('font-size', '10px')
      .style('pointer-events', 'none');

    this.simulation.on('tick', () => {
      link
        .attr('x1', (d: GraphLink) => (d.source as GraphNode).x!)
        .attr('y1', (d: GraphLink) => (d.source as GraphNode).y!)
        .attr('x2', (d: GraphLink) => (d.target as GraphNode).x!)
        .attr('y2', (d: GraphLink) => (d.target as GraphNode).y!);

      node
        .attr('cx', (d: GraphNode) => d.x!)
        .attr('cy', (d: GraphNode) => d.y!);
        
      labels
        .attr('x', (d: GraphNode) => d.x!)
        .attr('y', (d: GraphNode) => d.y!);
    });
    
    this.simulation.alpha(1).restart();
  }

  private drag(simulation: d3.Simulation<GraphNode, GraphLink>) {
    function dragstarted(event: any) {
      if (!event.active) simulation.alphaTarget(0.3).restart();
      event.subject.fx = event.subject.x;
      event.subject.fy = event.subject.y;
    }

    function dragged(event: any) {
      event.subject.fx = event.x;
      event.subject.fy = event.y;
    }

    function dragended(event: any) {
      if (!event.active) simulation.alphaTarget(0);
      event.subject.fx = null;
      event.subject.fy = null;
    }

    return d3.drag<any, GraphNode>()
      .on('start', dragstarted)
      .on('drag', dragged)
      .on('end', dragended);
  }
}
