package http

import (
	"context"
	"log"
	"net/http"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/config"
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

var Module = fx.Module("http",
	fx.Provide(
		NewHandler,
	),
	fx.Invoke(RegisterHandlers),
	fx.Invoke(NewHTTPServer),
)

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
