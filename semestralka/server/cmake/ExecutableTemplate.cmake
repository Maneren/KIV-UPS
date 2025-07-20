include(${CMAKE_CURRENT_LIST_DIR}/CommonUtils.cmake)

# Function to create a standardized executable with common settings
function(create_executable TARGET_NAME SOURCES)
    # Parse optional arguments
    cmake_parse_arguments(EXE
        "CONSOLE;GUI;TEST"
        "VERSION;OUTPUT_NAME"
        "PRIVATE_DEPS;COMPILE_DEFS;INCLUDE_DIRS"
        ${ARGN}
    )

    # Create executable
    if(SOURCES)
        add_executable(${TARGET_NAME} ${SOURCES})
    else()
        # Auto-detect main source file
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
            add_executable(${TARGET_NAME} main.cpp)
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}.cpp")
            add_executable(${TARGET_NAME} ${TARGET_NAME}.cpp)
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
            add_executable(${TARGET_NAME} src/main.cpp)
        else()
            message(FATAL_ERROR "No source files specified and no main.cpp found for ${TARGET_NAME}")
        endif()
    endif()

    # Set include directories
    add_include_directories_if_provided(${TARGET_NAME} PRIVATE "${EXE_INCLUDE_DIRS}")

    # Set common compile options and sanitizers
    set_common_compile_options(${TARGET_NAME} PRIVATE)
    set_sanitizer_options(${TARGET_NAME})

    # Set C++ standard
    set_cxx_standard(${TARGET_NAME} PRIVATE)

    # Add compile definitions
    add_compile_definitions_with_visibility(${TARGET_NAME} PRIVATE "${EXE_COMPILE_DEFS}")

    # Link dependencies
    if(EXE_PRIVATE_DEPS)
        target_link_libraries(${TARGET_NAME} PRIVATE ${EXE_PRIVATE_DEPS})
    endif()

    # Set executable properties
    set_output_directories(${TARGET_NAME} EXECUTABLE)

    # Set custom output name if provided
    if(EXE_OUTPUT_NAME)
        set_target_properties(${TARGET_NAME} PROPERTIES
            OUTPUT_NAME ${EXE_OUTPUT_NAME}
        )
    endif()

    # Set version if provided
    set_version_properties(${TARGET_NAME} "${EXE_VERSION}")

    # Set subsystem for Windows
    if(WIN32)
        if(EXE_CONSOLE)
            set_target_properties(${TARGET_NAME} PROPERTIES
                WIN32_EXECUTABLE FALSE
            )
        elseif(EXE_GUI)
            set_target_properties(${TARGET_NAME} PROPERTIES
                WIN32_EXECUTABLE TRUE
            )
        endif()
    endif()
endfunction()

# Helper function to automatically create executable
function(auto_create_executable TARGET_NAME)
    get_filename_component(DIR_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

    # Auto-collect sources
    file(GLOB_RECURSE SOURCES
        "src/*.cpp" "src/*.c" "src/*.cxx" "src/*.cc"
        "src/*.h" "src/*.hpp" "src/*.hxx"
    )

    create_executable(${TARGET_NAME} "${SOURCES}" ${ARGN})
endfunction()
