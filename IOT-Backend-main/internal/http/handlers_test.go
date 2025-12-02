package http

import (
	"bytes"
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"testing"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/repository"
	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/types"
	"github.com/gorilla/mux"
)

// MockRepository implements the repository interface for testing
type MockRepository struct {
	coordinators map[string]*types.Coordinator
	nodes        map[string]*types.Node
	telemetry    []types.Telemetry
}

func NewMockRepository() *MockRepository {
	return &MockRepository{
		coordinators: make(map[string]*types.Coordinator),
		nodes:        make(map[string]*types.Node),
		telemetry:    make([]types.Telemetry, 0),
	}
}

func (m *MockRepository) CreateCoordinator(ctx context.Context, coord *types.Coordinator) (string, error) {
	id := "coord-" + coord.Name
	coord.ID = id
	m.coordinators[id] = coord
	return id, nil
}

func (m *MockRepository) GetCoordinator(ctx context.Context, id string) (*types.Coordinator, error) {
	if coord, exists := m.coordinators[id]; exists {
		return coord, nil
	}
	return nil, repository.ErrNotFound
}

func (m *MockRepository) UpdateCoordinator(ctx context.Context, coord *types.Coordinator) error {
	if _, exists := m.coordinators[coord.ID]; exists {
		m.coordinators[coord.ID] = coord
		return nil
	}
	return repository.ErrNotFound
}

func (m *MockRepository) DeleteCoordinator(ctx context.Context, id string) error {
	delete(m.coordinators, id)
	return nil
}

func (m *MockRepository) ListCoordinators(ctx context.Context) ([]*types.Coordinator, error) {
	coords := make([]*types.Coordinator, 0, len(m.coordinators))
	for _, coord := range m.coordinators {
		coords = append(coords, coord)
	}
	return coords, nil
}

func (m *MockRepository) CreateNode(ctx context.Context, node *types.Node) (string, error) {
	id := "node-" + node.MAC
	node.ID = id
	m.nodes[id] = node
	return id, nil
}

func (m *MockRepository) GetNode(ctx context.Context, id string) (*types.Node, error) {
	if node, exists := m.nodes[id]; exists {
		return node, nil
	}
	return nil, repository.ErrNotFound
}

func (m *MockRepository) GetNodesByCoordinator(ctx context.Context, coordID string) ([]*types.Node, error) {
	nodes := make([]*types.Node, 0)
	for _, node := range m.nodes {
		if node.CoordinatorID == coordID {
			nodes = append(nodes, node)
		}
	}
	return nodes, nil
}

func (m *MockRepository) SaveTelemetry(ctx context.Context, telemetry *types.Telemetry) error {
	m.telemetry = append(m.telemetry, *telemetry)
	return nil
}

// TestHandler_GetCoordinators tests listing all coordinators
func TestHandler_GetCoordinators(t *testing.T) {
	mockRepo := NewMockRepository()
	handler := &Handler{repo: mockRepo}

	// Add test coordinators
	mockRepo.CreateCoordinator(context.Background(), &types.Coordinator{
		Name:   "Coord1",
		SiteID: "site-001",
		Status: "online",
	})
	mockRepo.CreateCoordinator(context.Background(), &types.Coordinator{
		Name:   "Coord2",
		SiteID: "site-002",
		Status: "offline",
	})

	req := httptest.NewRequest(http.MethodGet, "/api/coordinators", nil)
	w := httptest.NewRecorder()

	handler.GetCoordinators(w, req)

	if w.Code != http.StatusOK {
		t.Errorf("Expected status 200, got %d", w.Code)
	}

	var coordinators []types.Coordinator
	if err := json.NewDecoder(w.Body).Decode(&coordinators); err != nil {
		t.Fatalf("Failed to decode response: %v", err)
	}

	if len(coordinators) != 2 {
		t.Errorf("Expected 2 coordinators, got %d", len(coordinators))
	}
}

