package http

import (
	"context"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"strconv"
	"strings"
	"sync"
	"time"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/repository"
	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/gorilla/mux"
	"github.com/gorilla/websocket"
	"go.uber.org/fx"
)

type Params struct {
	fx.In

	Repo        repository.Repository
	MqttClient  mqtt.Client
	Broadcaster *WSBroadcaster
	RadarCache  *RadarCache
}

type Handler struct {
	Repo        repository.Repository
	mqttClient  mqtt.Client
	broadcaster *WSBroadcaster
	radarCache  *RadarCache
}

func NewHandler(p Params) *Handler {
	return &Handler{
		Repo:        p.Repo,
		mqttClient:  p.MqttClient,
		broadcaster: p.Broadcaster,
		radarCache:  p.RadarCache,
	}
}

func RegisterHandlers(h *Handler, router *mux.Router) {
	// Health check
	router.HandleFunc("/health", h.healthCheck).Methods("GET")

	// Sites API
	router.HandleFunc("/sites", h.getSites).Methods("GET")
	router.HandleFunc("/sites/{id}", h.getSiteById).Methods("GET")
	router.HandleFunc("/sites/{siteId}/coordinators/{coordId}", h.GetCoordinator).Methods("GET")
	router.HandleFunc("/sites/{siteId}/coordinators/{coordId}/nodes", h.GetNodes).Methods("GET")
	router.HandleFunc("/sites/{siteId}/coordinators/{coordId}/nodes/{nodeId}", h.DeleteNode).Methods("DELETE")

	// OTA API
	router.HandleFunc("/ota/start", h.StartOTAUpdate).Methods("POST")
	router.HandleFunc("/ota/status", h.RetrieveOTAJobStatus).Methods("GET")

	// Coordinators API
	router.HandleFunc("/coordinators/{id}", h.getCoordinatorById).Methods("GET")

	// Coordinator Control API
	router.HandleFunc("/api/v1/coordinator/pair", h.StartPairing).Methods("POST")
	router.HandleFunc("/api/v1/coordinator/restart", h.RestartCoordinator).Methods("POST")
	router.HandleFunc("/api/v1/coordinator/wifi", h.UpdateWiFiConfig).Methods("POST")

	// Node Management API
	router.HandleFunc("/api/v1/node/zone", h.UpdateNodeZone).Methods("PUT")
	router.HandleFunc("/api/v1/node/name", h.UpdateNodeName).Methods("PUT")
	router.HandleFunc("/api/v1/node/test-color", h.SendNodeColor).Methods("POST")
	router.HandleFunc("/api/v1/node/off", h.TurnOffNode).Methods("POST")
	router.HandleFunc("/api/v1/node/brightness", h.SetNodeBrightness).Methods("POST")
	
	// Live Monitor Light Control API
	router.HandleFunc("/api/v1/node/light/control", h.ControlNodeLight).Methods("POST")

	// Settings API
	router.HandleFunc("/api/v1/settings", h.GetSettings).Methods("GET")
	router.HandleFunc("/api/v1/settings", h.SaveSettings).Methods("PUT")

	// Zone Management API
	router.HandleFunc("/api/v1/zones", h.CreateZone).Methods("POST")
	router.HandleFunc("/api/v1/zones", h.GetZones).Methods("GET")
	router.HandleFunc("/api/v1/zones/{id}", h.GetZone).Methods("GET")
	router.HandleFunc("/api/v1/zones/{id}", h.UpdateZone).Methods("PUT")
	router.HandleFunc("/api/v1/zones/{id}", h.DeleteZone).Methods("DELETE")

	// Radar Visualization API
	router.HandleFunc("/api/radar/{siteId}/{coordId}/image", h.GetRadarImage).Methods("GET")
	router.HandleFunc("/api/radar/{siteId}/{coordId}/data", h.GetRadarData).Methods("GET")

	// Google Home Integration API
	router.HandleFunc("/api/v1/google/auth", h.InitiateGoogleAuth).Methods("GET")
	router.HandleFunc("/api/v1/google/callback", h.GoogleAuthCallback).Methods("GET")
	router.HandleFunc("/api/v1/google/disconnect", h.DisconnectGoogleHome).Methods("POST")

	// Customization API
	router.HandleFunc("/api/v1/{type:coordinator|node}/{id}/customize", h.GetCustomizationConfig).Methods("GET")
	router.HandleFunc("/api/v1/{type:coordinator|node}/{id}/customize/config", h.UpdateCoordinatorConfig).Methods("PUT")
	router.HandleFunc("/api/v1/{type:coordinator|node}/{id}/customize/led", h.UpdateLEDConfig).Methods("PUT")
	router.HandleFunc("/api/v1/{type:coordinator|node}/{id}/customize/reset", h.ResetToDefaults).Methods("POST")
	router.HandleFunc("/api/v1/{type:coordinator|node}/{id}/led/preview", h.LEDPreview).Methods("POST")

	// Nodes API
	router.HandleFunc("/nodes/{id}", h.getNodeById).Methods("GET")

	// mmWave API
	router.HandleFunc("/mmwave/history", h.getMmwaveHistory).Methods("GET")

	// Commands API
	router.HandleFunc("/color-profile", h.sendColorProfile).Methods("POST")
	router.HandleFunc("/set-light", h.setLight).Methods("POST")
	router.HandleFunc("/pairing/approve", h.ApproveNodePairing).Methods("POST")

	// WebSocket endpoints
	router.HandleFunc("/ws", h.websocket)
	router.HandleFunc("/mqtt", h.websocket) // MQTT WebSocket bridge (same as /ws)
}

