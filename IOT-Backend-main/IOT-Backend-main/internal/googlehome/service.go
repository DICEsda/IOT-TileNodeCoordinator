package googlehome

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"time"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/repository"
	"go.uber.org/zap"
	"golang.org/x/oauth2"
	"golang.org/x/oauth2/google"
)

type GoogleHomeService struct {
	logger       *zap.Logger
	repo         repository.Repository
	config       *oauth2.Config
	projectID    string
	apiKey       string
	homeGraphURL string
	enabled      bool
}

func NewGoogleHomeService(logger *zap.Logger, repo repository.Repository) *GoogleHomeService {
	enabled := os.Getenv("GOOGLE_HOME_ENABLED") == "true"

	if !enabled {
		logger.Info("Google Home integration disabled")
		return &GoogleHomeService{
			logger:  logger,
			repo:    repo,
			enabled: false,
		}
	}

	config := &oauth2.Config{
		ClientID:     os.Getenv("GOOGLE_HOME_CLIENT_ID"),
		ClientSecret: os.Getenv("GOOGLE_HOME_CLIENT_SECRET"),
		RedirectURL:  os.Getenv("GOOGLE_OAUTH_REDIRECT_URI"),
		Scopes: []string{
			"https://www.googleapis.com/auth/homegraph",
		},
		Endpoint: google.Endpoint,
	}

	return &GoogleHomeService{
		logger:       logger,
		repo:         repo,
		config:       config,
		projectID:    os.Getenv("GOOGLE_HOME_PROJECT_ID"),
		apiKey:       os.Getenv("GOOGLE_HOME_API_KEY"),
		homeGraphURL: os.Getenv("GOOGLE_HOMEGRAPH_API_URL"),
		enabled:      true,
	}
}

func (s *GoogleHomeService) IsEnabled() bool {
	return s.enabled
}

// SyncRequest handles SYNC intent from Google
func (s *GoogleHomeService) SyncRequest(userID string) (map[string]interface{}, error) {
	if !s.enabled {
		return nil, fmt.Errorf("Google Home integration is disabled")
	}

	s.logger.Info("Processing SYNC request", zap.String("userId", userID))

	// Get all nodes from database
	// TODO: Implement repository method to get all nodes for a user
	devices := []map[string]interface{}{
		{
			"id":   "node-001",
			"type": "action.devices.types.LIGHT",
			"traits": []string{
				"action.devices.traits.OnOff",
				"action.devices.traits.Brightness",
				"action.devices.traits.ColorSetting",
			},
			"name": map[string]interface{}{
				"defaultNames": []string{"Smart Tile"},
				"name":         "Living Room Tile",
				"nicknames":    []string{"Tile", "Light"},
			},
			"willReportState": true,
			"roomHint":        "Living Room",
			"deviceInfo": map[string]interface{}{
				"manufacturer": "Smart Tile",
				"model":        os.Getenv("GOOGLE_HOME_DEVICE_MODEL"),
				"hwVersion":    "1.0",
				"swVersion":    "1.0.0",
			},
			"attributes": map[string]interface{}{
				"colorModel": "rgb",
			},
		},
	}

	return map[string]interface{}{
		"agentUserId": userID,
		"devices":     devices,
	}, nil
}

// QueryRequest handles QUERY intent from Google
func (s *GoogleHomeService) QueryRequest(userID string, deviceIDs []string) (map[string]interface{}, error) {
	if !s.enabled {
		return nil, fmt.Errorf("Google Home integration is disabled")
	}

	s.logger.Info("Processing QUERY request",
		zap.String("userId", userID),
		zap.Strings("devices", deviceIDs))

	devices := make(map[string]interface{})

	for _, deviceID := range deviceIDs {
		// TODO: Get actual device state from MongoDB
		devices[deviceID] = map[string]interface{}{
			"online":     true,
			"on":         true,
			"brightness": 80,
			"color": map[string]interface{}{
				"spectrumRgb": 16711680, // Red
			},
		}
	}

	return map[string]interface{}{
		"devices": devices,
	}, nil
}

