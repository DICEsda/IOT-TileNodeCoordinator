import { ApplicationConfig, provideExperimentalZonelessChangeDetection } from '@angular/core';
import { provideRouter } from '@angular/router';
import { routes } from './app.routes';

// Core services
import { EnvironmentService } from './core/services/environment.service';
import { ApiService } from './core/services/api.service';
import { WebSocketService } from './core/services/websocket.service';
import { MqttService } from './core/services/mqtt.service';

export const appConfig: ApplicationConfig = {
  providers: [
    provideExperimentalZonelessChangeDetection(),
    provideRouter(routes),
    EnvironmentService,
    ApiService,
    WebSocketService,
    MqttService
  ]
};
