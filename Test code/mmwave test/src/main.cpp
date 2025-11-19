#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LD2450.h>
#include <cmath>
// for snprintf
#include <cstdio>

// ---------- Colorful logger ----------
#ifndef LOG_USE_COLOR
#define LOG_USE_COLOR 1
#endif

#if LOG_USE_COLOR
#define C_RESET "\033[0m"
#define C_DIM   "\033[2m"
#define C_TAG   "\033[90m"
#define C_INFO  "\033[36m"
#define C_WARN  "\033[33m"
#define C_ERROR "\033[31m"
#define C_CRIT  "\033[95;1m"
#else
#define C_RESET ""
#define C_DIM   ""
#define C_TAG   ""
#define C_INFO  ""
#define C_WARN  ""
#define C_ERROR ""
#define C_CRIT  ""
#endif

static inline void log_prefix() { Serial.printf("[%8lu] ", millis()); }

template <typename... Args>
static inline void logf(const char *level, const char *tag, const char *fmt, Args... args) {
  char buf[256];
  snprintf(buf, sizeof(buf), fmt, args...);
  log_prefix();
  const char *col = C_INFO;
  if (level[0] == 'W') col = C_WARN;
  else if (level[0] == 'E') col = C_ERROR;
  else if (level[0] == 'C') col = C_CRIT;
  Serial.printf("%s[%s][%s]%s %s\n", col, level, tag, C_RESET, buf);
}

#define LOGI(tag, fmt, ...) logf("I", tag, fmt, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...) logf("W", tag, fmt, ##__VA_ARGS__)
#define LOGE(tag, fmt, ...) logf("E", tag, fmt, ##__VA_ARGS__)
#define LOGC(tag, fmt, ...) logf("C", tag, fmt, ##__VA_ARGS__)

// Access Point configuration
constexpr char AP_SSID[] = "ESP32-Radar";
constexpr char AP_PASSWORD[] = "radar12345";  // Min 8 characters

// STA credentials: fill these with your network to have the ESP connect as a station.
// If left empty the ESP will fall back to AP mode.
const char WIFI_SSID[] = "Pixel_3935"; // e.g. "MyHomeWiFi"
const char WIFI_PASSWORD[] = "12341234"; // e.g. "supersecret"

// HLK-LD2450 wiring (adjust to match your ESP32-S3 wiring)
constexpr int RADAR_UART_RX_PIN = 44; // Sensor TX -> ESP32-S3 GPIO17 (UART1 RX)
constexpr int RADAR_UART_TX_PIN = 43; // Sensor RX -> ESP32-S3 GPIO18 (UART1 TX)

// Telemetry configuration
constexpr size_t HISTORY_SIZE = 180;      // ~9 seconds of history at 50 ms cadence
// Set to 0 to push every new sensor frame immediately (highest possible rate)
constexpr uint32_t FRAME_INTERVAL_MS = 0;

HardwareSerial RadarSerial(1);
LD2450 radar;
AsyncWebServer server(80);
AsyncWebSocket radarSocket("/radar");
volatile uint32_t wsClientCount = 0;

struct TargetFrame {
  uint32_t timestamp_ms;
  LD2450::RadarTarget targets[LD2450_MAX_SENSOR_TARGETS];
};

TargetFrame historyBuffer[HISTORY_SIZE];
size_t historyCount = 0;
size_t historyIndex = 0;
TargetFrame lastFrame = {};
bool hasFrame = false;
unsigned long lastBroadcast = 0;
unsigned long lastSerialPrint = 0;
int lastReadStatus = -99;
unsigned long lastReadStatusLog = 0;
unsigned long lastHeartbeat = 0;

void connectToWiFi();
void initialiseRadar();
void addFrame(const LD2450::RadarTarget *targets);
String buildFrameJson(const TargetFrame &frame);
String buildHistoryJson();
void broadcastLatestFrame();
void handleSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                       void *arg, uint8_t *data, size_t len);

