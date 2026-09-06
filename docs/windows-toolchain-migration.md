# Windows toolchain migration: mxebzl -> llvm-mingw (UCRT), on Bzlmod

Status: **done** (cross-compiles `//:z2edit` for Windows + packages the zip)
Depends on: Bazel 7.7.1
Goal: remove `@mxebzl` (gcc 5.5 + 2019 MXE snapshot) and move to a maintained,
modern cross-compiler + explicit SDK deps, upgraded protobuf/absl/gflags, and
a Bzlmod repo layout (Bazel 9 readiness).

---

## What mxebzl provided (removed)

mxebzl bundled three unrelated things into one `git_repository`
(`tag 20191215_RC01`, python2-based downloader):

| Piece | Source | What it gave us |
|---|---|---|
| Cross-compiler | `mxe_compiler` repo rule | x86_64-w64-mingw32 **gcc 5.5.0** via `driver/*` |
| Prebuilt Windows SDK libs | 2019 MXE snapshot (`tools/downloader.py`) | SDL2 (+image/mixer/gfx/net/ttf), freetype/etc, headers in the sysroot |
| Packaging | `pkg_winzip` + `tools/zip4win.py` | EXE + DLL bundle in a zip |

## What replaced it (current state)

`WORKSPACE` has been deleted. The repo layout is Bzlmod-only:

```
MODULE.bazel  (root module "z2edit")
├── bazel_dep: platforms 0.0.10, bazel_skylib 1.7.1, rules_cc 0.0.x,
├──            rules_proto 4.0.0, protobuf 24.4, abseil-cpp 20240722.0,
├──            gflags 2.2.2.bcr.1, zlib 1.2.11
├── single_version_override(gflags): rules/gflags.patch (config_setting ->
│       @platforms//os:windows constraint) + rules/gflags-module.patch
│       (make the gflags module depend on @platforms so the label resolves)
└── use_extension //:deps.bzl  -> llvm_mingw, sdl2_ext, imgui_git,
        iconfonts, nativefiledialog_git (git/http archives) + rules_ruby stub
```

Key files:
- `toolchains/cc_toolchain_config.bzl` - `llvm_mingw_cc_toolchain_config` rule.
- `rules/llvm_mingw.BUILD` - external-repo build file: whole-sysroot filegroup +
  per-tool filegroups + `cc_toolchain`. Loads the config rule as `@//toolchains:...`.
- `rules/sdl.BUILD` - `sdl2` (static `libSDL2.a` + `includes`) and `sdl2main`
  (`libSDL2main.a`). The static `libSDL2.a` means **no SDL2.dll at runtime**.
  Declares `hdrs = glob(["x86_64-w64-mingw32/include/**"])` so the includes are
  visible inside the sandbox (pre-existing bug, fixed during this migration).
- `MODULE.bazel` / `deps.bzl` - the module + extension (replaces WORKSPACE).
- `rules/rules_ruby_stub/` - no-op `ruby_library` stub; `protobuf.bzl` loads
  `@rules_ruby//ruby:defs.bzl` for its (unused) ruby test targets. The old
  self-hosted `rules_python_stub` is gone - real rules_python comes in via the
  protobuf module's own dependencies.
- `BUILD` - `make_version` genrule, `z2edit` cc_binary, and the
  `z2edit-windows` genrule zip (replaces `pkg_winzip`).
- `util/fpsmgr.h` - rewritten without SDL_gfx.
- `util/status.h`/`util/statusor.h` - migrated from `google/protobuf/stubs/status`
  (`util::Status`, `.ValueOrDie()`) to `absl::Status` / `absl::StatusOr`,
  with a `util::error::Code` alias of `absl::StatusCode`; call sites updated
  (`.value()`, `absl::StatusCode::kX` enumerators).

### Toolchain design decisions
- `gcc` and `ld` tool paths both point at `bin/x86_64-w64-mingw32-g++` (a symlink
  to llvm-mingw's `clang-target-wrapper.sh`, which adds `--driver-mode=g++` and
  `-target x86_64-w64-mingw32`), so the libc++ runtime link is automatic.
- `-x c`/`-x c++` (with gnu11/gnu++14) are forced because the g++ driver mode
  otherwise treats `.c` files as C++. protobuf 24.4 still builds under C++14
  (v25+ would force a C++17 bump).
- Include dirs (`libc++` c++/v1, target CRT, generic CRT, clang resource dir
  `lib/clang/23/include`) are passed as `--sysroot <abs>` + per-dir `-isystem`.
- **`compiler = "mingw-gcc"`** in the toolchain config. This is the value that
  `config_setting(values = {compiler: "mingw-gcc"})` selects match, e.g. absl's
  `//absl:mingw_compiler` -> `-ldbghelp` linkopts for `//absl/debugging:symbolize`
  (`SymFromAddr`). With `compiler = "clang"` those selects fell through to the
  default and the Windows binary failed to link.
