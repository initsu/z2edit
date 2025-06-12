# Generates version.h the same shape bazel's workspace-status awk produced:
#   #define BUILD_GIT_VERSION <git describe>
#   #define BUILD_SCM_REVISION <git rev-parse HEAD>
#   #define BUILD_SCM_STATUS Clean|Modified
# Each line is quoted and folded from multiple tokens (no-op for these keys,
# but kept for parity with the old awk).

find_package(Git QUIET)

set(_git "${GIT_EXECUTABLE}")
if(NOT _git)
  set(_git git)
endif()

execute_process(COMMAND ${_git} -C "${SRC}" describe --tags
  OUTPUT_VARIABLE _describe ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND ${_git} -C "${SRC}" rev-parse HEAD
  OUTPUT_VARIABLE _rev ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(_describe)
  set(BUILD_GIT_VERSION "${_describe}")
else()
  set(BUILD_GIT_VERSION "unknown")
endif()
set(BUILD_SCM_REVISION "${_rev}")

execute_process(COMMAND ${_git} -C "${SRC}" diff-index --quiet HEAD --
  RESULT_VARIABLE _dirty)
if(_dirty EQUAL 0)
  set(BUILD_SCM_STATUS "Clean")
else()
  set(BUILD_SCM_STATUS "Modified")
endif()

set(_out "#define BUILD_GIT_VERSION \"${BUILD_GIT_VERSION}\"\n")
set(_out "${_out}#define BUILD_SCM_REVISION \"${BUILD_SCM_REVISION}\"\n")
set(_out "${_out}#define BUILD_SCM_STATUS \"${BUILD_SCM_STATUS}\"\n")
file(WRITE "${OUT}" "${_out}")