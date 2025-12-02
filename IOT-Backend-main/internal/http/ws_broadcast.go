package http

import (
	"log"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

// WSBroadcaster manages WebSocket connections and broadcasts messages to all clients
type WSBroadcaster struct {
	clients   map[*websocket.Conn]bool
	broadcast chan interface{}
	register  chan *websocket.Conn
	unregister chan *websocket.Conn
	mu        sync.RWMutex
}

// NewWSBroadcaster creates a new WebSocket broadcaster
func NewWSBroadcaster() *WSBroadcaster {
	return &WSBroadcaster{
		clients:    make(map[*websocket.Conn]bool),
		broadcast:  make(chan interface{}, 256),
		register:   make(chan *websocket.Conn),
		unregister: make(chan *websocket.Conn),
	}
}

// Start begins the broadcaster event loop
func (b *WSBroadcaster) Start() {
	go b.run()
}

// run is the main event loop for the broadcaster
func (b *WSBroadcaster) run() {
	for {
		select {
		case client := <-b.register:
			b.mu.Lock()
			b.clients[client] = true
			b.mu.Unlock()
			log.Printf("WebSocket client registered. Total clients: %d", len(b.clients))

		case client := <-b.unregister:
			b.mu.Lock()
			if _, ok := b.clients[client]; ok {
				delete(b.clients, client)
				client.Close()
			}
			b.mu.Unlock()
			log.Printf("WebSocket client unregistered. Total clients: %d", len(b.clients))

		case message := <-b.broadcast:
			b.mu.RLock()
			for client := range b.clients {
				// Send to each client in a goroutine to avoid blocking
				go func(c *websocket.Conn, msg interface{}) {
					c.SetWriteDeadline(time.Now().Add(10 * time.Second))
					if err := c.WriteJSON(msg); err != nil {
						log.Printf("Error writing to WebSocket client: %v", err)
						// Unregister client on error
						b.unregister <- c
					}
				}(client, message)
			}
			b.mu.RUnlock()
		}
	}
}

// Register adds a WebSocket connection to the broadcaster
func (b *WSBroadcaster) Register(conn *websocket.Conn) {
	b.register <- conn
}

// Unregister removes a WebSocket connection from the broadcaster
func (b *WSBroadcaster) Unregister(conn *websocket.Conn) {
	b.unregister <- conn
}

// Broadcast sends a message to all connected clients
func (b *WSBroadcaster) Broadcast(message interface{}) {
	select {
	case b.broadcast <- message:
	default:
		log.Println("Broadcast channel full, dropping message")
	}
}

// NodeTelemetryMessage represents a node telemetry WebSocket message
type NodeTelemetryMessage struct {
	Type    string               `json:"type"`
	Payload NodeTelemetryPayload `json:"payload"`
}

// NodeTelemetryPayload contains node telemetry data
type NodeTelemetryPayload struct {
	NodeID     string  `json:"nodeId"`
	LightID    string  `json:"lightId"`
	Timestamp  int64   `json:"ts"`
	TempC      float32 `json:"tempC"`
	Light      LightState `json:"light"`
	VbatMv     int     `json:"vbatMv"`
	StatusMode string  `json:"statusMode"`
}

// LightState represents the current state of the node's light
type LightState struct {
	On         bool `json:"on"`
	Brightness int  `json:"brightness"`
	AvgR       int  `json:"avgR"`
	AvgG       int  `json:"avgG"`
	AvgB       int  `json:"avgB"`
	AvgW       int  `json:"avgW"`
}

// CoordinatorTelemetryMessage represents coordinator telemetry WebSocket message
type CoordinatorTelemetryMessage struct {
	Type    string                      `json:"type"`
	Payload CoordinatorTelemetryPayload `json:"payload"`
}

// CoordinatorTelemetryPayload contains coordinator telemetry data
type CoordinatorTelemetryPayload struct {
	CoordID    string  `json:"coordId"`
	SiteID     string  `json:"siteId"`
	Timestamp  int64   `json:"ts"`
	LightLux   float32 `json:"lightLux"`
	TempC      float32 `json:"tempC"`
	WifiRssi   int     `json:"wifiRssi"`
	MMWave     MMWaveState `json:"mmwave"`
}

// MMWaveState represents mmWave sensor state
type MMWaveState struct {
	Presence   bool    `json:"presence"`
	Confidence float32 `json:"confidence"`
	Online     bool    `json:"online"`
}

// BroadcastNodeTelemetry sends node telemetry to all WebSocket clients
func (b *WSBroadcaster) BroadcastNodeTelemetry(nodeID, lightID string, ts int64, tempC float32, 
	avgR, avgG, avgB, avgW, vbatMv int, statusMode string) {
	
	// Compute "on" state and brightness from RGBW values
	// Light is considered "on" if any RGBW channel is active
	on := (avgR + avgG + avgB + avgW) > 0
	
	// Brightness is approximated as the max of RGBW channels
	brightness := max(avgR, avgG, avgB, avgW)
	
	msg := NodeTelemetryMessage{
		Type: "node_telemetry",
		Payload: NodeTelemetryPayload{
			NodeID:     nodeID,
			LightID:    lightID,
			Timestamp:  ts,
			TempC:      tempC,
			Light: LightState{
				On:         on,
				Brightness: brightness,
				AvgR:       avgR,
				AvgG:       avgG,
				AvgB:       avgB,
				AvgW:       avgW,
			},
			VbatMv:     vbatMv,
			StatusMode: statusMode,
		},
	}
	
	b.Broadcast(msg)
}

// BroadcastCoordinatorTelemetry sends coordinator telemetry to all WebSocket clients
func (b *WSBroadcaster) BroadcastCoordinatorTelemetry(coordID, siteID string, ts int64, lightLux, tempC float32,
	wifiRssi int, mmwavePresence bool, mmwaveConfidence float32, mmwaveOnline bool) {
	
	msg := CoordinatorTelemetryMessage{
		Type: "coord_telemetry",
		Payload: CoordinatorTelemetryPayload{
			CoordID:   coordID,
			SiteID:    siteID,
			Timestamp: ts,
			LightLux:  lightLux,
			TempC:     tempC,
			WifiRssi:  wifiRssi,
			MMWave: MMWaveState{
				Presence:   mmwavePresence,
				Confidence: mmwaveConfidence,
				Online:     mmwaveOnline,
			},
		},
	}
	
	b.Broadcast(msg)
}

// max returns the maximum of a list of integers
func max(values ...int) int {
	if len(values) == 0 {
		return 0
	}
	maxVal := values[0]
	for _, v := range values[1:] {
		if v > maxVal {
			maxVal = v
		}
	}
	return maxVal
}
