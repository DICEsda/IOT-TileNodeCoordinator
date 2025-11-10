package repository

import (
	"context"
	"errors"
	"fmt"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/types"
	"go.mongodb.org/mongo-driver/bson"
	"go.mongodb.org/mongo-driver/mongo"
	"go.mongodb.org/mongo-driver/mongo/options"
	"go.uber.org/zap"
)

var _ Repository = (*MongoRepository)(nil)

type MongoRepository struct {
	db     *mongo.Database
	logger *zap.Logger
}

func NewMongoRepository(db *mongo.Database, logger *zap.Logger) *MongoRepository {
	return &MongoRepository{
		db:     db,
		logger: logger,
	}
}

func (r *MongoRepository) GetCoordinatorById(id string) (*types.Coordinator, error) {
	ctx := context.Background()
	coll := r.db.Collection("coordinators")
	coordinator := &types.Coordinator{}
	if err := coll.FindOne(ctx, bson.M{"_id": id}).Decode(coordinator); err != nil {
		if errors.Is(err, mongo.ErrNoDocuments) {
			return nil, fmt.Errorf("coordinator with id %s not found: %w", id, err)
		}
		return nil, err
	}
	return coordinator, nil
}

func (r *MongoRepository) GetNodeById(id string) (*types.Node, error) {
	ctx := context.Background()
	coll := r.db.Collection("nodes")
	node := &types.Node{}
	if err := coll.FindOne(ctx, bson.M{"_id": id}).Decode(node); err != nil {
		if errors.Is(err, mongo.ErrNoDocuments) {
			return nil, fmt.Errorf("node with id %s not found: %w", id, err)
		}
		return nil, err
	}
	return node, nil
}

func (r *MongoRepository) GetOTAJobById(id string) (*types.OTAJob, error) {
	ctx := context.Background()

	coll := r.db.Collection("ota_jobs")
	otaJob := &types.OTAJob{}
	if err := coll.FindOne(ctx, bson.M{"_id": id}).Decode(otaJob); err != nil {
		if errors.Is(err, mongo.ErrNoDocuments) {
			return nil, fmt.Errorf("ota job with id %s not found: %w", id, err)
		}
		return nil, err
	}
	return otaJob, nil
}

func (r *MongoRepository) GetSites() ([]types.Site, error) {
	ctx := context.Background()

	coll := r.db.Collection("sites")
	cursor, err := coll.Find(ctx, struct{}{})
	if err != nil {
		return nil, err
	}

	results := []types.Site{}
	if err = cursor.All(ctx, &results); err != nil {
		return nil, err
	}
	return results, nil
}

func (r *MongoRepository) GetSiteById(id string) (*types.Site, error) {
	ctx := context.Background()
	coll := r.db.Collection("sites")
	site := &types.Site{}
	if err := coll.FindOne(ctx, bson.M{"_id": id}).Decode(site); err != nil {
		return nil, err
	}
	return site, nil
}

func (r *MongoRepository) UpsertCoordinator(ctx context.Context, coordinator *types.Coordinator) error {
	coll := r.db.Collection("coordinators")
	update := bson.M{"$set": coordinator}
	opts := options.Update().SetUpsert(true)
	if _, err := coll.UpdateOne(ctx, bson.M{"_id": coordinator.Id}, update, opts); err != nil {
		r.logger.Error("Failed to upsert coordinator", zap.Error(err))
		return err
	}
	return nil
}

func (r *MongoRepository) UpsertNode(ctx context.Context, node *types.Node) error {
	coll := r.db.Collection("nodes")
	update := bson.M{"$set": node}
	opts := options.Update().SetUpsert(true)
	if _, err := coll.UpdateOne(ctx, bson.M{"_id": node.Id}, update, opts); err != nil {
		r.logger.Error("Failed to upsert node", zap.Error(err))
		return err
	}
	return nil
}

func (r *MongoRepository) CreateOTAJob(ctx context.Context, job *types.OTAJob) error {
	coll := r.db.Collection("ota_jobs")
	_, err := coll.InsertOne(ctx, job)
	if err != nil {
		r.logger.Error("Failed to create OTA job", zap.Error(err))
		return err
	}
	return nil
}

func (r *MongoRepository) UpdateOTAJobStatus(ctx context.Context, id string, status types.Status) error {
	coll := r.db.Collection("ota_jobs")
	update := bson.M{"$set": bson.M{
		"status":     status,
		"updated_at": context.Background(),
	}}
	if _, err := coll.UpdateOne(ctx, bson.M{"_id": id}, update); err != nil {
		r.logger.Error("Failed to update OTA job status", zap.Error(err))
		return err
	}
	return nil
}

func (r *MongoRepository) CreateSite(ctx context.Context, site *types.Site) error {
	coll := r.db.Collection("sites")
	_, err := coll.InsertOne(ctx, site)
	if err != nil {
		r.logger.Error("Failed to create site", zap.Error(err))
		return err
	}
	return nil
}