func (h *Handler) getNodeById(w http.ResponseWriter, r *http.Request) {
	id := mux.Vars(r)["id"]
	node, err := h.Repo.GetNodeById(id)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	jsonData, err := json.Marshal(node)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.Write(jsonData)
}

func (h *Handler) getMmwaveHistory(w http.ResponseWriter, r *http.Request) {
	query := r.URL.Query()
	siteId := query.Get("site_id")
	coordId := query.Get("coord_id")
	limit := 200
	if raw := query.Get("limit"); raw != "" {
		if v, err := strconv.Atoi(raw); err == nil && v > 0 {
			limit = v
		}
	}

	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()

	frames, err := h.Repo.GetMmwaveFrames(ctx, siteId, coordId, limit)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	payload, err := json.Marshal(frames)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	_, _ = w.Write(payload)
}

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

func (h *Handler) websocket(w http.ResponseWriter, r *http.Request) {
	type subHandle struct {
		topic string
		token mqtt.Token
	}

	ws, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println("ws upgrade:", err)
		return
	}
	
	// Register this connection with the broadcaster
	if h.broadcaster != nil {
		h.broadcaster.Register(ws)
		log.Printf("WebSocket client registered with broadcaster")
	}
	
	defer func() {
		// Unregister from broadcaster
		if h.broadcaster != nil {
			h.broadcaster.Unregister(ws)
			log.Printf("WebSocket client unregistered from broadcaster")
		}
		if err := ws.Close(); err != nil {
			log.Println("ws close:", err)
		}
	}()

	// Track MQTT subscriptions for this client
	var (
		subs   = make(map[string]subHandle)
		writeM sync.Mutex
	)

	// Helper: write JSON safely
	writeJSON := func(v any) error {
		writeM.Lock()
		defer writeM.Unlock()
		ws.SetWriteDeadline(time.Now().Add(10 * time.Second))
		return ws.WriteJSON(v)
	}

	// Cleanup on exit
	defer func() {
		for topic, sh := range subs {
			if sh.token != nil {
				h.mqttClient.Unsubscribe(topic)
			}
		}
	}()

	// Read loop
	log.Printf("[WS-DEBUG] Entering read loop for client from %s", r.RemoteAddr)
	for {
		_, msg, err := ws.ReadMessage()
		if err != nil {
			log.Println("ws read:", err)
			return
		}
		log.Printf("[WS-DEBUG] Received raw message: %s", string(msg))
		var req struct {
			Type    string      `json:"type"`
			Topic   string      `json:"topic"`
			Payload interface{} `json:"payload"`
			QoS     byte        `json:"qos"`
		}
		if err := json.Unmarshal(msg, &req); err != nil {
			log.Printf("[WS-DEBUG] Failed to unmarshal message: %v, raw: %s", err, string(msg))
			_ = writeJSON(map[string]any{"type": "error", "message": "invalid json"})
			continue
		}

		log.Printf("[WS-DEBUG] Received type=%s topic=%s", req.Type, req.Topic)

		switch req.Type {
		case "subscribe":
			topic := req.Topic
			if topic == "" {
				_ = writeJSON(map[string]any{"type": "error", "message": "missing topic"})
				continue
			}
			// If already subscribed, skip
			if _, ok := subs[topic]; ok {
				_ = writeJSON(map[string]any{"type": "subscribed", "topic": topic})
				continue
			}
			// Subscribe with callback bound to this WS connection
			token := h.mqttClient.Subscribe(topic, 1, func(c mqtt.Client, m mqtt.Message) {
				// Try to decode JSON payload; fallback to string
				var parsed any
				if err := json.Unmarshal(m.Payload(), &parsed); err != nil {
					parsed = string(m.Payload())
				}
				_ = writeJSON(map[string]any{
					"type":      "message",
					"topic":     m.Topic(),
					"payload":   parsed,
					"timestamp": time.Now().UnixMilli(),
				})
			})
			token.Wait()
			if token.Error() != nil {
				_ = writeJSON(map[string]any{"type": "error", "message": token.Error().Error()})
				continue
			}
			subs[topic] = subHandle{topic: topic, token: token}
			_ = writeJSON(map[string]any{"type": "subscribed", "topic": topic})

		case "unsubscribe":
			topic := req.Topic
			if topic == "" {
				_ = writeJSON(map[string]any{"type": "error", "message": "missing topic"})
				continue
			}
			if _, ok := subs[topic]; ok {
				h.mqttClient.Unsubscribe(topic)
				delete(subs, topic)
			}
			_ = writeJSON(map[string]any{"type": "unsubscribed", "topic": topic})

		case "publish":
			topic := req.Topic
			if topic == "" {
				_ = writeJSON(map[string]any{"type": "error", "message": "missing topic"})
				continue
			}
			// Encode payload as JSON if it's a structured object, otherwise send as string
			var payloadBytes []byte
			switch p := req.Payload.(type) {
			case string:
				payloadBytes = []byte(p)
			default:
				b, err := json.Marshal(p)
				if err != nil {
					_ = writeJSON(map[string]any{"type": "error", "message": "invalid payload"})
					continue
				}
				payloadBytes = b
			}
			log.Printf("[WS-DEBUG] Publishing to MQTT: topic=%s qos=%d payload=%s", topic, req.QoS, string(payloadBytes))
			token := h.mqttClient.Publish(topic, req.QoS, false, payloadBytes)
			token.Wait()
			if token.Error() != nil {
				log.Printf("[WS-ERROR] Failed to publish: %v", token.Error())
				_ = writeJSON(map[string]any{"type": "error", "message": token.Error().Error()})
				continue
			}
			log.Printf("[WS-DEBUG] Published successfully to: %s", topic)
			_ = writeJSON(map[string]any{"type": "published", "topic": topic})

		default:
			_ = writeJSON(map[string]any{"type": "error", "message": "unknown type"})
		}
	}
}

