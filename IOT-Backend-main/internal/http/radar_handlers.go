package http

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"sync"
	"time"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/types"
	"github.com/gorilla/mux"
)

// RadarCache stores the latest mmwave frame per coordinator
type RadarCache struct {
	mu     sync.RWMutex
	frames map[string]*types.MmwaveFrame // key: "siteId:coordId"
}

func NewRadarCache() *RadarCache {
	return &RadarCache{
		frames: make(map[string]*types.MmwaveFrame),
	}
}

func (rc *RadarCache) Set(siteId, coordId string, frame *types.MmwaveFrame) {
	rc.mu.Lock()
	defer rc.mu.Unlock()
	key := fmt.Sprintf("%s:%s", siteId, coordId)
	rc.frames[key] = frame
}

func (rc *RadarCache) Get(siteId, coordId string) *types.MmwaveFrame {
	rc.mu.RLock()
	defer rc.mu.RUnlock()
	key := fmt.Sprintf("%s:%s", siteId, coordId)
	return rc.frames[key]
}

// GetRadarImage returns a PNG image of the radar visualization
func (h *Handler) GetRadarImage(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	siteId := vars["siteId"]
	coordId := vars["coordId"]

	// Get latest frame from cache
	frame := h.radarCache.Get(siteId, coordId)

	// Render radar image
	renderer := NewRadarRenderer()
	imgData, err := renderer.RenderRadar(frame)
	if err != nil {
		log.Printf("Error rendering radar: %v", err)
		http.Error(w, "Failed to render radar", http.StatusInternalServerError)
		return
	}

	// Return PNG image
	w.Header().Set("Content-Type", "image/png")
	w.Header().Set("Cache-Control", "no-cache, no-store, must-revalidate")
	w.Header().Set("Pragma", "no-cache")
	w.Header().Set("Expires", "0")
	w.Write(imgData)
}

// GetRadarData returns JSON data of the latest radar frame
func (h *Handler) GetRadarData(w http.ResponseWriter, r *http.Request) {
	vars := mux.Vars(r)
	siteId := vars["siteId"]
	coordId := vars["coordId"]

	frame := h.radarCache.Get(siteId, coordId)
	if frame == nil {
		json.NewEncoder(w).Encode(map[string]interface{}{
			"site_id":        siteId,
			"coordinator_id": coordId,
			"presence":       false,
			"targets":        []types.MmwaveTarget{},
			"timestamp":      time.Now(),
		})
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(frame)
}
