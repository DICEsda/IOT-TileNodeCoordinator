package repository

import (
	"context"
	"testing"
	"time"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/types"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/bson/primitive"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
)

// TestRepository_CreateCoordinator tests coordinator creation
func TestRepository_CreateCoordinator(t *testing.T) {
	repo, cleanup := setupTestRepository(t)
	defer cleanup()

	ctx := context.Background()
	coord := &types.Coordinator{
		Name:     "TestCoordinator",
		SiteID:   "site-001",
		Location: "TestLab",
		Status:   "online",
	}

	id, err := repo.CreateCoordinator(ctx, coord)
	if err != nil {
		t.Fatalf("CreateCoordinator failed: %v", err)
	}

	if id == "" {
		t.Error("Expected non-empty coordinator ID")
	}

	// Verify coordinator was created
	retrieved, err := repo.GetCoordinator(ctx, id)
	if err != nil {
		t.Fatalf("GetCoordinator failed: %v", err)
	}

	if retrieved.Name != coord.Name {
		t.Errorf("Expected name %s, got %s", coord.Name, retrieved.Name)
	}
	if retrieved.SiteID != coord.SiteID {
		t.Errorf("Expected siteID %s, got %s", coord.SiteID, retrieved.SiteID)
	}
}

// TestRepository_UpdateCoordinator tests coordinator updates
func TestRepository_UpdateCoordinator(t *testing.T) {
	repo, cleanup := setupTestRepository(t)
	defer cleanup()

	ctx := context.Background()
	coord := &types.Coordinator{
		Name:     "OriginalName",
		SiteID:   "site-002",
		Location: "Lab",
		Status:   "online",
	}

	id, err := repo.CreateCoordinator(ctx, coord)
	if err != nil {
		t.Fatalf("CreateCoordinator failed: %v", err)
	}

	// Update coordinator
	coord.ID = id
	coord.Name = "UpdatedName"
	coord.Status = "offline"

	if err := repo.UpdateCoordinator(ctx, coord); err != nil {
		t.Fatalf("UpdateCoordinator failed: %v", err)
	}

	// Verify update
	retrieved, err := repo.GetCoordinator(ctx, id)
	if err != nil {
		t.Fatalf("GetCoordinator failed: %v", err)
	}

	if retrieved.Name != "UpdatedName" {
		t.Errorf("Expected name UpdatedName, got %s", retrieved.Name)
	}
	if retrieved.Status != "offline" {
		t.Errorf("Expected status offline, got %s", retrieved.Status)
	}
}

// TestRepository_CreateNode tests node creation
func TestRepository_CreateNode(t *testing.T) {
	repo, cleanup := setupTestRepository(t)
	defer cleanup()

	ctx := context.Background()
	node := &types.Node{
		MAC:          "AA:BB:CC:DD:EE:FF",
		CoordinatorID: "coord-001",
		Zone:         1,
		Status:       "online",
		LastSeen:     time.Now(),
	}

	id, err := repo.CreateNode(ctx, node)
	if err != nil {
		t.Fatalf("CreateNode failed: %v", err)
	}

	if id == "" {
		t.Error("Expected non-empty node ID")
	}

	// Verify node was created
	retrieved, err := repo.GetNode(ctx, id)
	if err != nil {
		t.Fatalf("GetNode failed: %v", err)
	}

	if retrieved.MAC != node.MAC {
		t.Errorf("Expected MAC %s, got %s", node.MAC, retrieved.MAC)
	}
	if retrieved.Zone != node.Zone {
		t.Errorf("Expected zone %d, got %d", node.Zone, retrieved.Zone)
	}
}

// TestRepository_GetNodesByCoordinator tests filtering nodes by coordinator
func TestRepository_GetNodesByCoordinator(t *testing.T) {
	repo, cleanup := setupTestRepository(t)
	defer cleanup()

	ctx := context.Background()
	coordID := "coord-test-001"

	// Create multiple nodes
	nodes := []types.Node{
		{MAC: "11:11:11:11:11:11", CoordinatorID: coordID, Zone: 1, Status: "online"},
		{MAC: "22:22:22:22:22:22", CoordinatorID: coordID, Zone: 2, Status: "online"},
		{MAC: "33:33:33:33:33:33", CoordinatorID: "other-coord", Zone: 1, Status: "online"},
	}

	for _, node := range nodes {
		n := node
		if _, err := repo.CreateNode(ctx, &n); err != nil {
			t.Fatalf("CreateNode failed: %v", err)
		}
	}

	// Get nodes for specific coordinator
	retrieved, err := repo.GetNodesByCoordinator(ctx, coordID)
	if err != nil {
		t.Fatalf("GetNodesByCoordinator failed: %v", err)
	}

	if len(retrieved) != 2 {
		t.Errorf("Expected 2 nodes, got %d", len(retrieved))
	}

	for _, node := range retrieved {
		if node.CoordinatorID != coordID {
			t.Errorf("Expected coordinator ID %s, got %s", coordID, node.CoordinatorID)
		}
	}
}

