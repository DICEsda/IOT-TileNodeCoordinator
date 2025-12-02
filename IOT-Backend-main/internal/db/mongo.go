package db

import (
	"context"
	"log"
	"time"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/config"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
	"go.uber.org/fx"
)

type MongoDBResult struct {
	fx.Out

	MongoDB *mongo.Database
}

func NewMongoDB(cfg *config.Config) (MongoDBResult, error) {
	ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
	defer cancel()
	
	log.Printf("Connecting to MongoDB at: %s", cfg.DB.URI)
	client, err := mongo.Connect(ctx, options.Client().ApplyURI(cfg.DB.URI))
	if err != nil {
		log.Fatalf("Failed to connect to MongoDB: %v", err)
		return MongoDBResult{}, err
	}
	
	// Ping to verify connection
	if err := client.Ping(ctx, nil); err != nil {
		log.Fatalf("Failed to ping MongoDB: %v", err)
		return MongoDBResult{}, err
	}
	
	log.Printf("Connected to MongoDB, using database: %s", cfg.DB.Database)
	db := client.Database(cfg.DB.Database)
	return MongoDBResult{MongoDB: db}, nil
}
