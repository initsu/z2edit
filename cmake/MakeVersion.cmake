find_package(Git QUIET)

set(_git "${GIT_EXECUTABLE}")
if(NOT _git)
  set(_git git)
endif()

execute_process(COMMAND "${_git}" -C "${SRC}" describe --tags --always
  OUTPUT_VARIABLE _describe ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(_describe)
  set(BUILD_GIT_VERSION "${_describe}")
else()
  set(BUILD_GIT_VERSION "unknown")
endif()

# Unlike diff-index, status refreshes stale index stat information before comparing.
execute_process(COMMAND "${_git}" --no-optional-locks -C "${SRC}"
  status --porcelain=v1 --untracked-files=no
  RESULT_VARIABLE _status_result OUTPUT_VARIABLE _changes
  ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
if(_status_result STREQUAL "0" AND NOT _changes STREQUAL "")
  string(APPEND BUILD_GIT_VERSION "-Modified")
endif()

# Only update the timestamp when the version changes.
file(CONFIGURE OUTPUT "${OUT}"
  CONTENT "#define BUILD_GIT_VERSION \"@BUILD_GIT_VERSION@\"\n" @ONLY)
