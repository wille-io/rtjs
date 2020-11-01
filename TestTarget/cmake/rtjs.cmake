macro(RtjsTarget target)
  set(cppast_target ${target})

  get_target_property(cppast_sources ${cppast_target} SOURCES)
  #message(STATUS "SOURCES for ${cppast_target} = ${cppast_sources}")

  set(output "${CMAKE_CURRENT_BINARY_DIR}/rtjs_${cppast_target}.cpp")

  add_custom_command(
    OUTPUT "rtjs_${cppast_target}.cpp"
    COMMAND rtjsgen ${cppast_sources} "-I" $<TARGET_PROPERTY:${cppast_target},INCLUDE_DIRECTORIES> "-D" $<TARGET_PROPERTY:${cppast_target},COMPILE_DEFINITIONS> "-O" ${output}
    #COMMAND "echo" ${cppast_sources} "-I" $<TARGET_PROPERTY:${cppast_target},INCLUDE_DIRECTORIES> "-D" $<TARGET_PROPERTY:${cppast_target},COMPILE_DEFINITIONS> "-O" ${output}
    #COMMAND ...  "-I$<JOIN:$<TARGET_PROPERTY:${cppast_target},INCLUDE_DIRECTORIES>, -I>"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    #DEPENDS rtjsgen
    DEPENDS ${cppast_sources}
    VERBATIM
  )

  add_custom_target(rtjs_${cppast_target} ALL
    DEPENDS "rtjs_${cppast_target}.cpp"
  )

  target_sources(${cppast_target} PRIVATE ${output})
  target_compile_definitions(${cppast_target} PRIVATE RTJS_INIT=__rtjs_init_${cppast_target})
  add_dependencies(${cppast_target} rtjs_${cppast_target})
endmacro()