void setup() {
  Serial.begin(115200);
  delay(200);
  LOGI("BOOT", "HLK_LD2450 telemetry starting");

  // Log WiFi events
  WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
    switch(event){
      default: LOGI("WIFI", "event=%d", (int)event); break;
#ifdef ARDUINO_EVENT_WIFI_STA_START
      case ARDUINO_EVENT_WIFI_STA_START: LOGI("WIFI", "sta=start"); break;
      case ARDUINO_EVENT_WIFI_STA_CONNECTED: LOGI("WIFI", "sta=connected"); break;
      case ARDUINO_EVENT_WIFI_STA_GOT_IP: LOGI("WIFI", "sta=got_ip ip=%s", WiFi.localIP().toString().c_str()); break;
      case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: LOGW("WIFI", "sta=disconnected reason=%d", (int)info.wifi_sta_disconnected.reason); break;
      case ARDUINO_EVENT_WIFI_AP_START: LOGI("WIFI", "ap=start ssid=%s ip=%s", AP_SSID, WiFi.softAPIP().toString().c_str()); break;
      case ARDUINO_EVENT_WIFI_AP_STACONNECTED: LOGI("WIFI", "ap=join mac=%02x:%02x:%02x:%02x:%02x:%02x",
        info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1], info.wifi_ap_staconnected.mac[2],
        info.wifi_ap_staconnected.mac[3], info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]); break;
      case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED: LOGW("WIFI", "ap=leave mac=%02x:%02x:%02x:%02x:%02x:%02x",
        info.wifi_ap_stadisconnected.mac[0], info.wifi_ap_stadisconnected.mac[1], info.wifi_ap_stadisconnected.mac[2],
        info.wifi_ap_stadisconnected.mac[3], info.wifi_ap_stadisconnected.mac[4], info.wifi_ap_stadisconnected.mac[5]); break;
#endif
    }
  });

  connectToWiFi();
  initialiseRadar();

  radarSocket.onEvent(handleSocketEvent);
  server.addHandler(&radarSocket);

  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain",
                  "HLK-LD2450 radar telemetry running. Connect via WebSocket at /radar or REST at /api/frame.");
  });

  server.on("/api/frame", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (!hasFrame) {
      request->send(503, "application/json", "{\"message\":\"no data yet\"}");
      return;
    }
    request->send(200, "application/json", buildFrameJson(lastFrame));
  });

  server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", buildHistoryJson());
  });

  server.begin();
  LOGI("HTTP", "Server ready. STA IP=%s AP IP=%s", WiFi.localIP().toString().c_str(), WiFi.softAPIP().toString().c_str());
}

void loop() {
  const int readStatus = radar.read();
  const unsigned long now = millis();

  // Log radar read status changes to help debugging
  if (readStatus != lastReadStatus && (now - lastReadStatusLog) > 200) {
    lastReadStatusLog = now;
    lastReadStatus = readStatus;
    if (readStatus >= 0) {
      LOGI("SENSOR", "read=%d (targets updated)", readStatus);
    } else if (readStatus == -1) {
      LOGI("SENSOR", "read: no data available");
    } else if (readStatus == -2) {
      LOGE("SENSOR", "read: UART not initialized");
    } else {
      LOGW("SENSOR", "read: code=%d", readStatus);
    }
  }

  if (readStatus >= 0 && (FRAME_INTERVAL_MS == 0 || (now - lastBroadcast) >= FRAME_INTERVAL_MS)) {
    LD2450::RadarTarget snapshot[LD2450_MAX_SENSOR_TARGETS];
    for (size_t i = 0; i < LD2450_MAX_SENSOR_TARGETS; ++i) {
      snapshot[i] = radar.getTarget(i);
    }
    addFrame(snapshot);
    lastBroadcast = now;
  }

  radarSocket.cleanupClients();

  // Periodic serial debug output (every 250ms) with a compact target summary.
  if (hasFrame && (now - lastSerialPrint) >= 250) {
    lastSerialPrint = now;
    log_prefix();
    Serial.print(C_INFO "[I]" C_RESET "[SENSOR] ts=");
    Serial.print(lastFrame.timestamp_ms);
    Serial.print(' ');
    for (size_t i = 0; i < LD2450_MAX_SENSOR_TARGETS; ++i) {
      const auto &t = lastFrame.targets[i];
      Serial.print("T"); Serial.print(t.id); Serial.print('=');
      Serial.print(t.valid ? 'V' : '-');
      if (t.valid) {
        Serial.print(",x="); Serial.print(t.x);
        Serial.print(",y="); Serial.print(t.y);
        Serial.print(",d="); Serial.print(t.distance);
        Serial.print(",v="); Serial.print(t.speed);
      }
      if (i + 1 < LD2450_MAX_SENSOR_TARGETS) Serial.print(" | ");
    }
    Serial.print('\n');
  }

  // Heartbeat every 2 seconds: shows basic liveness and connectivity
  if ((now - lastHeartbeat) >= 2000) {
    lastHeartbeat = now;
  LOGI("HB", "alive sta=%s ap=%s wifi=%d ws=%u",
         WiFi.localIP().toString().c_str(),
         WiFi.softAPIP().toString().c_str(),
         (int)WiFi.status(),
         (unsigned)wsClientCount);
  }
}

void connectToWiFi() {
  // If STA credentials are provided, try to connect as a station first.
  if (strlen(WIFI_SSID) > 0) {
    Serial.print("Attempting to connect to WiFi SSID: ");
    Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    uint32_t start = millis();
    const uint32_t timeout = 20000; // 20s
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout) {
      delay(250);
      Serial.print('.');
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi.");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      return;
    }

    Serial.println("\nFailed to connect to WiFi, falling back to Access Point mode.");
  }

  // Start Access Point
  WiFi.mode(WIFI_AP);
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  WiFi.setSleep(false);

  Serial.println("Access Point started");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);
  Serial.print("Password: ");
  Serial.println(AP_PASSWORD);
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
}

