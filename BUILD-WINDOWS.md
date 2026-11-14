# Building Z2Edit for Windows

This project now uses CMake. It can be built in Windows using either
Visual Studio or MinGW. Both produce a fully static `z2edit.exe` that
needs no DLLs next to it.

---

## Dependencies are fetched from source (CPM)

**CPM.cmake** fetches dependencies (SDL3, protobuf, absl, gflags, zlib, imgui,
and nativefiledialog) during configure. CMake compiles them from source during
the build. Versions are pinned in `cmake/dependencies.cmake`, and Git clones
are cached under `%USERPROFILE%\.cpm` and shared between build presets.

The first configure needs Git on `PATH` and access to GitHub. Later builds
are incremental.

Run `python cpm_check_updates.py` to see which pins have newer stable tags.
The checker requires Python 3.9+, Git, and access to GitHub.

---

## Visual Studio Requirements (`build_vs_release.bat`)

- Visual Studio with the **"Desktop development with C++"** workload.
- The bundled `cmake.exe` and `ninja.exe` are detected and used automatically.

- If building from within Visual Studio, select z2edit.exe as the target.

From the repository root, run `.\build_vs_release.bat`.

---

## MinGW Requirements (`build_mingw.bat`)

- `g++`, `cmake`, and `ninja` on `PATH` when the script runs. The easy way is
  MSYS2:
  ```
  pacman -S --needed mingw-w64-ucrt-x86_64-gcc \
                     mingw-w64-ucrt-x86_64-cmake \
                     mingw-w64-ucrt-x86_64-ninja
  ```
  and run the script from the **"MSYS2 UCRT64"** shell. [winlibs] also works.

[winlibs]: https://winlibs.com/

---

|                   | MSVC / Visual Studio            | MinGW                          |
|-------------------|---------------------------------|--------------------------------|
| Script            | `build_vs_release.bat`          | `build_mingw.bat`              |
| CMake Preset      | `windows-msvc-static-release`   | `windows-mingw`                |
| Compiler          | `cl.exe` (VS C++ workload)      | `g++` (MSYS2 ucrt64/winlibs)   |
| Static CRT        | `/MT` (forced by the preset)    | n/a (MinGW is static by default) |
| Output            | `build\windows-msvc\z2edit.exe` | `build\windows-mingw\z2edit.exe` |

Both scripts must be run from the repository root and package the executable
as `z2edit-windows.zip`.

The first build compiles all dependencies and can take 20–40 minutes.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| Configure fails downloading a dependency | Check that Git is on `PATH` and GitHub is reachable, then rerun the build script. |
| `LNK2038: mismatch detected for 'RuntimeLibrary'` | Linked objects use different CRT settings. Use `build_vs_release.bat` for the release build with `/MT`. |
| `error: can't load cache ... __VERSION` | The build directory may contain a stale CMake cache. Delete `build\windows-msvc` and rerun the build script. |
| Very long first build | Compiling protobuf and abseil takes time. Later builds reuse the compiled objects and cached sources. |
