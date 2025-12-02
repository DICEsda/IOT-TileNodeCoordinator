package main

import (
	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/config"
	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/db"
	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/googlehome"
	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/http"
	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/mqtt"
	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/repository"

	"github.com/gorilla/mux"
	"go.uber.org/fx"
	"go.uber.org/zap"
)

func main() {
	fx.New(
		http.Module,
		mqtt.Module,
		googlehome.Module,
		fx.Provide(
			zap.NewProduction,
			db.NewMongoDB,
			mux.NewRouter,
			config.NewConfig,
			fx.Annotate(
				repository.NewMongoRepository,
				fx.As(new(repository.Repository)),
			),
		),
	).Run()
}
