import { Component, OnInit, inject } from '@angular/core';
import { RouterOutlet } from '@angular/router';
import { WebSocketService } from './core/services/websocket.service';
import { MqttService } from './core/services/mqtt.service';

@Component({
  selector: 'app-root',
  imports: [RouterOutlet],
  templateUrl: './app.component.html',
  styleUrl: './app.component.scss'
})
export class AppComponent implements OnInit {
  title = 'iotFrontend';

  private readonly ws = inject(WebSocketService);
  private readonly mqtt = inject(MqttService);

  ngOnInit(): void {
    // Ensure real-time channels connect as app boots
    try { this.ws.connect(); } catch {}
    try { this.mqtt.connect(); } catch {}
  }
}
