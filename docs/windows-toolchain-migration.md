# Windows toolchain migration: mxebzl -> llvm-mingw (UCRT)

Status: **done** (fully cross-compiles `//:z2edit` for Windows + packages the zip)
Depends on: Bazel 7.7.1 work already committed (`34d86d5`, `7210680`)
Goal: remove `@mxebzl` (gcc 5.5 + 2019 MXE snapshot) and move to a maintained,
modern cross-compiler + explicit SDK deps. This is the prerequisite for
upgrading protobuf beyond 3.11.4 and for Bazel 9 (both currently blocked by
gcc 5.5).

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

```
WORKSPACE
├── llvm_mingw (http_archive: 20260826 UCRT tarball, sha256-pinned, build_file=rules/llvm_mingw.BUILD)
├── sdl2_ext  (http_archive: SDL2-devel-2.32.10-mingw, sha256-pinned, build_file=rules/sdl.BUILD)
├── (everything else unchanged: gflags, imgui, iconfonts, absl, protobuf 3.11.4,
│    rules_cc, rules_proto, nfd, zlib, platforms)
└── register_toolchains("//toolchains:win64_toolchain")
```

Key files:
- `toolchains/cc_toolchain_config.bzl` - `llvm_mingw_cc_toolchain_config` rule.
- `rules/llvm_mingw.BUILD` - external-repo build file: whole-sysroot filegroup +
  per-tool filegroups + `cc_toolchain`. Loads the config rule as `@//toolchains:...`.
- `rules/sdl.BUILD` - `sdl2` (static `libSDL2.a` + `includes`) and `sdl2main`
  (`libSDL2main.a`). The static `libSDL2.a` means **no SDL2.dll at runtime**.
- `BUILD` - `make_version` genrule (fixed quoting), `z2edit` cc_binary, and the
  `z2edit-windows` genrule zip (replaces `pkg_winzip`).
- `util/fpsmgr.h` - rewritten without SDL_gfx.

### Toolchain design decisions
- `gcc` and `ld` tool paths both point at `bin/x86_64-w64-mingw32-g++` (a symlink
  to llvm-mingw's `clang-target-wrapper.sh`, which adds `--driver-mode=g++` and
  `-target x86_64-w64-mingw32`), so the libc++ runtime link is automatic.
- `-x c`/`-x c++` (with gnu11/gnu++14) are forced because the g++ driver mode
  otherwise treats `.c` files as C++.
- Include dirs (`libc++` c++/v1, target CRT, generic CRT, clang resource dir
  `lib/clang/23/include`) are passed as `--sysroot <abs>` + per-dir `-isystem`.
- **Bazel include validator gotcha:** `cxx_builtin_include_directories` only
  accepts ABSOLUTE execroot paths (relative `external/...` forms fail with
  "undeclared inclusion(s)"). `ABSROOT` in the bzl is therefore hardcoded to
  `$output_base/execroot/__main__/external/llvm_mingw` and the build must use the
  matching `--output_base=/home/marcus/z2bazel` (see scripts). This is the one
  machine-specific wart; if CI differs, change `ABSROOT` and the output base
  together.
- `artifact_name_patterns = [executable -> ".exe"]` - required because the clang
  driver appends `.exe` itself; without it bazel complains
  "output 'z2edit' was not created".
- `-DABSL_FORCE_THREAD_IDENTITY_MODE=2` (USE_CPP11) - absl's MinGW default picks
  the pthread_setspecific path whose `pthread_key_t` code `<pthread.h>` never
  includes under `_WIN32`. The C++11 TLS mode (same as MSVC) avoids pthreads.

### Fixes made to get the app building under clang 23
- `util/BUILD`: `crc`/`compress` link zlib via `@zlib//:zlib` dep instead of
  `-lz` (no libz in the llvm-mingw sysroot).
- `rules/sdl.BUILD`: `includes = ["x86_64-w64-mingw32/include"]` so
  `<SDL2/SDL.h>` resolves (headers used to live in the mxe sysroot); added
  `sdl2main` lib + `bind(sdl2main)`; `//external:sdl2`/`//external:sdl2main`
  wired into `app`/`z2edit` and `//util:imgui_sdl_opengl`/`//imwidget:base`/
  `//imwidget:glbitmap`.
- `BUILD` `make_version`: the old sed emitted `FORMATTED_DATE "2026 "Sep ...`
  (invalid C++); now an awk that quotes each line's whole value.
- `BUILD` `z2edit-windows` genrule: explicitly zips `z2edit.exe` +
  `libc++.dll` + `libunwind.dll` (the exe's only non-OS imports; the
  `api-ms-win-crt-*` entries are the Windows 10+ UCRT forwarders).

### Build commands (WSL2 / Debian 13, user `marcus`, output base `/home/marcus/z2bazel`)
```
cd /mnt/c/source/z2edit
/home/marcus/bazel-7.7.1-linux-x86_64 --output_base=/home/marcus/z2bazel \
  build --noenable_bzlmod //:z2edit-windows --platforms=//toolchains:win64 --verbose_failures
```
Artifact: `bazel-out/k8-fastbuild/bin/z2edit-windows.zip` (z2edit.exe +
libc++.dll + libunwind.dll). Both default-sandbox and `--spawn_strategy=local`
build cleanly.

## Remaining / follow-ons
- Manual smoke test on a real Windows box (extract zip, launch editor).
- `build-help2026.txt`: drop the python2 install section (whole downloader path is gone).
- `tools/release_windows.sh` may need updating if it references the old zip path/name.
- Later tasks (unchanged from before): protobuf upgrade, Bzlmod migration, ARM64 Windows.
- Portability cleanup if a CI/non-`marcus` host is ever used: `ABSROOT` +
  `--output_base` must be set consistently.