package http

import (
	"context"
	"log"
	"net/http"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/config"
	"github.com/gorilla/mux"
	"go.uber.org/fx"
)

var Module = fx.Module("http",
	fx.Provide(
		NewHandler,
	),
	fx.Invoke(RegisterHandlers),
	fx.Invoke(NewHTTPServer),
)

func NewHTTPServer(lc fx.Lifecycle, r *mux.Router, cfg *config.Config) {
	srv := &http.Server{
		Addr:    cfg.HTTP.Addr,
		Handler: r,
	}

	lc.Append(fx.Hook{
		OnStart: func(ctx context.Context) error {
			go func() {
				if err := srv.ListenAndServe(); err != nil {
					log.Fatal(err)
				}
			}()
			return nil
		},
		OnStop: func(ctx context.Context) error {
			return srv.Shutdown(ctx)
		},
	})
}
