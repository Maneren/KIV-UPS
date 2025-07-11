# Common utilities for CMake templates
# This file contains shared functions used across library, executable, and test templates

# Function to set common compile options for a target
function(set_common_compile_options TARGET_NAME VISIBILITY)
    target_compile_options(${TARGET_NAME} ${VISIBILITY}
        # Warning flags
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>

        # Debug flags
        $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:GNU,Clang>:-g3 -gdwarf-5 -fno-omit-frame-pointer>>
        $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:MSVC>:/Od /Zi>>

        # Release flags
        $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -DNDEBUG>>
        $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:MSVC>:/O2 /DNDEBUG>>
    )
endfunction()

# Function to set enhanced compile options for tests
function(set_test_compile_options TARGET_NAME)
    target_compile_options(${TARGET_NAME} PRIVATE
        # Warning flags (more aggressive for tests)
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wconversion>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>

        # Debug flags (tests usually run in debug)
        $<$<CXX_COMPILER_ID:GNU,Clang>:-g3 -O0>
        $<$<CXX_COMPILER_ID:MSVC>:/Od /Zi>

        # Sanitizers (always enabled for tests)
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=address,undefined,leak>
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fno-omit-frame-pointer>
    )
endfunction()

# Function to set sanitizer options (debug builds only for non-test targets)
function(set_sanitizer_options TARGET_NAME)
    target_compile_options(${TARGET_NAME} PRIVATE
        $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU,Clang>>:-fsanitize=address,undefined,leak>
    )

    target_link_options(${TARGET_NAME} PRIVATE
        $<$<AND:$<CONFIG:Debug>,$<CXX_COMPILER_ID:GNU,Clang>>:-fsanitize=address,undefined,leak>
    )
endfunction()

# Function to set sanitizer options for tests (always enabled)
function(set_test_sanitizer_options TARGET_NAME)
    target_link_options(${TARGET_NAME} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-fsanitize=address,undefined,leak>
    )
endfunction()

# Function to set C++ standard based on target type
function(set_cxx_standard TARGET_NAME VISIBILITY)
    target_compile_features(${TARGET_NAME} ${VISIBILITY} cxx_std_20)
endfunction()

# Function to handle compile definitions
function(add_compile_definitions_with_visibility TARGET_NAME VISIBILITY DEFINITIONS)
    if(DEFINITIONS)
        target_compile_definitions(${TARGET_NAME} ${VISIBILITY} ${DEFINITIONS})
    endif()
endfunction()

# Function to handle include directories
function(add_include_directories_if_provided TARGET_NAME VISIBILITY DIRECTORIES)
    if(DIRECTORIES)
        target_include_directories(${TARGET_NAME} ${VISIBILITY} ${DIRECTORIES})
    endif()
endfunction()

# Function to handle library dependencies
function(link_dependencies TARGET_NAME PRIVATE_DEPS PUBLIC_DEPS INTERFACE_DEPS IS_HEADER_ONLY)
    if(PRIVATE_DEPS AND NOT IS_HEADER_ONLY)
        target_link_libraries(${TARGET_NAME} PRIVATE ${PRIVATE_DEPS})
    endif()

    if(PUBLIC_DEPS)
        if(IS_HEADER_ONLY)
            target_link_libraries(${TARGET_NAME} INTERFACE ${PUBLIC_DEPS})
        else()
            target_link_libraries(${TARGET_NAME} PUBLIC ${PUBLIC_DEPS})
        endif()
    endif()

    if(INTERFACE_DEPS)
        target_link_libraries(${TARGET_NAME} INTERFACE ${INTERFACE_DEPS})
    endif()
endfunction()

# Function to set common output directories
function(set_output_directories TARGET_NAME TYPE)
    if(TYPE STREQUAL "LIBRARY")
        set_target_properties(${TARGET_NAME} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    elseif(TYPE STREQUAL "EXECUTABLE")
        set_target_properties(${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    elseif(TYPE STREQUAL "TEST")
        set_target_properties(${TARGET_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/bin
            FOLDER "Tests"
        )
    endif()
endfunction()

# Function to set version properties
function(set_version_properties TARGET_NAME VERSION)
    if(VERSION)
        set_target_properties(${TARGET_NAME} PROPERTIES
            VERSION ${VERSION}
            SOVERSION ${VERSION}
        )
    endif()
endfunction()
