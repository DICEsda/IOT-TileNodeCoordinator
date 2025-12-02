package types

import (
	"encoding/json"
	"testing"
	"time"
)

// TestCoordinator_JSON tests coordinator JSON serialization
func TestCoordinator_JSON(t *testing.T) {
	coord := Coordinator{
		ID:       "coord-001",
		Name:     "TestCoordinator",
		SiteID:   "site-001",
		Location: "Lab A",
		Status:   "online",
	}

	// Marshal to JSON
	data, err := json.Marshal(coord)
	if err != nil {
		t.Fatalf("Failed to marshal coordinator: %v", err)
	}

	// Unmarshal back
	var decoded Coordinator
	if err := json.Unmarshal(data, &decoded); err != nil {
		t.Fatalf("Failed to unmarshal coordinator: %v", err)
	}

	if decoded.Name != coord.Name {
		t.Errorf("Expected name %s, got %s", coord.Name, decoded.Name)
	}
	if decoded.SiteID != coord.SiteID {
		t.Errorf("Expected siteID %s, got %s", coord.SiteID, decoded.SiteID)
	}
}

// TestNode_JSON tests node JSON serialization
func TestNode_JSON(t *testing.T) {
	now := time.Now()
	node := Node{
		ID:            "node-001",
		MAC:           "AA:BB:CC:DD:EE:FF",
		CoordinatorID: "coord-001",
		Zone:          1,
		Status:        "online",
		LastSeen:      now,
	}

	data, err := json.Marshal(node)
	if err != nil {
		t.Fatalf("Failed to marshal node: %v", err)
	}

	var decoded Node
	if err := json.Unmarshal(data, &decoded); err != nil {
		t.Fatalf("Failed to unmarshal node: %v", err)
	}

	if decoded.MAC != node.MAC {
		t.Errorf("Expected MAC %s, got %s", node.MAC, decoded.MAC)
	}
	if decoded.Zone != node.Zone {
		t.Errorf("Expected zone %d, got %d", node.Zone, decoded.Zone)
	}
}

// TestTelemetry_JSON tests telemetry JSON serialization
func TestTelemetry_JSON(t *testing.T) {
	telemetry := Telemetry{
		NodeID:      "node-001",
		Temperature: 25.5,
		Brightness:  80,
		Voltage:     3.3,
		Timestamp:   time.Now(),
	}

	data, err := json.Marshal(telemetry)
	if err != nil {
		t.Fatalf("Failed to marshal telemetry: %v", err)
	}

	var decoded Telemetry
	if err := json.Unmarshal(data, &decoded); err != nil {
		t.Fatalf("Failed to unmarshal telemetry: %v", err)
	}

	if decoded.Temperature != telemetry.Temperature {
		t.Errorf("Expected temperature %.2f, got %.2f", telemetry.Temperature, decoded.Temperature)
	}
	if decoded.Brightness != telemetry.Brightness {
		t.Errorf("Expected brightness %d, got %d", telemetry.Brightness, decoded.Brightness)
	}
}

// TestCommand_JSON tests command JSON serialization
func TestCommand_JSON(t *testing.T) {
	cmd := Command{
		Cmd:        "pair",
		DurationMS: 60000,
		NodeID:     "node-001",
		Zone:       1,
	}

	data, err := json.Marshal(cmd)
	if err != nil {
		t.Fatalf("Failed to marshal command: %v", err)
	}

	var decoded Command
	if err := json.Unmarshal(data, &decoded); err != nil {
		t.Fatalf("Failed to unmarshal command: %v", err)
	}

	if decoded.Cmd != cmd.Cmd {
		t.Errorf("Expected cmd %s, got %s", cmd.Cmd, decoded.Cmd)
	}
	if decoded.DurationMS != cmd.DurationMS {
		t.Errorf("Expected duration %d, got %d", cmd.DurationMS, decoded.DurationMS)
	}
}

// TestNodeTelemetry_Validation tests node telemetry validation
func TestNodeTelemetry_Validation(t *testing.T) {
	tests := []struct {
		name      string
		telemetry NodeTelemetry
		wantValid bool
	}{
		{
			name: "valid telemetry",
			telemetry: NodeTelemetry{
				NodeID:      "node-001",
				Temperature: 25.0,
				Brightness:  80,
				Red:         255,
				Green:       128,
				Blue:        64,
				White:       200,
				Voltage:     3.3,
			},
			wantValid: true,
		},
		{
			name: "invalid temperature",
			telemetry: NodeTelemetry{
				NodeID:      "node-001",
				Temperature: -273.16, // Below absolute zero
				Brightness:  80,
			},
			wantValid: false,
		},
		{
			name: "invalid brightness",
			telemetry: NodeTelemetry{
				NodeID:     "node-001",
				Brightness: 256, // Out of range
			},
			wantValid: false,
		},
		{
			name: "invalid voltage",
			telemetry: NodeTelemetry{
				NodeID:  "node-001",
				Voltage: -1.0,
			},
			wantValid: false,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			valid := tt.telemetry.Validate()
			if valid != tt.wantValid {
				t.Errorf("Expected valid=%v, got %v", tt.wantValid, valid)
			}
		})
	}
}

