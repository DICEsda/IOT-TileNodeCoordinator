package types

import (
	"time"

	"go.mongodb.org/mongo-driver/bson/primitive"
)

type StatusMode string

const (
	Operational StatusMode = "operational"
	Pairing     StatusMode = "pairing"
	Ota         StatusMode = "ota"
	Error       StatusMode = "error"
)

type Node struct {
	Id            string     `bson:"_id,omitempty" json:"_id"`
	LightId       string     `bson:"light_id" json:"light_id"`
	StatusMode    StatusMode `bson:"status_mode" json:"status_mode"`
	AvgR          int        `bson:"avg_r" json:"avg_r"`
	TempC         float32    `bson:"temp_c" json:"temp_c"`
	VbatMv        int        `bson:"vbat_mv" json:"vbat_mv"`
	FwVersion     string     `bson:"fw_version" json:"fw_version"`
	LastSeen      time.Time  `bson:"last_seen" json:"last_seen"`
	SiteId        string     `bson:"site_id" json:"site_id"`
	CoordinatorId string     `bson:"coordinator_id" json:"coordinator_id"`
}

type Coordinator struct {
	Id              string    `bson:"_id,omitempty" json:"_id"`
	CoordId         string    `bson:"coord_id" json:"coord_id"`
	SiteId          string    `bson:"site_id" json:"site_id"`
	FwVersion       string    `bson:"fw_version" json:"fw_version"`
	NodesOnline     int       `bson:"nodes_online" json:"nodes_online"`
	WifiRssi        int       `bson:"wifi_rssi" json:"wifi_rssi"`
	MmwaveEventRate float32   `bson:"mmwave_event_rate" json:"mmwave_event_rate"`
	LightLux        float32   `bson:"light_lux" json:"light_lux"`
	TempC           float32   `bson:"temp_c" json:"temp_c"`
	LastSeen        time.Time `bson:"last_seen" json:"last_seen"`
}

type Site struct {
	Id        string    `bson:"_id,omitempty" json:"_id"`
	Name      string    `bson:"name" json:"name"`
	Location  string    `bson:"location" json:"location"`
	Config    string    `bson:"config" json:"config"`
	CreatedAt time.Time `bson:"created_at" json:"created_at"`
	UpdatedAt time.Time `bson:"updated_at" json:"updated_at"`
}

type Telemetry struct {
	Ts       time.Time `bson:"ts" json:"ts"`
	DeviceId string    `bson:"device_id" json:"device_id"`
	Payload  string    `bson:"payload" json:"payload"`
}

type MmwaveTarget struct {
	ID           int     `bson:"id" json:"id"`
	DistanceMM   int     `bson:"distance_mm" json:"distance_mm"`
	SpeedCmS     int     `bson:"speed_cm_s" json:"speed_cm_s"`
	ResolutionMM int     `bson:"resolution_mm" json:"resolution_mm"`
	PositionXMM  int     `bson:"position_x_mm" json:"position_x_mm"`
	PositionYMM  int     `bson:"position_y_mm" json:"position_y_mm"`
	VelocityXMPS float32 `bson:"velocity_x_m_s" json:"velocity_x_m_s"`
	VelocityYMPS float32 `bson:"velocity_y_m_s" json:"velocity_y_m_s"`
}

type MmwaveFrame struct {
	ID            primitive.ObjectID `bson:"_id,omitempty" json:"_id"`
	SiteId        string             `bson:"site_id" json:"site_id"`
	CoordinatorId string             `bson:"coordinator_id" json:"coordinator_id"`
	SensorId      string             `bson:"sensor_id" json:"sensor_id"`
	Presence      bool               `bson:"presence" json:"presence"`
	Confidence    float32            `bson:"confidence" json:"confidence"`
	Targets       []MmwaveTarget     `bson:"targets" json:"targets"`
	Timestamp     time.Time          `bson:"timestamp" json:"timestamp"`
}

type TargetType string

const (
	NodeTarget        TargetType = "node"
	CoordinatorTarget TargetType = "coordinator"
)

type Status string

const (
	OTAStatusPending    Status = "pending"
	OTAStatusInProgress Status = "in_progress"
	OTAStatusSuccess    Status = "success"
	OTAStatusRollback   Status = "rollback"
	OTAStatusFailed     Status = "failed"
)

type OTAJob struct {
	Id            string     `bson:"_id,omitempty" json:"_id"`
	TargetType    TargetType `bson:"target_type" json:"target_type"`
	TargetVersion string     `bson:"target_version" json:"target_version"`
	Status        Status     `bson:"status" json:"status"`
	CreatedAt     time.Time  `bson:"created_at" json:"created_at"`
	UpdatedAt     time.Time  `bson:"updated_at" json:"updated_at"`
}