// TestHandler_CreateCoordinator tests coordinator creation endpoint
func TestHandler_CreateCoordinator(t *testing.T) {
	mockRepo := NewMockRepository()
	handler := &Handler{repo: mockRepo}

	coord := types.Coordinator{
		Name:     "NewCoord",
		SiteID:   "site-003",
		Location: "Lab A",
		Status:   "online",
	}

	body, _ := json.Marshal(coord)
	req := httptest.NewRequest(http.MethodPost, "/api/coordinators", bytes.NewReader(body))
	req.Header.Set("Content-Type", "application/json")
	w := httptest.NewRecorder()

	handler.CreateCoordinator(w, req)

	if w.Code != http.StatusCreated {
		t.Errorf("Expected status 201, got %d", w.Code)
	}

	var response map[string]string
	if err := json.NewDecoder(w.Body).Decode(&response); err != nil {
		t.Fatalf("Failed to decode response: %v", err)
	}

	if response["id"] == "" {
		t.Error("Expected non-empty coordinator ID in response")
	}
}

// TestHandler_GetCoordinator tests getting a single coordinator
func TestHandler_GetCoordinator(t *testing.T) {
	mockRepo := NewMockRepository()
	handler := &Handler{repo: mockRepo}

	id, _ := mockRepo.CreateCoordinator(context.Background(), &types.Coordinator{
		Name:   "TestCoord",
		SiteID: "site-test",
		Status: "online",
	})

	req := httptest.NewRequest(http.MethodGet, "/api/coordinators/"+id, nil)
	req = mux.SetURLVars(req, map[string]string{"id": id})
	w := httptest.NewRecorder()

	handler.GetCoordinator(w, req)

	if w.Code != http.StatusOK {
		t.Errorf("Expected status 200, got %d", w.Code)
	}

	var coord types.Coordinator
	if err := json.NewDecoder(w.Body).Decode(&coord); err != nil {
		t.Fatalf("Failed to decode response: %v", err)
	}

	if coord.Name != "TestCoord" {
		t.Errorf("Expected name TestCoord, got %s", coord.Name)
	}
}

// TestHandler_UpdateCoordinator tests coordinator update endpoint
func TestHandler_UpdateCoordinator(t *testing.T) {
	mockRepo := NewMockRepository()
	handler := &Handler{repo: mockRepo}

	id, _ := mockRepo.CreateCoordinator(context.Background(), &types.Coordinator{
		Name:   "OriginalName",
		SiteID: "site-update",
		Status: "online",
	})

	updatedCoord := types.Coordinator{
		ID:     id,
		Name:   "UpdatedName",
		SiteID: "site-update",
		Status: "offline",
	}

	body, _ := json.Marshal(updatedCoord)
	req := httptest.NewRequest(http.MethodPut, "/api/coordinators/"+id, bytes.NewReader(body))
	req = mux.SetURLVars(req, map[string]string{"id": id})
	req.Header.Set("Content-Type", "application/json")
	w := httptest.NewRecorder()

	handler.UpdateCoordinator(w, req)

	if w.Code != http.StatusOK {
		t.Errorf("Expected status 200, got %d", w.Code)
	}

	// Verify update
	updated, _ := mockRepo.GetCoordinator(context.Background(), id)
	if updated.Name != "UpdatedName" {
		t.Errorf("Expected name UpdatedName, got %s", updated.Name)
	}
}

// TestHandler_DeleteCoordinator tests coordinator deletion
func TestHandler_DeleteCoordinator(t *testing.T) {
	mockRepo := NewMockRepository()
	handler := &Handler{repo: mockRepo}

	id, _ := mockRepo.CreateCoordinator(context.Background(), &types.Coordinator{
		Name:   "ToDelete",
		SiteID: "site-delete",
		Status: "online",
	})

	req := httptest.NewRequest(http.MethodDelete, "/api/coordinators/"+id, nil)
	req = mux.SetURLVars(req, map[string]string{"id": id})
	w := httptest.NewRecorder()

	handler.DeleteCoordinator(w, req)

	if w.Code != http.StatusNoContent {
		t.Errorf("Expected status 204, got %d", w.Code)
	}

	// Verify deletion
	_, err := mockRepo.GetCoordinator(context.Background(), id)
	if err != repository.ErrNotFound {
		t.Error("Expected coordinator to be deleted")
	}
}

