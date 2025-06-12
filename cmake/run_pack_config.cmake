# Runs pack_config and captures its stdout into OUT.  Used instead of a shell
# redirection so the command is portable across generators.

execute_process(
  COMMAND "${PACK_CONFIG}" --config "${CONFIG}" --symbol "${SYMBOL}"
  RESULT_VARIABLE _result
  OUTPUT_VARIABLE _out
  ERROR_VARIABLE _err)
if(NOT _result EQUAL 0)
  message(FATAL_ERROR "pack_config failed: ${_err}")
endif()
file(WRITE "${OUT}" "${_out}")