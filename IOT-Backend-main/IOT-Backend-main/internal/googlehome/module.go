package googlehome

import (
	"go.uber.org/fx"
)

var Module = fx.Options(
	fx.Provide(
		NewGoogleHomeService,
		NewHandler,
	),
	fx.Invoke(RegisterHandlers),
)
