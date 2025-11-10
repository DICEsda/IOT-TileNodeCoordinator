package http

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"

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

	// OTA API
	router.HandleFunc("/ota/start", h.StartOTAUpdate).Methods("POST")
	router.HandleFunc("/ota/status", h.RetrieveOTAJobStatus).Methods("GET")

	// Coordinators API
	router.HandleFunc("/coordinators/{id}", h.getCoordinatorById).Methods("GET")

	// Nodes API
	router.HandleFunc("/nodes/{id}", h.getNodeById).Methods("GET")

	// Commands API
	router.HandleFunc("/color-profile", h.sendColorProfile).Methods("POST")
	router.HandleFunc("/set-light", h.setLight).Methods("POST")
	router.HandleFunc("/pairing/approve", h.ApproveNodePairing).Methods("POST")

	// WebSocket
	router.HandleFunc("/ws", h.websocket)

	// Enable CORS
	router.Use(corsMiddleware)
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

var upgrader = websocket.Upgrader{
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

func (h *Handler) websocket(w http.ResponseWriter, r *http.Request) {
	ws, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Println(err)
	}
	defer func() {
		if err := ws.Close(); err != nil {
			log.Println(err)
		}
	}()

	for {
		msgType, msg, err := ws.ReadMessage()
		if err != nil {
			log.Println(err)
			return
		}
		fmt.Println(msgType, string(msg))

		err = ws.WriteMessage(msgType, msg)
		if err != nil {
			log.Println(err)
			return
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

func (h *Handler) healthCheck(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "healthy",
		"service": "iot-backend",
	})
}

func corsMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")

		if r.Method == "OPTIONS" {
			w.WriteHeader(http.StatusOK)
			return
		}

		next.ServeHTTP(w, r)
	})
}
