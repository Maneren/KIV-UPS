include(${CMAKE_CURRENT_LIST_DIR}/CommonUtils.cmake)

# Function to create a standardized library with common settings
function(create_library TARGET_NAME SOURCES HEADERS)
    # Parse optional arguments
    cmake_parse_arguments(LIB
        "STATIC;SHARED;HEADER_ONLY"
        "VERSION;NAMESPACE"
        "PRIVATE_DEPS;PUBLIC_DEPS;INTERFACE_DEPS;COMPILE_DEFS;INCLUDE_DIRS"
        ${ARGN}
    )

    # Determine library type
    if(LIB_HEADER_ONLY)
        add_library(${TARGET_NAME} INTERFACE)
        set(VISIBILITY INTERFACE)
    elseif(LIB_STATIC)
        add_library(${TARGET_NAME} STATIC ${SOURCES})
        set(VISIBILITY PUBLIC)
    elseif(LIB_SHARED)
        add_library(${TARGET_NAME} SHARED ${SOURCES})
        set(VISIBILITY PUBLIC)
    else()
        # Default to static
        add_library(${TARGET_NAME} STATIC ${SOURCES})
        set(VISIBILITY PUBLIC)
    endif()

    # Set include directories
    if(NOT LIB_HEADER_ONLY)
        target_include_directories(${TARGET_NAME}
            ${VISIBILITY}
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
            PRIVATE
                ${CMAKE_CURRENT_SOURCE_DIR}/src
        )
    else()
        target_include_directories(${TARGET_NAME}
            INTERFACE
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:include>
        )
    endif()

    # Add custom include directories
    add_include_directories_if_provided(${TARGET_NAME} PRIVATE "${LIB_INCLUDE_DIRS}")

    # Set common compile options and sanitizers (only for non-header-only)
    if(NOT LIB_HEADER_ONLY)
        set_common_compile_options(${TARGET_NAME} PRIVATE)
        set_sanitizer_options(${TARGET_NAME})
    endif()

    # Set C++ standard
    set_cxx_standard(${TARGET_NAME} ${VISIBILITY})

    # Add compile definitions
    add_compile_definitions_with_visibility(${TARGET_NAME}
        $<IF:${LIB_HEADER_ONLY},INTERFACE,PRIVATE>
        "${LIB_COMPILE_DEFS}")

    # Link dependencies
    link_dependencies(${TARGET_NAME}
        "${LIB_PRIVATE_DEPS}"
        "${LIB_PUBLIC_DEPS}"
        "${LIB_INTERFACE_DEPS}"
        ${LIB_HEADER_ONLY})

    # Set library properties
    if(NOT LIB_HEADER_ONLY)
        set_target_properties(${TARGET_NAME} PROPERTIES
            OUTPUT_NAME ${TARGET_NAME}
        )
        set_output_directories(${TARGET_NAME} LIBRARY)
        set_version_properties(${TARGET_NAME} "${LIB_VERSION}")
    endif()

    # Create alias with namespace if provided
    if(LIB_NAMESPACE)
        add_library(${LIB_NAMESPACE}::${TARGET_NAME} ALIAS ${TARGET_NAME})
    endif()

    # Enable position independent code for shared libraries
    if(LIB_SHARED)
        set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    endif()
endfunction()

# Helper function to automatically collect sources
function(auto_create_library TARGET_NAME)
    cmake_parse_arguments(AUTO "" "" "EXCLUDE_PATTERNS" ${ARGN})

    # Auto-collect sources
    file(GLOB_RECURSE SOURCES
        "src/*.cpp" "src/*.c" "src/*.cxx" "src/*.cc"
    )

    # Auto-collect headers
    file(GLOB_RECURSE HEADERS
        "include/*.h" "include/*.hpp" "include/*.hxx"
    )

    # Remove excluded patterns
    if(AUTO_EXCLUDE_PATTERNS)
        foreach(pattern ${AUTO_EXCLUDE_PATTERNS})
            list(FILTER SOURCES EXCLUDE REGEX ${pattern})
            list(FILTER HEADERS EXCLUDE REGEX ${pattern})
        endforeach()
    endif()

    # Check if it's header-only
    if(NOT SOURCES AND HEADERS)
        create_library(${TARGET_NAME} "" "${HEADERS}" HEADER_ONLY ${ARGN})
    else()
        create_library(${TARGET_NAME} "${SOURCES}" "${HEADERS}" ${ARGN})
    endif()
endfunction()
