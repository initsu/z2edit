# All third-party dependencies, fetched and built from source through
# CPM.cmake.  Every version here is pinned to the exact release the previous
# vcpkg baseline (builtin-baseline 04a9d8e5) installed, so behaviour stays
# identical while dropping vcpkg entirely.
#
# Nothing in this file should ever require a system package; the only system
# pieces left are compiler/platform libraries (Threads, OpenGL) and what the
# platform backends SDL/GTK talk to (X11/Wayland on Linux, GTK3 for
# nativefiledialog).

# Global: never build a third-party dep as a shared library.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

# MSVC: force the static CRT (/MT) for every dependency, like the old vcpkg
# x64-windows-static triplet did.  CMP0091 (CMAKE_MSVC_RUNTIME_LIBRARY) is
# ignored inside deps whose cmake_minimum_required predates 3.15 (SDL,
# gflags) and they would otherwise default to /MD, which can't link against
# the /MT objects we compile and would re-introduce the LNK2038 mess.
if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(APPEND CMAKE_C_FLAGS " /MTd")
    string(APPEND CMAKE_CXX_FLAGS " /MTd")
  else()
    string(APPEND CMAKE_C_FLAGS " /MT")
    string(APPEND CMAKE_CXX_FLAGS " /MT")
  endif()
endif()

# All the small fetch-and-assemble repos (imgui,
# nativefiledialog) have no CMakeLists.txt of their own; CPM populates their
# source dirs and we build the targets ourselves below.
# (Absolutely never discover a dep that later grows a CMakeLists.)

# --- silence third-party warnings ------------------------------------------
# Every dependency is compiled from source purely for z2edit's benefit, and its
# diagnostic noise (C4715/C4624/C4047 from protobuf and upb, C4723 from an
# absl random helper, etc.) is irrelevant here.  There is no configure-time way
# to enumerate subproject targets (the ALL_TARGETS properties are only filled
# at generation), so instead CMAKE_PROJECT_INCLUDE fires a stub *inside each
# dependency's own project() call*, with the directory context of that
# dependency.  The /w (MSVC) or -w (GCC/Clang) it registers is therefore a
# directory compile option inherited by the entire tree - e.g. protobuf's upb,
# json and compiler subdirectories - while z2edit's own targets, whose
# project() ran back at the top level, never see it.
#
# abseil and SDL3 are deliberately excluded from the blanket suppression:
# both raise their own warning level (/W3 on MSVC, -Wall on GCC/Clang) after
# this hook, which would both defeat the suppression and trip cl's D9025
# ("overriding '/w' with '/W3'") on every one of their files.  They stay at
# their normal level, and the single warning absl actually emits (a C4723) is
# silenced separately below.
set(_z2edit_project_warn_stub "${CMAKE_BINARY_DIR}/z2edit_dep_warn.cmake")
file(WRITE "${_z2edit_project_warn_stub}" [=[
if(NOT PROJECT_NAME STREQUAL "z2edit" AND NOT PROJECT_NAME STREQUAL ""
    AND NOT PROJECT_NAME STREQUAL "absl" AND NOT PROJECT_NAME STREQUAL "SDL3")
  if(MSVC)
    add_compile_options(/w)
  else()
    add_compile_options(-w)
  endif()
endif()
]=])
set(CMAKE_PROJECT_INCLUDE "${_z2edit_project_warn_stub}")
unset(_z2edit_project_warn_stub)

# --- abseil (used directly by z2edit and required by protobuf) ---------------
set(ABSL_PROPAGATE_CXX_STD ON CACHE BOOL "" FORCE)
# abseil overrides CMAKE_MSVC_RUNTIME_LIBRARY to the DLL variant unless this
# option is on; keep it aligned with our global static-CRT choice.
set(ABSL_MSVC_STATIC_RUNTIME ON CACHE BOOL "" FORCE)
CPMAddPackage(
  NAME absl
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp.git
  GIT_TAG 20260817.0
)
# absl raises its own warning level (/W3 on MSVC, -Wall on GCC/Clang) after the
# project() stub above, which is fine for the library as a whole - the only
# thing that actually fires is a C4723 inside this one always-built test-util
# file - but bury it there too.  A plain /w would just be deduplicated against
# the hook's and land before absl's /W3, so use /wd with the specific code:
# it is context-free and wins regardless of the warning level in force.
foreach(_z2edit_abslt IN ITEMS
    absl_random_internal_distribution_test_util
    absl_random_internal_distribution_test_util_nonprod)
  if(TARGET "${_z2edit_abslt}")
    if(MSVC)
      target_compile_options("${_z2edit_abslt}" PRIVATE /wd4723)
    else()
      target_compile_options("${_z2edit_abslt}" PRIVATE -w)
    endif()
  endif()
