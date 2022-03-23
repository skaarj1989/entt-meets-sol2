function(ADD_SCRIPTS)
  cmake_parse_arguments(PARSE_ARGV 0 ARGS "" "TARGET;DIR" "")

  file(GLOB_RECURSE SCRIPT_FILES "${ARGS_DIR}/*.*")

  add_custom_target(${ARGS_TARGET} ALL SOURCES ${SCRIPT_FILES})

  set(OUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/lua")
  foreach(FILE ${SCRIPT_FILES})
    file(RELATIVE_PATH FILENAME "${ARGS_DIR}" "${FILE}")
    set(OUT_FILE "${OUT_DIR}/${FILENAME}")

    add_custom_command(
      OUTPUT ${OUT_FILE}
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${FILE} ${OUT_FILE}
      DEPENDS ${FILE}
      COMMENT "Copying script: ${FILENAME}")

    list(APPEND OUT_FILES ${OUT_FILE})
  endforeach()

  add_custom_target(CopyScripts DEPENDS ${OUT_FILES})
endfunction()
