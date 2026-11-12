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

nanobind_add_stub(
    hive_native_stub
    MODULE hive_native
    OUTPUT "${_output_dir}/hive_native.pyi"
    PYTHON_PATH "${_output_dir}"
    DEPENDS hive_native
)