// TestHandler_GetNodesByCoordinator tests getting nodes for a coordinator
func TestHandler_GetNodesByCoordinator(t *testing.T) {
	mockRepo := NewMockRepository()
	handler := &Handler{repo: mockRepo}

	coordID := "coord-test"
	mockRepo.CreateNode(context.Background(), &types.Node{
		MAC:          "11:11:11:11:11:11",
		CoordinatorID: coordID,
		Zone:         1,
		Status:       "online",
	})
	mockRepo.CreateNode(context.Background(), &types.Node{
		MAC:          "22:22:22:22:22:22",
		CoordinatorID: coordID,
		Zone:         2,
		Status:       "online",
	})

	req := httptest.NewRequest(http.MethodGet, "/api/coordinators/"+coordID+"/nodes", nil)
	req = mux.SetURLVars(req, map[string]string{"coordID": coordID})
	w := httptest.NewRecorder()

	handler.GetNodesByCoordinator(w, req)

	if w.Code != http.StatusOK {
		t.Errorf("Expected status 200, got %d", w.Code)
	}

	var nodes []types.Node
	if err := json.NewDecoder(w.Body).Decode(&nodes); err != nil {
		t.Fatalf("Failed to decode response: %v", err)
	}

	if len(nodes) != 2 {
		t.Errorf("Expected 2 nodes, got %d", len(nodes))
	}
}

// TestHandler_InvalidJSON tests handling of invalid JSON
func TestHandler_InvalidJSON(t *testing.T) {
	mockRepo := NewMockRepository()
	handler := &Handler{repo: mockRepo}

	req := httptest.NewRequest(http.MethodPost, "/api/coordinators", bytes.NewReader([]byte("invalid json")))
	req.Header.Set("Content-Type", "application/json")
	w := httptest.NewRecorder()

	handler.CreateCoordinator(w, req)

	if w.Code != http.StatusBadRequest {
		t.Errorf("Expected status 400, got %d", w.Code)
	}
}

// TestHandler_NotFound tests 404 responses
func TestHandler_NotFound(t *testing.T) {
	mockRepo := NewMockRepository()
	handler := &Handler{repo: mockRepo}

	req := httptest.NewRequest(http.MethodGet, "/api/coordinators/nonexistent", nil)
	req = mux.SetURLVars(req, map[string]string{"id": "nonexistent"})
	w := httptest.NewRecorder()

	handler.GetCoordinator(w, req)

	if w.Code != http.StatusNotFound {
		t.Errorf("Expected status 404, got %d", w.Code)
	}
}

// TestHandler_CORS tests CORS headers
func TestHandler_CORS(t *testing.T) {
	mockRepo := NewMockRepository()
	handler := &Handler{repo: mockRepo}

	req := httptest.NewRequest(http.MethodOptions, "/api/coordinators", nil)
	req.Header.Set("Origin", "http://localhost:4200")
	w := httptest.NewRecorder()

	handler.HandleCORS(w, req)

	if w.Header().Get("Access-Control-Allow-Origin") == "" {
		t.Error("Expected CORS headers to be set")
	}
}

// Benchmark tests
func BenchmarkHandler_GetCoordinators(b *testing.B) {
	mockRepo := NewMockRepository()
	handler := &Handler{repo: mockRepo}

	// Add some test data
	for i := 0; i < 100; i++ {
		mockRepo.CreateCoordinator(context.Background(), &types.Coordinator{
			Name:   "Coord",
			SiteID: "site",
			Status: "online",
		})
	}

	req := httptest.NewRequest(http.MethodGet, "/api/coordinators", nil)

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		w := httptest.NewRecorder()
		handler.GetCoordinators(w, req)
	}
}
