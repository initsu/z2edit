package(default_visibility = ["//visibility:public"])

cc_library(
    name = "gamecontrollerdb",
    srcs = ["gamecontrollerdb.cc"],
    hdrs = ["gamecontrollerdb.h"],
)

cc_library(
    name = "browser",
    srcs = [
        "browser.cc",
    ],
    hdrs = [
        "browser.h",
    ],
)

cc_library(
    name = "compress",
    srcs = [
        "compress.cc",
    ],
    hdrs = [
        "compress.h",
    ],
    deps = [
        "@zlib//:zlib",
        "//util:logging",
        "//util:status",
    ],
)

cc_library(
    name = "config",
    hdrs = [
        "config.h",
    ],
    deps = [
        "//util:file",
        "//util:logging",
        "//util:os",
    ],
)

cc_library(
    name = "crc",
    srcs = ["crc.cc"],
    hdrs = ["crc.h"],
    deps = [
        "@zlib//:zlib",
    ],
)

cc_library(
    name = "file",
    srcs = [
        "file.cc",
    ],
    hdrs = [
        "file.h",
    ],
    deps = [
        ":status",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "logging",
    srcs = [
        "logging.cc",
    ],
    hdrs = [
        "logging.h",
    ],
    deps = [
        "@gflags//:gflags",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "os",
    srcs = [
        "os.cc",
    ],
    hdrs = [
        "os.h",
    ],
    deps = [
        ":logging",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "status",
    srcs = [
        "status.cc",
    ],
    hdrs = [
        "status.h",
        "statusor.h",
    ],
    deps = [
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
    ],
)

cc_library(
    name = "fpsmgr",
    hdrs = [
        "fpsmgr.h",
    ],
)

cc_library(
    name = "macros",
    hdrs = [
        "macros.h",
    ],
)

cc_library(
    name = "stb-tilemap-editor",
    hdrs = [
        "stb_tilemap_editor.h",
    ],
)

cc_library(
    name = "stb-perlin",
    hdrs = [
        "stb_perlin.h",
    ],
)

cc_library(
    name = "imgui_sdl_opengl",
    srcs = [
        "imgui_impl_opengl3.cpp",
        "imgui_impl_sdl2.cpp",
        "imgui_hidpi.cc",
    ],
    hdrs = [
        "imgui_impl_opengl3.h",
        "imgui_impl_opengl3_loader.h",
        "imgui_impl_sdl2.h",
        "imgui_hidpi.h",
    ],
    deps = [
        "@imgui_git//:imgui",
        "@sdl2_ext//:sdl2",
    ],
)
