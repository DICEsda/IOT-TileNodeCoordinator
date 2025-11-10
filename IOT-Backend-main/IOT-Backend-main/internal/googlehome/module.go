package googlehome

import (
	"go.uber.org/fx"
)

var Module = fx.Module("googlehome",
	fx.Provide(
		NewGoogleHomeService,
		NewHandler,
	),
	fx.Invoke(RegisterHandlers),
)