void initialiseRadar() {
  // Configure UART1 for the LD2450's high baudrate data stream.
  RadarSerial.begin(LD2450_SERIAL_SPEED, SERIAL_8N1, RADAR_UART_RX_PIN, RADAR_UART_TX_PIN);
  radar.begin(RadarSerial, true);
  radar.setNumberOfTargets(LD2450_MAX_SENSOR_TARGETS);

  if (radar.waitForSensorMessage()) {
    LOGI("SENSOR", "Sensor stream detected");
  } else {
    LOGW("SENSOR", "No sensor data yet - check wiring and power");
  }
}

void addFrame(const LD2450::RadarTarget *targets) {
  TargetFrame &slot = historyBuffer[historyIndex];
  slot.timestamp_ms = millis();

  for (size_t i = 0; i < LD2450_MAX_SENSOR_TARGETS; ++i) {
    slot.targets[i] = targets[i];
    slot.targets[i].id = i + 1;
  }

  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  if (historyCount < HISTORY_SIZE) {
    historyCount++;
  }

  lastFrame = slot;
  hasFrame = true;
  broadcastLatestFrame();
}

static String buildTargetJson(const LD2450::RadarTarget &target) {
  String json;
  json.reserve(180);

  json += F("{\"id\":");
  json += target.id;
  json += F(",\"valid\":");
  json += target.valid ? F("true") : F("false");
  json += F(",\"distance_mm\":");
  json += target.distance;
  json += F(",\"position_mm\":{\"x\":");
  json += target.x;
  json += F(",\"y\":");
  json += target.y;
  json += F(",\"z\":0}");
  json += F(",\"speed_cm_s\":");
  json += target.speed;

  float vx_m_s = 0.0f;
  float vy_m_s = 0.0f;
  float vz_m_s = 0.0f;

  if (target.valid && target.distance != 0) {
    const float distance_mm = static_cast<float>(target.distance);
    const float unitX = static_cast<float>(target.x) / distance_mm;
    const float unitY = static_cast<float>(target.y) / distance_mm;
    const float speed_m_s = static_cast<float>(target.speed) / 100.0f; // cm/s -> m/s
    vx_m_s = speed_m_s * unitX;
    vy_m_s = speed_m_s * unitY;
    vz_m_s = 0.0f;
  }

  json += F(",\"velocity_m_s\":{\"x\":");
  json += String(vx_m_s, 4);
  json += F(",\"y\":");
  json += String(vy_m_s, 4);
  json += F(",\"z\":");
  json += String(vz_m_s, 4);
  json += F("}");
  json += F(",\"resolution_mm\":");
  json += target.resolution;
  json += F("}");

  return json;
}

String buildFrameJson(const TargetFrame &frame) {
  String json;
  json.reserve(256);
  json += F("{\"timestamp_ms\":");
  json += frame.timestamp_ms;
  json += F(",\"targets\":[");
  for (size_t i = 0; i < LD2450_MAX_SENSOR_TARGETS; ++i) {
    if (i > 0) {
      json += ',';
    }
    json += buildTargetJson(frame.targets[i]);
  }
  json += F("]}");
  return json;
}

String buildHistoryJson() {
  String json;
  json.reserve(historyCount * 220 + 4);
  json += '[';
  for (size_t i = 0; i < historyCount; ++i) {
    if (i > 0) {
      json += ',';
    }
    const size_t index = (historyIndex + HISTORY_SIZE - historyCount + i) % HISTORY_SIZE;
    json += buildFrameJson(historyBuffer[index]);
  }
  json += ']';
  return json;
}

void broadcastLatestFrame() {
  if (!hasFrame) {
    return;
  }
  const String payload = buildFrameJson(lastFrame);
  radarSocket.textAll(payload);
}

void handleSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                       void *arg, uint8_t *data, size_t len) {
  (void)server;
  (void)arg;
  (void)data;
  (void)len;
  switch (type) {
  case WS_EVT_CONNECT:
    wsClientCount++;
  LOGI("WS", "Client #%u connected from %s (clients=%u)", client->id(), client->remoteIP().toString().c_str(), (unsigned)wsClientCount);
    if (hasFrame) {
      client->text(buildFrameJson(lastFrame));
    }
    break;
  case WS_EVT_DISCONNECT:
    if (wsClientCount > 0) wsClientCount--;
  LOGI("WS", "Client #%u disconnected (clients=%u)", client->id(), (unsigned)wsClientCount);
    break;
  case WS_EVT_DATA:
    // The front-end currently only listens; ignore incoming payloads.
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
  default:
    break;
  }
}