// ExecuteRequest handles EXECUTE intent from Google
func (s *GoogleHomeService) ExecuteRequest(userID string, commands []Command) (map[string]interface{}, error) {
	if !s.enabled {
		return nil, fmt.Errorf("Google Home integration is disabled")
	}

	s.logger.Info("Processing EXECUTE request",
		zap.String("userId", userID),
		zap.Int("commandCount", len(commands)))

	var commandResults []map[string]interface{}

	for _, cmd := range commands {
		for _, device := range cmd.Devices {
			for _, execution := range cmd.Execution {
				result := s.executeCommand(device.ID, execution.Command, execution.Params)
				commandResults = append(commandResults, result)
			}
		}
	}

	return map[string]interface{}{
		"commands": commandResults,
	}, nil
}

func (s *GoogleHomeService) executeCommand(deviceID, command string, params map[string]interface{}) map[string]interface{} {
	s.logger.Info("Executing command",
		zap.String("device", deviceID),
		zap.String("command", command),
		zap.Any("params", params))

	// TODO: Send MQTT command to coordinator/node

	switch command {
	case "action.devices.commands.OnOff":
		on := params["on"].(bool)
		s.logger.Info("Setting OnOff", zap.Bool("on", on))

	case "action.devices.commands.BrightnessAbsolute":
		brightness := int(params["brightness"].(float64))
		s.logger.Info("Setting brightness", zap.Int("brightness", brightness))

	case "action.devices.commands.ColorAbsolute":
		if color, ok := params["color"].(map[string]interface{}); ok {
			if rgb, ok := color["spectrumRGB"].(float64); ok {
				s.logger.Info("Setting color", zap.Float64("rgb", rgb))
			}
		}
	}

	return map[string]interface{}{
		"ids":    []string{deviceID},
		"status": "SUCCESS",
		"states": map[string]interface{}{
			"online": true,
			"on":     true,
		},
	}
}

// ReportState sends device state update to Google
func (s *GoogleHomeService) ReportState(userID string, deviceID string, state map[string]interface{}) error {
	if !s.enabled {
		return nil
	}

	s.logger.Info("Reporting state to Google",
		zap.String("userId", userID),
		zap.String("device", deviceID))

	requestBody := map[string]interface{}{
		"requestId":   fmt.Sprintf("req-%d", time.Now().Unix()),
		"agentUserId": userID,
		"payload": map[string]interface{}{
			"devices": map[string]interface{}{
				"states": map[string]interface{}{
					deviceID: state,
				},
			},
		},
	}

	return s.callHomeGraphAPI("devices:reportStateAndNotification", requestBody)
}

// RequestSync requests Google to sync devices
func (s *GoogleHomeService) RequestSync(userID string) error {
	if !s.enabled {
		return nil
	}

	s.logger.Info("Requesting sync from Google", zap.String("userId", userID))

	requestBody := map[string]interface{}{
		"agentUserId": userID,
	}

	return s.callHomeGraphAPI("devices:requestSync", requestBody)
}

func (s *GoogleHomeService) callHomeGraphAPI(endpoint string, body map[string]interface{}) error {
	url := fmt.Sprintf("%s/%s?key=%s", s.homeGraphURL, endpoint, s.apiKey)

	jsonData, err := json.Marshal(body)
	if err != nil {
		return fmt.Errorf("failed to marshal request: %w", err)
	}

	req, err := http.NewRequestWithContext(context.Background(), "POST", url, bytes.NewBuffer(jsonData))
	if err != nil {
		return fmt.Errorf("failed to create request: %w", err)
	}

	req.Header.Set("Content-Type", "application/json")

	client := &http.Client{Timeout: 10 * time.Second}
	resp, err := client.Do(req)
	if err != nil {
		return fmt.Errorf("failed to call HomeGraph API: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusOK {
		body, _ := io.ReadAll(resp.Body)
		return fmt.Errorf("HomeGraph API error: %d - %s", resp.StatusCode, string(body))
	}

	s.logger.Info("Successfully called HomeGraph API", zap.String("endpoint", endpoint))
	return nil
}

// Command structures
type Command struct {
	Devices   []Device    `json:"devices"`
	Execution []Execution `json:"execution"`
}

type Device struct {
	ID string `json:"id"`
}

type Execution struct {
	Command string                 `json:"command"`
	Params  map[string]interface{} `json:"params"`
}
