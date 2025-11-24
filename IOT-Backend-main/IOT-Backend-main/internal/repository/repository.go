package repository

import (
	"context"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/types"
)

type Repository interface {
	// Coordinator operations
	GetCoordinatorById(id string) (*types.Coordinator, error)
	GetCoordinatorBySiteAndId(siteId string, coordId string) (*types.Coordinator, error)
	UpsertCoordinator(ctx context.Context, coordinator *types.Coordinator) error

	// Node operations
	GetNodeById(id string) (*types.Node, error)
	GetNodesByCoordinator(siteId string, coordId string) ([]*types.Node, error)
	UpsertNode(ctx context.Context, node *types.Node) error
	DeleteNode(siteId string, coordId string, nodeId string) error
	UpdateNodeZone(siteId string, coordId string, nodeId string, zoneId string) error
	UpdateNodeName(siteId string, coordId string, nodeId string, name string) error

	// mmWave operations
	InsertMmwaveFrame(ctx context.Context, frame *types.MmwaveFrame) error
	GetMmwaveFrames(ctx context.Context, siteId string, coordinatorId string, limit int) ([]types.MmwaveFrame, error)

	// OTA operations
	GetOTAJobById(id string) (*types.OTAJob, error)
	CreateOTAJob(ctx context.Context, job *types.OTAJob) error
	UpdateOTAJobStatus(ctx context.Context, id string, status types.Status) error

	// Site operations
	GetSites() ([]types.Site, error)
	GetSiteById(id string) (*types.Site, error)
	CreateSite(ctx context.Context, site *types.Site) error
	UpsertSite(ctx context.Context, site *types.Site) error

	// Settings operations
	GetSettings(siteId string) (*Settings, error)
	SaveSettings(settings *Settings) error
}

// Settings represents system settings (moved here from http package)
type Settings struct {
	SiteID            string   `bson:"site_id" json:"site_id"`
	AutoMode          bool     `bson:"auto_mode" json:"auto_mode"`
	MotionSensitivity int      `bson:"motion_sensitivity" json:"motion_sensitivity"`
	LightIntensity    int      `bson:"light_intensity" json:"light_intensity"`
	AutoOffDelay      int      `bson:"auto_off_delay" json:"auto_off_delay"`
	Zones             []string `bson:"zones" json:"zones"`
	MqttBroker        string   `bson:"mqtt_broker,omitempty" json:"mqtt_broker,omitempty"`
	MqttUsername      string   `bson:"mqtt_username,omitempty" json:"mqtt_username,omitempty"`
	GoogleHomeEnabled bool     `bson:"google_home_enabled" json:"google_home_enabled"`
	GoogleProjectID   string   `bson:"google_project_id,omitempty" json:"google_project_id,omitempty"`
	GoogleClientID    string   `bson:"google_client_id,omitempty" json:"google_client_id,omitempty"`
	GoogleClientSecret string  `bson:"google_client_secret,omitempty" json:"google_client_secret,omitempty"`
	GoogleApiKey      string   `bson:"google_api_key,omitempty" json:"google_api_key,omitempty"`
}
