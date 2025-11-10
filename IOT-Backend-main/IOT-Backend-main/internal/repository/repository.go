package repository

import (
	"context"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/types"
)

type Repository interface {
	// Coordinator operations
	GetCoordinatorById(id string) (*types.Coordinator, error)
	UpsertCoordinator(ctx context.Context, coordinator *types.Coordinator) error

	// Node operations
	GetNodeById(id string) (*types.Node, error)
	UpsertNode(ctx context.Context, node *types.Node) error

	// OTA operations
	GetOTAJobById(id string) (*types.OTAJob, error)
	CreateOTAJob(ctx context.Context, job *types.OTAJob) error
	UpdateOTAJobStatus(ctx context.Context, id string, status types.Status) error

	// Site operations
	GetSites() ([]types.Site, error)
	GetSiteById(id string) (*types.Site, error)
	CreateSite(ctx context.Context, site *types.Site) error
}
