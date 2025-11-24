package http

import (
	"context"
	"encoding/json"
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

	Repo       repository.Repository
	MqttClient mqtt.Client
}

type Handler struct {
	Repo       repository.Repository
	mqttClient mqtt.Client
}

func NewHandler(p Params) *Handler {
	return &Handler{
		Repo:       p.Repo,
		mqttClient: p.MqttClient,
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

	// Settings API
	router.HandleFunc("/api/v1/settings", h.GetSettings).Methods("GET")
	router.HandleFunc("/api/v1/settings", h.SaveSettings).Methods("PUT")

	// Google Home Integration API
	router.HandleFunc("/api/v1/google/auth", h.InitiateGoogleAuth).Methods("GET")
	router.HandleFunc("/api/v1/google/callback", h.GoogleAuthCallback).Methods("GET")
	router.HandleFunc("/api/v1/google/disconnect", h.DisconnectGoogleHome).Methods("POST")

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
	defer func() {
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
	for {
		_, msg, err := ws.ReadMessage()
		if err != nil {
			log.Println("ws read:", err)
			return
		}
		var req struct {
			Type    string      `json:"type"`
			Topic   string      `json:"topic"`
			Payload interface{} `json:"payload"`
			QoS     byte        `json:"qos"`
		}
		if err := json.Unmarshal(msg, &req); err != nil {
			_ = writeJSON(map[string]any{"type": "error", "message": "invalid json"})
			continue
		}

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
			token := h.mqttClient.Publish(topic, req.QoS, false, payloadBytes)
			token.Wait()
			if token.Error() != nil {
				_ = writeJSON(map[string]any{"type": "error", "message": token.Error().Error()})
				continue
			}
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
