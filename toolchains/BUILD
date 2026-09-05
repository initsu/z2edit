package(default_visibility = ["//visibility:public"])

platform(
    name = "win64",
    constraint_values = [
        "@platforms//cpu:x86_64",
        "@platforms//os:windows",
    ],
)

toolchain(
    name = "win64_toolchain",
    exec_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:linux",
    ],
    target_compatible_with = [
        "@platforms//cpu:x86_64",
        "@platforms//os:windows",
    ],
    toolchain = "@llvm_mingw//:win64_llvm_toolchain",
    toolchain_type = "@bazel_tools//tools/cpp:toolchain_type",
)