// TestCoordinatorTelemetry_Validation tests coordinator telemetry validation
func TestCoordinatorTelemetry_Validation(t *testing.T) {
	tests := []struct {
		name      string
		telemetry CoordinatorTelemetry
		wantValid bool
	}{
		{
			name: "valid telemetry",
			telemetry: CoordinatorTelemetry{
				CoordinatorID: "coord-001",
				Light:         450,
				Temperature:   26.5,
				WiFiRSSI:      -65,
				MQTTConnected: true,
			},
			wantValid: true,
		},
		{
			name: "invalid RSSI",
			telemetry: CoordinatorTelemetry{
				CoordinatorID: "coord-001",
				WiFiRSSI:      10, // RSSI should be negative
			},
			wantValid: false,
		},
	}

	for _, tt := range tests {
		t.Run(tt.name, func(t *testing.T) {
			valid := tt.telemetry.Validate()
			if valid != tt.wantValid {
				t.Errorf("Expected valid=%v, got %v", tt.wantValid, valid)
			}
		})
	}
}

// TestZoneConfig_JSON tests zone configuration serialization
func TestZoneConfig_JSON(t *testing.T) {
	config := ZoneConfig{
		ZoneID:    1,
		Name:      "Living Room",
		NodeIDs:   []string{"node-001", "node-002"},
		Enabled:   true,
		Occupancy: true,
	}

	data, err := json.Marshal(config)
	if err != nil {
		t.Fatalf("Failed to marshal zone config: %v", err)
	}

	var decoded ZoneConfig
	if err := json.Unmarshal(data, &decoded); err != nil {
		t.Fatalf("Failed to unmarshal zone config: %v", err)
	}

	if decoded.Name != config.Name {
		t.Errorf("Expected name %s, got %s", config.Name, decoded.Name)
	}
	if len(decoded.NodeIDs) != len(config.NodeIDs) {
		t.Errorf("Expected %d nodes, got %d", len(config.NodeIDs), len(decoded.NodeIDs))
	}
}

// TestMMWaveEvent_JSON tests mmWave event serialization
func TestMMWaveEvent_JSON(t *testing.T) {
	event := MMWaveEvent{
		CoordinatorID: "coord-001",
		Detected:      true,
		Distance:      150,
		Energy:        75,
		Timestamp:     time.Now(),
	}

	data, err := json.Marshal(event)
	if err != nil {
		t.Fatalf("Failed to marshal mmWave event: %v", err)
	}

	var decoded MMWaveEvent
	if err := json.Unmarshal(data, &decoded); err != nil {
		t.Fatalf("Failed to unmarshal mmWave event: %v", err)
	}

	if decoded.Detected != event.Detected {
		t.Errorf("Expected detected %v, got %v", event.Detected, decoded.Detected)
	}
	if decoded.Distance != event.Distance {
		t.Errorf("Expected distance %d, got %d", event.Distance, decoded.Distance)
	}
}

// TestMACAddress_Validation tests MAC address format validation
func TestMACAddress_Validation(t *testing.T) {
	tests := []struct {
		mac   string
		valid bool
	}{
		{"AA:BB:CC:DD:EE:FF", true},
		{"aa:bb:cc:dd:ee:ff", true},
		{"AA-BB-CC-DD-EE-FF", true},
		{"AABBCCDDEEFF", false},       // No separators
		{"AA:BB:CC:DD:EE", false},     // Too short
		{"AA:BB:CC:DD:EE:FF:GG", false}, // Too long
		{"GG:HH:II:JJ:KK:LL", false}, // Invalid hex
		{"", false},                   // Empty
	}

	for _, tt := range tests {
		t.Run(tt.mac, func(t *testing.T) {
			valid := IsValidMAC(tt.mac)
			if valid != tt.valid {
				t.Errorf("MAC %s: expected valid=%v, got %v", tt.mac, tt.valid, valid)
			}
		})
	}
}

// TestSystemStatus tests system status aggregation
func TestSystemStatus(t *testing.T) {
	status := SystemStatus{
		Online:        true,
		Coordinators:  3,
		Nodes:         15,
		ActiveZones:   5,
		LastUpdate:    time.Now(),
		HealthStatus:  "healthy",
	}

	if !status.Online {
		t.Error("Expected system to be online")
	}

	if status.Coordinators != 3 {
		t.Errorf("Expected 3 coordinators, got %d", status.Coordinators)
	}
}

// Benchmark tests
func BenchmarkTelemetry_Marshal(b *testing.B) {
	telemetry := NodeTelemetry{
		NodeID:      "node-bench",
		Temperature: 25.5,
		Brightness:  80,
		Red:         255,
		Green:       128,
		Blue:        64,
		White:       200,
		Voltage:     3.3,
		Timestamp:   time.Now(),
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		json.Marshal(telemetry)
	}
}

func BenchmarkCommand_Unmarshal(b *testing.B) {
	data := []byte(`{"cmd":"pair","duration_ms":60000,"node_id":"node-001","zone":1}`)

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		var cmd Command
		json.Unmarshal(data, &cmd)
	}
}
