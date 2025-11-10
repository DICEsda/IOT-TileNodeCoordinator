import { CommonModule } from '@angular/common';
import { Component, signal } from '@angular/core';

interface CalibrationStep {
  id: number;
  title: string;
  description: string;
  status: 'pending' | 'running' | 'completed' | 'failed';
  progress: number;
}

interface NodeCalibration {
  nodeId: number;
  nodeName: string;
  isCalibrating: boolean;
  lastCalibrated: Date | null;
  status: 'success' | 'warning' | 'error' | 'idle';
}

@Component({
  selector: 'app-calibrate',
  imports: [CommonModule],
  templateUrl: './calibrate.component.html',
  styleUrl: './calibrate.component.scss'
})
export class CalibrateComponent {
  calibrationSteps = signal<CalibrationStep[]>([
    {
      id: 1,
      title: 'Motion Sensor Baseline',
      description: 'Establishing baseline readings for motion sensors',
      status: 'pending',
      progress: 0
    },
    {
      id: 2,
      title: 'Light Sensor Calibration',
      description: 'Calibrating ambient light sensors',
      status: 'pending',
      progress: 0
    },
    {
      id: 3,
      title: 'Distance Measurement',
      description: 'Measuring detection range and distance accuracy',
      status: 'pending',
      progress: 0
    },
    {
      id: 4,
      title: 'Response Time Test',
      description: 'Testing sensor response time and latency',
      status: 'pending',
      progress: 0
    },
    {
      id: 5,
      title: 'Integration Test',
      description: 'Validating sensor integration and communication',
      status: 'pending',
      progress: 0
    }
  ]);

  nodes = signal<NodeCalibration[]>([
    { nodeId: 1, nodeName: 'Node 01 - Front Left', isCalibrating: false, lastCalibrated: new Date(Date.now() - 1000 * 60 * 60 * 24 * 2), status: 'success' },
    { nodeId: 2, nodeName: 'Node 02 - Front Center', isCalibrating: false, lastCalibrated: new Date(Date.now() - 1000 * 60 * 60 * 24), status: 'success' },
    { nodeId: 3, nodeName: 'Node 03 - Front Right', isCalibrating: false, lastCalibrated: new Date(Date.now() - 1000 * 60 * 60 * 24 * 5), status: 'warning' },
    { nodeId: 4, nodeName: 'Node 04 - Back Left', isCalibrating: false, lastCalibrated: new Date(Date.now() - 1000 * 60 * 60 * 24 * 10), status: 'error' },
    { nodeId: 5, nodeName: 'Node 05 - Back Center', isCalibrating: false, lastCalibrated: new Date(Date.now() - 1000 * 60 * 60 * 12), status: 'success' },
    { nodeId: 6, nodeName: 'Node 06 - Back Right', isCalibrating: false, lastCalibrated: null, status: 'idle' }
  ]);

  isCalibrating = signal(false);
  currentStep = signal(0);

  startFullCalibration() {
    this.isCalibrating.set(true);
    this.currentStep.set(0);

    const steps = this.calibrationSteps();
    steps.forEach(step => {
      step.status = 'pending';
      step.progress = 0;
    });
    this.calibrationSteps.set([...steps]);

    this.runCalibrationSteps();
  }

  private runCalibrationSteps() {
    const steps = this.calibrationSteps();
    const currentStepIndex = this.currentStep();

    if (currentStepIndex >= steps.length) {
      this.isCalibrating.set(false);
      alert('Full calibration completed successfully!');
      return;
    }

    const step = steps[currentStepIndex];
    step.status = 'running';
    this.calibrationSteps.set([...steps]);

    // Simulate calibration progress
    const interval = setInterval(() => {
      step.progress += 10;
      this.calibrationSteps.set([...steps]);

      if (step.progress >= 100) {
        clearInterval(interval);
        step.status = 'completed';
        this.calibrationSteps.set([...steps]);
        this.currentStep.set(currentStepIndex + 1);

        setTimeout(() => this.runCalibrationSteps(), 500);
      }
    }, 300);
  }

  calibrateNode(nodeId: number) {
    const nodes = this.nodes();
    const node = nodes.find(n => n.nodeId === nodeId);

    if (node) {
      node.isCalibrating = true;
      this.nodes.set([...nodes]);

      setTimeout(() => {
        node.isCalibrating = false;
        node.lastCalibrated = new Date();
        node.status = 'success';
        this.nodes.set([...nodes]);
      }, 3000);
    }
  }

  formatLastCalibrated(date: Date | null): string {
    if (!date) return 'Never calibrated';

    const now = new Date();
    const diff = now.getTime() - date.getTime();
    const days = Math.floor(diff / (1000 * 60 * 60 * 24));
    const hours = Math.floor(diff / (1000 * 60 * 60));

    if (days === 0) {
      if (hours === 0) return 'Just now';
      if (hours === 1) return '1 hour ago';
      return `${hours} hours ago`;
    }

    if (days === 1) return '1 day ago';
    return `${days} days ago`;
  }
}

