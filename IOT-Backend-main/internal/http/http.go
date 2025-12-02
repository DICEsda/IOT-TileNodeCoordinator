package http

import (
	"context"
	"log"
	"net/http"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/config"
	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/types"
	"github.com/gorilla/mux"
	"go.uber.org/fx"
)

// CORS middleware to allow frontend access
func corsMiddleware(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		// Allow requests from localhost:4200 (Angular dev server)
		origin := r.Header.Get("Origin")
		if origin == "" {
			origin = "*"
		}
		w.Header().Set("Access-Control-Allow-Origin", origin)
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type, Authorization")

		// Handle preflight requests
		if r.Method == "OPTIONS" {
			w.WriteHeader(http.StatusOK)
			return
		}

		next.ServeHTTP(w, r)
	})
}

var Module = fx.Options(
	fx.Provide(
		NewRadarCache,
		fx.Annotate(
			NewRadarCache,
			fx.As(new(RadarCacheProvider)),
		),
		NewHandler,
		NewWSBroadcaster,
	),
	fx.Invoke(RegisterHandlers),
	fx.Invoke(StartBroadcaster),
	fx.Invoke(NewHTTPServer),
)

type RadarCacheProvider interface {
	Set(siteId, coordId string, frame *types.MmwaveFrame)
	Get(siteId, coordId string) *types.MmwaveFrame
}

func StartBroadcaster(lc fx.Lifecycle, broadcaster *WSBroadcaster) {
	lc.Append(fx.Hook{
		OnStart: func(ctx context.Context) error {
			broadcaster.Start()
			return nil
		},
	})
}

func NewHTTPServer(lc fx.Lifecycle, r *mux.Router, cfg *config.Config) {
	handler := corsMiddleware(r)

	srv := &http.Server{
		Addr:    cfg.HTTP.Addr,
		Handler: handler,
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