- **Bazel include validator gotcha:** `cxx_builtin_include_directories` only
  accepts ABSOLUTE execroot paths. The toolchain repo path is derived at analysis
  time as `<EXECUTION_ROOT>/<workspace_root>` via `--define EXECUTION_ROOT=$(bazel
  info execution_root)` (see `tools/release_windows.sh` and .bazelrc comments).
  `DEFAULT_EXECUTION_ROOT` is the documented `/home/marcus/z2bazel/execroot/_main`
  fallback so plain builds without the define still work on this machine. Bzlmod's
  execroot layout is `external/_main` for the root repo and
  `external/_main~deps~llvm_mingw` (etc.) for extension-created repos, so
  `ctx.label.workspace_root` must be used rather than a hardcoded `external/...</...>`.
- `artifact_name_patterns = [executable -> ".exe"]` - required because the clang
  driver appends `.exe` itself; without it bazel complains
  "output 'z2edit' was not created".
- `-DABSL_FORCE_THREAD_IDENTITY_MODE=2` (USE_CPP11) - still required on modern
  absl: its MinGW default picks the pthread_setspecific path even though 2024-era
  absl ships pthreads-win32 support. The C++11 TLS mode (same as MSVC) avoids it.
- gflags: the BCR module keeps a `config_setting(name = "x64_windows", values =
  {"cpu": "x64_windows"})`, which a non-MSVC toolchain never satisfies. Pinned
  via `single_version_override` with a patch switching it to
  `@platforms//os:windows`, and a second patch adding a `platforms` bazel_dep so
  the label is visible from the gflags module (repo visibility follows the module
  dependency graph in Bzlmod).

### Fixes made to get the app building under clang 23
- `util/BUILD`: `crc`/`compress` link zlib via `@zlib//:zlib` dep instead of
  `-lz` (no libz in the llvm-mingw sysroot).
- `util/BUILD` `//util:status`: protobuf dep replaced with
  `@com_google_absl//absl/status` + `absl/status:statusor` after the Status API
  migration (protobuf v24 removed `google/protobuf/stubs/status.h|statusor.h`).
- `rules/sdl.BUILD`: `includes = ["x86_64-w64-mingw32/include"]` so
  `<SDL2/SDL.h>` resolves; declared the headers as `hdrs` for sandbox visibility
  (the includes were silently ignored by the sandbox before); added
  `sdl2main` lib + `bind(sdl2main)`; `@sdl2_ext//:sdl2`/`//:sdl2main` wired into
  `app`/`z2edit` and `//util:imgui_sdl_opengl`/`//imwidget:base`/
  `//imwidget:glbitmap`.
- `BUILD` `make_version`: the old sed emitted `FORMATTED_DATE "2026 "Sep ...`
  (invalid C++); now an awk that quotes each line's whole value.
- `BUILD` `z2edit-windows` genrule: explicitly zips `z2edit.exe` +
  `libc++.dll` + `libunwind.dll` (the exe's only non-OS imports; the
  `api-ms-win-crt-*` entries are the Windows 10+ UCRT forwarders).

### Bzlmod migration notes
- `.bazelrc` has `common --enable_workspace=false`; there is no WORKSPACE file.
- All `//external:*` labels became their repo identities: `@gflags//:gflags`,
  `@imgui_git//:imgui`, `@sdl2_ext//:sdl2` / `:sdl2main`,
  `@nativefiledialog_git//:nfd`, `@iconfonts//:fontawesome`.
- The absl repo name is still `@com_google_absl` (module `abseil-cpp`); protobuf
  is `@protobuf` (module `protobuf`).
- `x64_windows-opt` config naming: `bazel-out/x64_windows-opt/...` for the win64
  config; host-config outputs (genrule zip) land under
  `bazel-out/k8-fastbuild/bin/`.

### Build commands (WSL2 / Debian 13, user `marcus`, output base `/home/marcus/z2bazel`)
```
cd /mnt/c/source/z2edit
# release script version (sets EXECUTION_ROOT + versioned zip name):
tools/release_windows.sh

# or by hand:
/home/marcus/bazel-7.7.1-linux-x86_64 \
  --output_base=/home/marcus/z2bazel \
  build --config win64 \
  --define EXECUTION_ROOT=/home/marcus/z2bazel/execroot/_main \
  //:z2edit-windows
```
Artifact: `bazel-out/k8-fastbuild/bin/z2edit-windows.zip` (z2edit.exe +
libc++.dll + libunwind.dll).

## Remaining / follow-ons
- Manual smoke test on a real Windows box (extract zip, launch editor).
- `build-help2026.txt`: python2 section removed (whole downloader path is gone);
  the direct-build command is updated with `--define EXECUTION_ROOT`.
- Later tasks: ARM64 Windows; a non-WSL-required `.bat` packaging script.

## Upgrades achieved by this migration
| Component | Before | After |
|---|---|---|
| protobuf | 3.11.4 (WORKSPACE git_repository) | 24.4 (BCR module) |
| absl | 2021-era WORKSPACE pin | 20240722.0 (BCR module) |
| gflags | 2.2.2 + hand-written CPU patch | 2.2.2.bcr.1 + constraint-based override patch |
| repo layout | WORKSPACE + rules_python/rules_ruby stubs | MODULE.bazel/deps.bzl extension (bzlmod) |