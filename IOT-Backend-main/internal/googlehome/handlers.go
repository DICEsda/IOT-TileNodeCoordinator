package googlehome

import (
	"encoding/json"
	"net/http"

	"github.com/gorilla/mux"
	"go.uber.org/zap"
)

type Handler struct {
	logger  *zap.Logger
	service *GoogleHomeService
}

func NewHandler(logger *zap.Logger, service *GoogleHomeService) *Handler {
	return &Handler{
		logger:  logger,
		service: service,
	}
}

func RegisterHandlers(h *Handler, router *mux.Router) {
	// Google Home Smart Home fulfillment
	router.HandleFunc("/api/v1/google/home/fulfillment", h.Fulfillment).Methods("POST")

	// OAuth endpoints
	router.HandleFunc("/oauth/google/authorize", h.Authorize).Methods("GET")
	router.HandleFunc("/oauth/google/token", h.Token).Methods("POST")
	router.HandleFunc("/oauth/google/callback", h.Callback).Methods("GET")

	// State reporting
	router.HandleFunc("/api/v1/google/home/report-state", h.ReportState).Methods("POST")
	router.HandleFunc("/api/v1/google/home/request-sync", h.RequestSync).Methods("POST")
}

// Fulfillment handles Google Assistant smart home requests
func (h *Handler) Fulfillment(w http.ResponseWriter, r *http.Request) {
	if !h.service.IsEnabled() {
		http.Error(w, "Google Home integration disabled", http.StatusServiceUnavailable)
		return
	}

	var request struct {
		RequestID string `json:"requestId"`
		Inputs    []struct {
			Intent  string                 `json:"intent"`
			Payload map[string]interface{} `json:"payload,omitempty"`
		} `json:"inputs"`
	}

	if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
		h.logger.Error("Failed to decode request", zap.Error(err))
		http.Error(w, "Invalid request", http.StatusBadRequest)
		return
	}

	h.logger.Info("Received fulfillment request",
		zap.String("requestId", request.RequestID),
		zap.String("intent", request.Inputs[0].Intent))

	var response map[string]interface{}

	// Extract user ID from request (implement authentication)
	userID := "user-default" // TODO: Get from authenticated session

	switch request.Inputs[0].Intent {
	case "action.devices.SYNC":
		payload, err := h.service.SyncRequest(userID)
		if err != nil {
			h.logger.Error("SYNC failed", zap.Error(err))
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		response = map[string]interface{}{
			"requestId": request.RequestID,
			"payload":   payload,
		}

	case "action.devices.QUERY":
		devices := request.Inputs[0].Payload["devices"].([]interface{})
		deviceIDs := make([]string, len(devices))
		for i, d := range devices {
			deviceIDs[i] = d.(map[string]interface{})["id"].(string)
		}

		payload, err := h.service.QueryRequest(userID, deviceIDs)
		if err != nil {
			h.logger.Error("QUERY failed", zap.Error(err))
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		response = map[string]interface{}{
			"requestId": request.RequestID,
			"payload":   payload,
		}

	case "action.devices.EXECUTE":
		commandsData := request.Inputs[0].Payload["commands"].([]interface{})
		var commands []Command

		for _, cmdData := range commandsData {
			cmdMap := cmdData.(map[string]interface{})
			var cmd Command

			// Parse devices
			devicesData := cmdMap["devices"].([]interface{})
			for _, devData := range devicesData {
				devMap := devData.(map[string]interface{})
				cmd.Devices = append(cmd.Devices, Device{ID: devMap["id"].(string)})
			}

			// Parse execution
			execData := cmdMap["execution"].([]interface{})
			for _, exData := range execData {
				exMap := exData.(map[string]interface{})
				cmd.Execution = append(cmd.Execution, Execution{
					Command: exMap["command"].(string),
					Params:  exMap["params"].(map[string]interface{}),
				})
			}

			commands = append(commands, cmd)
		}

		payload, err := h.service.ExecuteRequest(userID, commands)
		if err != nil {
			h.logger.Error("EXECUTE failed", zap.Error(err))
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		response = map[string]interface{}{
			"requestId": request.RequestID,
			"payload":   payload,
		}

	case "action.devices.DISCONNECT":
		// Handle account unlink
		h.logger.Info("User disconnected Google Home", zap.String("userId", userID))
		response = map[string]interface{}{
			"requestId": request.RequestID,
		}

	default:
		h.logger.Warn("Unknown intent", zap.String("intent", request.Inputs[0].Intent))
		http.Error(w, "Unknown intent", http.StatusBadRequest)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(response)
}

// Authorize handles OAuth authorization
func (h *Handler) Authorize(w http.ResponseWriter, r *http.Request) {
	if !h.service.IsEnabled() {
		http.Error(w, "Google Home integration disabled", http.StatusServiceUnavailable)
		return
	}

	// TODO: Implement proper OAuth flow
	// For now, return a simple authorization page
	html := `
	<!DOCTYPE html>
	<html>
	<head>
		<title>Authorize Smart Tile</title>
	</head>
	<body>
		<h1>Authorize Smart Tile with Google Home</h1>
		<p>Click authorize to allow Google Home to control your tiles.</p>
		<button onclick="authorize()">Authorize</button>
		<script>
			function authorize() {
				window.location.href = '/oauth/google/callback?code=AUTH_CODE&state=' + 
					new URLSearchParams(window.location.search).get('state');
			}
		</script>
	</body>
	</html>
	`
	w.Header().Set("Content-Type", "text/html")
	w.Write([]byte(html))
}

// Token handles OAuth token exchange
func (h *Handler) Token(w http.ResponseWriter, r *http.Request) {
	if !h.service.IsEnabled() {
		http.Error(w, "Google Home integration disabled", http.StatusServiceUnavailable)
		return
	}

	if err := r.ParseForm(); err != nil {
		http.Error(w, "Invalid request", http.StatusBadRequest)
		return
	}

	grantType := r.FormValue("grant_type")

	var response map[string]interface{}

	switch grantType {
	case "authorization_code":
		// TODO: Validate authorization code
		response = map[string]interface{}{
			"token_type":    "Bearer",
			"access_token":  "ACCESS_TOKEN",
			"refresh_token": "REFRESH_TOKEN",
			"expires_in":    3600,
		}

	case "refresh_token":
		// TODO: Validate and refresh token
		response = map[string]interface{}{
			"token_type":   "Bearer",
			"access_token": "NEW_ACCESS_TOKEN",
			"expires_in":   3600,
		}

	default:
		http.Error(w, "Invalid grant_type", http.StatusBadRequest)
		return
	}

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(response)
}

// Callback handles OAuth callback
func (h *Handler) Callback(w http.ResponseWriter, r *http.Request) {
	if !h.service.IsEnabled() {
		http.Error(w, "Google Home integration disabled", http.StatusServiceUnavailable)
		return
	}

	code := r.URL.Query().Get("code")
	state := r.URL.Query().Get("state")

	h.logger.Info("OAuth callback received",
		zap.String("code", code),
		zap.String("state", state))

	// Redirect back to Google with the authorization
	redirectURL := r.URL.Query().Get("redirect_uri")
	if redirectURL == "" {
		redirectURL = "https://oauth-redirect.googleusercontent.com/r/" + h.service.projectID
	}

	http.Redirect(w, r, redirectURL+"?code="+code+"&state="+state, http.StatusFound)
}

// ReportState manually reports device state to Google
func (h *Handler) ReportState(w http.ResponseWriter, r *http.Request) {
	if !h.service.IsEnabled() {
		http.Error(w, "Google Home integration disabled", http.StatusServiceUnavailable)
		return
	}

	var request struct {
		UserID   string                 `json:"user_id"`
		DeviceID string                 `json:"device_id"`
		State    map[string]interface{} `json:"state"`
	}

	if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
		http.Error(w, "Invalid request", http.StatusBadRequest)
		return
	}

	if err := h.service.ReportState(request.UserID, request.DeviceID, request.State); err != nil {
		h.logger.Error("Failed to report state", zap.Error(err))
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{"status": "success"})
}

// RequestSync manually requests device sync from Google
func (h *Handler) RequestSync(w http.ResponseWriter, r *http.Request) {
	if !h.service.IsEnabled() {
		http.Error(w, "Google Home integration disabled", http.StatusServiceUnavailable)
		return
	}

	var request struct {
		UserID string `json:"user_id"`
	}

	if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
		http.Error(w, "Invalid request", http.StatusBadRequest)
		return
	}

	if err := h.service.RequestSync(request.UserID); err != nil {
		h.logger.Error("Failed to request sync", zap.Error(err))
		http.Error(w, err.Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
	json.NewEncoder(w).Encode(map[string]string{"status": "success"})
}
