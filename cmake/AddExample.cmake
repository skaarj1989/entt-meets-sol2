function(ADD_EXAMPLE)
  cmake_parse_arguments(PARSE_ARGV 0 ARGS "" "TARGET" "SOURCES")

  add_executable(${ARGS_TARGET} ${ARGS_SOURCES})
  target_link_libraries(${ARGS_TARGET} PUBLIC EnTT::EnTT sol2 Lua::Lua MetaHelper)
  add_dependencies(${ARGS_TARGET} CopyScripts)

  set_property(
    TARGET ${ARGS_TARGET}
    PROPERTY VS_DEBUGGER_WORKING_DIRECTORY
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
  set_property(TARGET ${ARGS_TARGET} PROPERTY FOLDER "Examples")
endfunction()
