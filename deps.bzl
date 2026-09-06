"""Repository definitions for third-party code not available as Bzlmod modules."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "new_git_repository")
load("@bazel_tools//tools/build_defs/repo:local.bzl", "local_repository")

def _deps_impl(mctx):
    # SDL2 (official mingw devel package)
    http_archive(
        name = "sdl2_ext",
        build_file = "//rules:sdl.BUILD",
        strip_prefix = "SDL2-2.32.10",
        sha256 = "83a5d74012311edc3c0d40ea6faecbe57ad692aa033fa5dc273cc937e3938ff2",
        urls = [
            "https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-devel-2.32.10-mingw.tar.gz",
        ],
    )

    # imgui
    new_git_repository(
        name = "imgui_git",
        build_file = "//rules:imgui.BUILD",
        remote = "https://github.com/ocornut/imgui.git",
        tag = "v1.92.9b",
    )

    # IconFontCppHeaders
    new_git_repository(
        name = "iconfonts",
        build_file = "//rules:iconfonts.BUILD",
        commit = "fda5f470b767f7b413e4a3995fa8cfe47f78b586",
        remote = "https://github.com/juliettef/IconFontCppHeaders.git",
    )

    # native file dialog
    new_git_repository(
        name = "nativefiledialog_git",
        build_file = "//rules:nfd.BUILD",
        commit = "5cfe5002eb0fac1e49777a17dec70134147931e2",
        remote = "https://github.com/mlabbe/nativefiledialog.git",
    )

    # compilers for windows
    http_archive(
        name = "llvm_mingw",
        sha256 = "cee8d2ce3da5145ce4dc882e70d0b0719a783d53a99752c60948fc0659975a65",
        strip_prefix = "llvm-mingw-20260826-ucrt-ubuntu-22.04-x86_64",
        urls = [
            "https://github.com/mstorsjo/llvm-mingw/releases/download/20260826/llvm-mingw-20260826-ucrt-ubuntu-22.04-x86_64.tar.xz",
        ],
        build_file = "//rules:llvm_mingw.BUILD",
    )

    # protobuf's protobuf.bzl loads rules_ruby for its own (unused) ruby test
    # targets.  Provide a no-op stand-in so the load resolves.
    local_repository(
        name = "rules_ruby",
        path = "rules/rules_ruby_stub",
    )

deps = module_extension(implementation = _deps_impl)