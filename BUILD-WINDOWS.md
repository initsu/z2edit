# Building Z2Edit for Windows

This project now uses CMake. It can be built in Windows using either
Visual Studio or MinGW. Both produce a fully static `z2edit.exe` that
needs no DLLs next to it.

---

## Dependencies are fetched from source (CPM)

All third-party dependencies (SDL3, protobuf, absl, gflags, zlib, imgui,
IconFontCppHeaders, nativefiledialog) are fetched and compiled from source
through **CPM.cmake** on the first configure. There is no system package
manager, no vcpkg, and no prebuilt binaries. Versions are pinned in
`cmake/dependencies.cmake`; git clones are cached under
`%USERPROFILE%\.cpm` and shared between build presets.

First configure needs internet. Everything after that is incremental.

---

## Visual Studio Requirements (`build_vs.bat`)

- Visual Studio with the **"Desktop development with C++"** workload.
- The bundled `cmake.exe` and `ninja.exe` are detected and used automatically.

- If building from within Visual Studio, select z2edit.exe as the target.

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
| Script            | `build_vs.bat`                  | `build_mingw.bat`              |
| CMake Preset      | `windows-msvc-static`           | `windows-mingw`                |
| Compiler          | `cl.exe` (VS C++ workload)      | `g++` (MSYS2 ucrt64/winlibs)   |
| Static CRT        | `/MT` (forced by the preset)    | n/a (MinGW is static by default) |
| Output            | `build\windows-msvc\z2edit.exe` | `build\windows-mingw\z2edit.exe` |

Both scripts must be run from the repository root.

First run of either build script compiles all dependencies; expect roughly
20-40 minutes (MSVC, release+debug) and a few gigabytes under the preset's
`build\...\_deps\`. Later runs are incremental.

## Troubleshooting

| Symptom | Cause / fix |
|---|---|
| Configure fails downloading a dependency | No internet, or GitHub blocked. Re-run once connectivity is back; the CPM cache under `%USERPROFILE%\.cpm` resumes partial clones. |
| `LNK2038: mismatch detected for 'RuntimeLibrary'` | Your objects were compiled with `/MD` but the deps are `/MT`. Make sure you used the `windows-msvc-static` preset (it forces static CRT), not a bare configure. |
| `error: can't load cache ... __VERSION` | Stale configure cache. The dependencies changed shape after the CPM switch; delete the `build\windows-msvc` directory and rebuild. |
| Very long first build | Expected: protobuf and abseil are large. Only the first configure builds them; the CPM source cache avoids re-clone, and subsequent builds are incremental. |