endforeach()
unset(_z2edit_abslt)

# --- gflags ------------------------------------------------------------------
set(gflags_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(gflags_BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(gflags_BUILD_gflags_LIB ON CACHE BOOL "" FORCE)
set(gflags_INSTALL OFF CACHE BOOL "" FORCE)
CPMAddPackage(
  NAME gflags
  GIT_REPOSITORY https://github.com/gflags/gflags.git
  GIT_TAG v2.3.1
)
if(TARGET gflags_static AND NOT TARGET gflags::gflags)
  add_library(gflags::gflags ALIAS gflags_static)
elseif(TARGET gflags AND NOT TARGET gflags::gflags)
  add_library(gflags::gflags ALIAS gflags)
endif()

# --- zlib --------------------------------------------------------------------
# zlib uses its own ZLIB_BUILD_SHARED/ZLIB_BUILD_STATIC options (defaults: both
# ON) and ignores BUILD_SHARED_LIBS; the shared variant (output name "z") would
# make z2edit.exe require z.dll at runtime. Use the static one only.
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
CPMAddPackage(
  NAME zlib
  GIT_REPOSITORY https://github.com/madler/zlib.git
  GIT_TAG v1.3.2
)
if(TARGET ZLIB::ZLIB)
  # provided by zlib itself
elseif(TARGET zlib)
  add_library(ZLIB::ZLIB ALIAS zlib)
elseif(TARGET zlibstatic)
  add_library(ZLIB::ZLIB ALIAS zlibstatic)
endif()

# --- SDL3 (static, in-tree; no prebuilt packages anywhere) --------------------
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)

# z2edit only plumbers SDL's video (window + OpenGL), audio
# (imwidget/imapp.cc:ImApp::InitAudio opens an unbound stream) and
# joystick/gamepad subsystems.  Everything else SDL3 drags in by default - the
# CPU/API renderers, GPU, camera, haptics, power, sensors, native file-dialog
# and tray support - is unused here, so it's switched off to shrink the static
# binary.  Platform render drivers (D3D/Metal/Vulkan/software) drop out with
# SDL_RENDER; DIALOG is dead weight because z2edit shells out to
# nativefiledialog.
set(SDL_AUDIO ON CACHE BOOL "" FORCE)
set(SDL_VIDEO ON CACHE BOOL "" FORCE)
set(SDL_JOYSTICK ON CACHE BOOL "" FORCE)
set(SDL_HIDAPI ON CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_GPU OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
set(SDL_TRAY OFF CACHE BOOL "" FORCE)
CPMAddPackage(
  NAME sdl3
  GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
  GIT_TAG release-3.4.16
)
if(NOT TARGET SDL3::SDL3)
  if(TARGET SDL3-static)
    add_library(SDL3::SDL3 ALIAS SDL3-static)
  elseif(TARGET SDL3)
    add_library(SDL3::SDL3 ALIAS SDL3)
  endif()
endif()
if(NOT TARGET SDL3::SDL3main AND TARGET SDL3main)
  add_library(SDL3::SDL3main ALIAS SDL3main)
endif()

# --- protobuf ----------------------------------------------------------------
set(protobuf_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_CONFORMANCE OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOBUF_BINARIES ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_PROTOC_BINARIES ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_LIBUPB ON CACHE BOOL "" FORCE)
set(protobuf_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(protobuf_WITH_ZLIB ON CACHE BOOL "" FORCE)
set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
# protobuf locates abseil through find_package(absl); abseil was added above
# as a subdirectory, so the config to find is the one in its build tree.
set(absl_DIR "${absl_BINARY_DIR}")
CPMAddPackage(
  NAME protobuf
  GIT_REPOSITORY https://github.com/protocolbuffers/protobuf.git
  GIT_TAG v36.1
)
if(NOT TARGET protobuf::libprotobuf)
  add_library(protobuf::libprotobuf ALIAS libprotobuf)
endif()
if(NOT TARGET protobuf::protoc)
  add_library(protobuf::protoc ALIAS protoc)
endif()

# --- imgui core --------------------------------------------------------------
CPMAddPackage(
  NAME imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.92.9b
  DOWNLOAD_ONLY YES
)
add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui PUBLIC Threads::Threads)

# The imgui repo ships a pile of example projects (with pre-built .vcxproj
# files), plus docs; none are used here.  Drop them so IDE folder views (e.g.
# Visual Studio) don't show a hundred example projects.  The backends/ dir is
# kept: util/CMakeLists.txt builds the SDL3+OpenGL3 backends straight from the
# fetched source, so no copy is carried in the repo.  (The SDL3 backend already
# includes each header as <SDL3/...>, matching SDL3's build-tree layout, so no
# in-place rewrite is needed.)
# Runs on every configure; if the source was already populated this prunes the
# stale copy in place.  Only prune the copy inside this build tree: with a CPM
# source cache the source dir may point straight at the shared cache (~/.cpm),
# which must stay pristine or future configures will surface a dirty worktree.
string(FIND "${imgui_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/" _z2edit_imgui_in_buildtree)
if(_z2edit_imgui_in_buildtree EQUAL 0)
  file(REMOVE_RECURSE
    "${imgui_SOURCE_DIR}/examples"
    "${imgui_SOURCE_DIR}/misc"
    "${imgui_SOURCE_DIR}/docs"
    "${imgui_SOURCE_DIR}/tools")
endif()

# --- nativefiledialog --------------------------------------------------------
CPMAddPackage(
  NAME nfd
  GIT_REPOSITORY https://github.com/mlabbe/nativefiledialog.git
  GIT_TAG release_116
  DOWNLOAD_ONLY YES
)

if(WIN32)
  # The upstream file includes <ShObjIdl.h>, which is fine on Windows' own
  # case-insensitive filesystem but breaks on case-sensitive drives ("linux"
  # style).  Materialise a fixed copy, like the Bazel genrule did.
  set(Z2EDIT_NFD_WIN "${CMAKE_BINARY_DIR}/nfd_win.cpp")
  file(READ ${nfd_SOURCE_DIR}/src/nfd_win.cpp Z2EDIT_NFD_WIN_SRC)
  string(REPLACE "ShObjIdl.h" "shobjidl.h" Z2EDIT_NFD_WIN_SRC "${Z2EDIT_NFD_WIN_SRC}")
  file(WRITE ${Z2EDIT_NFD_WIN} "${Z2EDIT_NFD_WIN_SRC}")

  add_library(nfd_impl STATIC
    ${nfd_SOURCE_DIR}/src/nfd_common.c
    ${Z2EDIT_NFD_WIN})
  target_compile_definitions(nfd_impl PRIVATE MINGW_HAS_SECURE_API=1)
  target_link_libraries(nfd_impl PUBLIC
    shell32 ole32 oleaut32 uuid comdlg32 shlwapi)
elseif(APPLE)
  add_library(nfd_impl STATIC
    ${nfd_SOURCE_DIR}/src/nfd_common.c
    ${nfd_SOURCE_DIR}/src/nfd_cocoa.m)
  target_link_libraries(nfd_impl PUBLIC "-framework Cocoa")
else()
  pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
  add_library(nfd_impl STATIC
    ${nfd_SOURCE_DIR}/src/nfd_common.c
    ${nfd_SOURCE_DIR}/src/nfd_gtk.c)
  target_link_libraries(nfd_impl PUBLIC PkgConfig::GTK3)
endif()
target_include_directories(nfd_impl PUBLIC
  ${nfd_SOURCE_DIR}/src ${nfd_SOURCE_DIR}/src/include)
set_target_properties(nfd_impl PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(nfd INTERFACE)
target_link_libraries(nfd INTERFACE nfd_impl)
target_include_directories(nfd INTERFACE
  ${nfd_SOURCE_DIR}/src ${nfd_SOURCE_DIR}/src/include)

add_compile_definitions(HAVE_NFD)

# All third-party trees have configured by now; drop the project() hook so any
# later project() in z2edit's own tree (none today) behaves normally.
unset(CMAKE_PROJECT_INCLUDE)