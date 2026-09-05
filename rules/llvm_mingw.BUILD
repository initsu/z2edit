package(default_visibility = ["//visibility:public"])

# This is the BUILD file for the llvm-mingw toolchain repository.  The
# tool_paths in //toolchains:cc_toolchain_config.bzl are relative to this
# repository's root (external/llvm_mingw in the execroot), and the driver
# binaries in bin/ locate the UCRT sysroot next to them.

load("@rules_cc//cc:defs.bzl", "cc_toolchain")
load("@//toolchains:cc_toolchain_config.bzl", "llvm_mingw_cc_toolchain_config")

filegroup(name = "empty")

filegroup(
    name = "all",
    srcs = glob(["**"]),
)

filegroup(
    name = "as_tool",
    srcs = ["bin/x86_64-w64-mingw32-as"],
)

filegroup(
    name = "ar_tool",
    srcs = ["bin/x86_64-w64-mingw32-ar"],
)

filegroup(
    name = "cc_tool",
    srcs = ["bin/x86_64-w64-mingw32-g++"],
)

filegroup(
    name = "cpp_tool",
    srcs = ["bin/x86_64-w64-mingw32-gcc"],
)

filegroup(
    name = "gcov_tool",
    srcs = ["bin/x86_64-w64-mingw32-gcc"],
)

filegroup(
    name = "ld_tool",
    srcs = ["bin/x86_64-w64-mingw32-g++"],
)

filegroup(
    name = "nm_tool",
    srcs = ["bin/x86_64-w64-mingw32-nm"],
)

filegroup(
    name = "objcopy_tool",
    srcs = ["bin/x86_64-w64-mingw32-objcopy"],
)

filegroup(
    name = "objdump_tool",
    srcs = ["bin/x86_64-w64-mingw32-objdump"],
)

filegroup(
    name = "strip_tool",
    srcs = ["bin/x86_64-w64-mingw32-strip"],
)

llvm_mingw_cc_toolchain_config(name = "win64_llvm_config")

cc_toolchain(
    name = "win64_llvm_toolchain",
    toolchain_identifier = "llvm-mingw-x86_64",
    toolchain_config = ":win64_llvm_config",
    all_files = ":all",
    ar_files = ":ar_tool",
    as_files = ":as_tool",
    compiler_files = ":cc_tool",
    dwp_files = ":empty",
    linker_files = ":ld_tool",
    objcopy_files = ":objcopy_tool",
    strip_files = ":strip_tool",
    supports_param_files = 0,
)