func (h *Handler) getSites(w http.ResponseWriter, r *http.Request) {
	sites, err := h.Repo.GetSites()
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	jsonData, err := json.Marshal(sites)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.Write(jsonData)

}
func (h *Handler) getSiteById(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	id := vars["id"]
	site, err := h.Repo.GetSiteById(id)
	if err != nil {
		if strings.Contains(err.Error(), "not found") {
			http.Error(w, err.Error(), http.StatusNotFound)
			return
		}
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	jsonData, err := json.Marshal(site)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.Write(jsonData)
}

func (h *Handler) getCoordinatorById(w http.ResponseWriter, r *http.Request) {
	id := mux.Vars(r)["id"]
	coordinator, err := h.Repo.GetCoordinatorById(id)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	jsonData, err := json.Marshal(coordinator)
	if err != nil {
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.Write(jsonData)
}

func (h *Handler) sendColorProfile(w http.ResponseWriter, r *http.Request) {
	w.Write([]byte("Hello, World!"))
}

func (h *Handler) setLight(w http.ResponseWriter, r *http.Request) {
	w.Write([]byte("Hello, World!"))
}

func (h *Handler) StartOTAUpdate(w http.ResponseWriter, r *http.Request) {
	w.Write([]byte("Hello, World!"))
}

func (h *Handler) RetrieveOTAJobStatus(w http.ResponseWriter, r *http.Request) {
	w.Write([]byte("Hello, World!"))
}

func (h *Handler) ApproveNodePairing(w http.ResponseWriter, r *http.Request) {
	w.Write([]byte("Hello, World!"))
}

// Note: GetCoordinator, GetNodes, and DeleteNode are implemented in coordinator_handlers.go

func (h *Handler) healthCheck(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	
	// Check database connection
	dbHealthy := false
	if h.Repo != nil {
		_, err := h.Repo.GetSites()
		dbHealthy = err == nil
	}
	
	// Check MQTT connection
	mqttHealthy := h.mqttClient != nil && h.mqttClient.IsConnected()
	
	// Check if any coordinators are online (seen in last 5 minutes)
	// For now, just check if coordinator with default ID exists
	coordOnline := false
	if h.Repo != nil {
		ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
		frames, err := h.Repo.GetMmwaveFrames(ctx, "", "", 1)
		cancel()
		if err == nil && len(frames) > 0 {
			if time.Since(frames[0].Timestamp) < 5*time.Minute {
				coordOnline = true
			}
		}

		if !coordOnline {
			coord, err := h.Repo.GetCoordinatorById("coord001")
			if err == nil && coord != nil {
				if time.Since(coord.LastSeen) < 5*time.Minute {
					coordOnline = true
				}
			}
		}
	}
	
	overallHealthy := dbHealthy && mqttHealthy
	statusCode := http.StatusOK
	status := "healthy"
	if !overallHealthy {
		statusCode = http.StatusServiceUnavailable
		status = "degraded"
	}
	
	w.WriteHeader(statusCode)
	json.NewEncoder(w).Encode(map[string]interface{}{
		"status":      status,
		"service":     "iot-backend",
		"database":    dbHealthy,
		"mqtt":        mqttHealthy,
		"coordinator": coordOnline,
		"timestamp":   time.Now().Unix(),
	})
}

// ControlNodeLight handles live monitor light control commands
func (h *Handler) ControlNodeLight(w http.ResponseWriter, r *http.Request) {
	var req struct {
		SiteID     string `json:"site_id"`
		NodeID     string `json:"node_id"`
		On         bool   `json:"on"`
		Brightness int    `json:"brightness"` // 0-255
	}
	
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}
	
	// Validate inputs
	if req.SiteID == "" || req.NodeID == "" {
		http.Error(w, "site_id and node_id are required", http.StatusBadRequest)
		return
	}
	
	if req.Brightness < 0 || req.Brightness > 255 {
		http.Error(w, "brightness must be between 0 and 255", http.StatusBadRequest)
		return
	}
	
	// Build the set_light command for the node
	// When ON, use green color (0, 255, 0) as specified
	// The node will control the SK6812B strip
	cmdPayload := map[string]interface{}{
		"cmd":        "set_light",
		"on":         req.On,
		"brightness": req.Brightness,
		"r":          0,
		"g":          255, // Always green when on
		"b":          0,
		"w":          0,
	}
	
	if !req.On {
		// When off, set all channels to 0
		cmdPayload["brightness"] = 0
		cmdPayload["g"] = 0
	}
	
	payloadBytes, err := json.Marshal(cmdPayload)
	if err != nil {
		http.Error(w, "Failed to create command", http.StatusInternalServerError)
		return
	}
	
	// Publish to the node command topic: site/{siteId}/node/{nodeId}/cmd
	topic := fmt.Sprintf("site/%s/node/%s/cmd", req.SiteID, req.NodeID)
	token := h.mqttClient.Publish(topic, 1, false, payloadBytes)
	token.Wait()
	
	if err := token.Error(); err != nil {
		log.Printf("Failed to publish light command: %v", err)
		http.Error(w, "Failed to send command", http.StatusInternalServerError)
		return
	}
	
	log.Printf("Light control command sent to %s: on=%v, brightness=%d", req.NodeID, req.On, req.Brightness)
	
	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]interface{}{
		"success": true,
		"message": "Command sent successfully",
		"topic":   topic,
	})
}
