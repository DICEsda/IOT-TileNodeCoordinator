import { Injectable } from '@angular/core';
import { InputState } from '../room-visualizer.models';

@Injectable({
  providedIn: 'root'
})
export class InputManagerService {
  input: InputState = {
    forward: false,
    backward: false,
    left: false,
    right: false,
    mouseX: 0,
    mouseY: 0,
    lastMouseX: 0,
    lastMouseY: 0
  };

  private boundKeyDown!: (e: KeyboardEvent) => void;
  private boundKeyUp!: (e: KeyboardEvent) => void;
  private boundMouseMove!: (e: MouseEvent) => void;
  private boundMouseDown!: (e: MouseEvent) => void;
  private boundMouseUp!: (e: MouseEvent) => void;

  constructor() {
    this.boundKeyDown = this.onKeyDown.bind(this);
    this.boundKeyUp = this.onKeyUp.bind(this);
    this.boundMouseMove = this.onMouseMove.bind(this);
    this.boundMouseDown = this.onMouseDown.bind(this);
    this.boundMouseUp = this.onMouseUp.bind(this);
  }

  setupInputHandlers(canvas: HTMLCanvasElement): void {
    window.addEventListener('keydown', this.boundKeyDown);
    window.addEventListener('keyup', this.boundKeyUp);
    canvas.addEventListener('mousemove', this.boundMouseMove);
    canvas.addEventListener('mousedown', this.boundMouseDown);
    canvas.addEventListener('mouseup', this.boundMouseUp);
  }

  removeInputHandlers(canvas: HTMLCanvasElement): void {
    window.removeEventListener('keydown', this.boundKeyDown);
    window.removeEventListener('keyup', this.boundKeyUp);
    canvas?.removeEventListener('mousemove', this.boundMouseMove);
    canvas?.removeEventListener('mousedown', this.boundMouseDown);
    canvas?.removeEventListener('mouseup', this.boundMouseUp);
  }

  private onKeyDown(e: KeyboardEvent): void {
    const key = e.key.toLowerCase();

    switch (key) {
      case 'w':
      case 'arrowup':
        e.preventDefault();
        this.input.forward = true;
        break;
      case 's':
      case 'arrowdown':
        e.preventDefault();
        this.input.backward = true;
        break;
      case 'a':
      case 'arrowleft':
        e.preventDefault();
        this.input.left = true;
        break;
      case 'd':
      case 'arrowright':
        e.preventDefault();
        this.input.right = true;
        break;
    }
  }

  private onKeyUp(e: KeyboardEvent): void {
    const key = e.key.toLowerCase();

    switch (key) {
      case 'w':
      case 'arrowup':
        this.input.forward = false;
        break;
      case 's':
      case 'arrowdown':
        this.input.backward = false;
        break;
      case 'a':
      case 'arrowleft':
        this.input.left = false;
        break;
      case 'd':
      case 'arrowright':
        this.input.right = false;
        break;
    }
  }

  private onMouseMove(e: MouseEvent): void {
    this.input.mouseX = e.clientX;
    this.input.mouseY = e.clientY;
    this.input.lastMouseX = this.input.mouseX;
    this.input.lastMouseY = this.input.mouseY;
  }

  private onMouseDown(): void {
    // Handled by component for state management
  }

  private onMouseUp(): void {
    // Handled by component for state management
  }

  getMouseDelta(): { x: number; y: number } {
    return {
      x: this.input.mouseX - this.input.lastMouseX,
      y: this.input.mouseY - this.input.lastMouseY
    };
  }
}
