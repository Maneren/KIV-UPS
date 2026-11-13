nanobind_add_module(hive_native NB_SUPPRESS_WARNINGS bindings.cpp)

target_link_libraries(hive_native PRIVATE hive)
target_compile_features(hive_native PRIVATE cxx_std_23)
target_compile_options(hive_native PRIVATE ${CXX_WARNING_FLAGS})

# nanobind's headers expand casts and Python macros in this translation unit.
if(NOT MSVC)
    target_compile_options(
        hive_native
        PRIVATE -Wno-old-style-cast -Wno-macro-redefined -Wno-cast-qual
    )
endif()

set(_output_dir "${CMAKE_BINARY_DIR}/python/$<CONFIG>")
set_target_properties(
    hive_native
    PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${_output_dir}"
        RUNTIME_OUTPUT_DIRECTORY "${_output_dir}"
)

add_custom_target(
    hive_native_stub
    ALL
    COMMAND
        "${CMAKE_COMMAND}" -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/hive_native.pyi"
        "${_output_dir}/hive_native.pyi"
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/hive_native.pyi"
    COMMENT "Copying hive_native type stub"
    VERBATIM
)
add_dependencies(hive_native_stub hive_native)