// TestRepository_SaveTelemetry tests telemetry storage
func TestRepository_SaveTelemetry(t *testing.T) {
	repo, cleanup := setupTestRepository(t)
	defer cleanup()

	ctx := context.Background()
	telemetry := &types.Telemetry{
		NodeID:      "node-001",
		Temperature: 25.5,
		Brightness:  75,
		Voltage:     3.3,
		Timestamp:   time.Now(),
	}

	if err := repo.SaveTelemetry(ctx, telemetry); err != nil {
		t.Fatalf("SaveTelemetry failed: %v", err)
	}

	// Verify telemetry was saved (implementation dependent)
	// This would require a GetTelemetry method
}

// TestRepository_DeleteCoordinator tests coordinator deletion
func TestRepository_DeleteCoordinator(t *testing.T) {
	repo, cleanup := setupTestRepository(t)
	defer cleanup()

	ctx := context.Background()
	coord := &types.Coordinator{
		Name:     "ToDelete",
		SiteID:   "site-delete",
		Location: "Nowhere",
		Status:   "online",
	}

	id, err := repo.CreateCoordinator(ctx, coord)
	if err != nil {
		t.Fatalf("CreateCoordinator failed: %v", err)
	}

	// Delete coordinator
	if err := repo.DeleteCoordinator(ctx, id); err != nil {
		t.Fatalf("DeleteCoordinator failed: %v", err)
	}

	// Verify deletion
	_, err = repo.GetCoordinator(ctx, id)
	if err == nil {
		t.Error("Expected error when getting deleted coordinator")
	}
}

// setupTestRepository creates a test repository with in-memory or test DB
func setupTestRepository(t *testing.T) (Repository, func()) {
	t.Helper()

	// Use in-memory MongoDB or test database
	// For real tests, you might want to use testcontainers or a test MongoDB instance
	clientOptions := options.Client().ApplyURI("mongodb://localhost:27017")
	client, err := mongo.Connect(context.Background(), clientOptions)
	if err != nil {
		t.Skipf("Skipping test: MongoDB not available: %v", err)
	}

	// Test connection
	ctx, cancel := context.WithTimeout(context.Background(), 2*time.Second)
	defer cancel()
	if err := client.Ping(ctx, nil); err != nil {
		client.Disconnect(context.Background())
		t.Skipf("Skipping test: MongoDB not reachable: %v", err)
	}

	testDB := client.Database("iot_test_" + primitive.NewObjectID().Hex())
	repo := NewMongoRepository(testDB)

	cleanup := func() {
		testDB.Drop(context.Background())
		client.Disconnect(context.Background())
	}

	return repo, cleanup
}

// TestRepository_ConcurrentWrites tests concurrent write safety
func TestRepository_ConcurrentWrites(t *testing.T) {
	repo, cleanup := setupTestRepository(t)
	defer cleanup()

	ctx := context.Background()
	coordID := "concurrent-coord"
	coord := &types.Coordinator{
		Name:     "ConcurrentTest",
		SiteID:   coordID,
		Location: "TestLab",
		Status:   "online",
	}

	id, err := repo.CreateCoordinator(ctx, coord)
	if err != nil {
		t.Fatalf("CreateCoordinator failed: %v", err)
	}

	// Simulate concurrent updates
	done := make(chan bool)
	for i := 0; i < 5; i++ {
		go func(idx int) {
			coord.Status = "update-" + string(rune(idx))
			repo.UpdateCoordinator(ctx, coord)
			done <- true
		}(i)
	}

	// Wait for all goroutines
	for i := 0; i < 5; i++ {
		<-done
	}

	// Verify coordinator still exists
	_, err = repo.GetCoordinator(ctx, id)
	if err != nil {
		t.Errorf("Coordinator lost after concurrent updates: %v", err)
	}
}

// Benchmark tests
func BenchmarkRepository_CreateNode(b *testing.B) {
	repo, cleanup := setupTestRepositoryBench(b)
	defer cleanup()

	ctx := context.Background()
	node := &types.Node{
		MAC:          "AA:BB:CC:DD:EE:FF",
		CoordinatorID: "bench-coord",
		Zone:         1,
		Status:       "online",
	}

	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		node.MAC = primitive.NewObjectID().Hex()
		repo.CreateNode(ctx, node)
	}
}

func setupTestRepositoryBench(b *testing.B) (Repository, func()) {
	b.Helper()

	clientOptions := options.Client().ApplyURI("mongodb://localhost:27017")
	client, err := mongo.Connect(context.Background(), clientOptions)
	if err != nil {
		b.Skip("MongoDB not available")
	}

	testDB := client.Database("iot_bench_" + primitive.NewObjectID().Hex())
	repo := NewMongoRepository(testDB)

	cleanup := func() {
		testDB.Drop(context.Background())
		client.Disconnect(context.Background())
	}

	return repo, cleanup
}
