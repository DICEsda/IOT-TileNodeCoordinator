load("@rules_go//go:def.bzl", "go_library")

go_library(
    name = "db",
    srcs = ["db.go"],
    importpath = "github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/db",
    visibility = ["//visibility:public"],
    deps = [
        "//internal/config",
        "@org_mongodb_go_mongo_driver//mongo",
        "@org_mongodb_go_mongo_driver//mongo/options",
        "@org_uber_go_fx//:fx",
    ],
)
