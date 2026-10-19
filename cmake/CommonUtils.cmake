include_guard(GLOBAL)

include(${CMAKE_SOURCE_DIR}/cmake/CompilerFlags.cmake)

function(set_compiler_and_linker_flags TARGET)
    cmake_parse_arguments(ARGS "" "CXX_STD" "" ${ARGN})

    set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)

    if(ARGS_CXX_STD)
        set_target_properties(${TARGET} PROPERTIES CXX_STANDARD ${ARGS_CXX_STD})
    endif()

    target_compile_options(${TARGET} PRIVATE ${CXX_WARNING_FLAGS})
    if(WERROR)
        target_compile_options(${TARGET} PRIVATE ${CXX_WERROR_FLAGS})
    endif()

    # Debug information for debug-ish configs.
    set(USE_DEBUG_INFO $<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>)

    target_compile_options(
        ${TARGET}
        PRIVATE
            $<$<CONFIG:Debug>:${CXX_DEBUG_OPT_FLAGS}>
            $<${USE_DEBUG_INFO}>:${CXX_DEBUG_INFO_FLAGS}>
            ${CXX_HARDENING_FLAGS}
            $<$<CONFIG:Release>:${CXX_SECTION_FLAGS}>
    )

    # _FORTIFY_SOURCE only works with optimisation and fights ASan.
    if(FORTIFY_FLAGS AND NOT SANITIZERS)
        target_compile_options(
            ${TARGET}
            PRIVATE $<$<NOT:$<CONFIG:Debug>>:${FORTIFY_FLAGS}>
        )
    endif()

    target_link_options(
        ${TARGET}
        PRIVATE
            ${CXX_LINK_HARDENING_FLAGS}
            $<$<CONFIG:Release>:${CXX_LINK_GC_FLAGS}>
            $<$<CONFIG:Release>:${CXX_LINK_ICF_FLAGS}>
    )

    if(USE_PCH)
        # Shared PCH: heavy + frequently included std headers
        target_precompile_headers(
            ${TARGET}
            PRIVATE
                <expected>
                <format>
                <functional>
                <optional>
                <print>
                <span>
                <string>
                <string_view>
                <unordered_map>
                <unordered_set>
                <utility>
                <variant>
                <vector>
        )
    endif()

    get_property(THIN_ARCHIVE GLOBAL PROPERTY THIN_ARCHIVE)
    # --thin is a GNU/LLVM ar extension
    if(THIN_ARCHIVE AND NOT MSVC AND NOT APPLE)
        set_property(
            TARGET ${TARGET}
            APPEND
            PROPERTY STATIC_LIBRARY_OPTIONS "--thin"
        )
    endif()
endfunction()

function(add_include_directories TARGET VISIBILITY DIRS)
    if(DIRS)
        target_include_directories(${TARGET} ${VISIBILITY} ${DIRS})
    endif()
endfunction()

function(set_output_directories TARGET TYPE)
    if(TYPE STREQUAL "LIBRARY")
        set_target_properties(
            ${TARGET}
            PROPERTIES
                ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    elseif(TYPE STREQUAL "EXECUTABLE")
        set_target_properties(
            ${TARGET}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )
    elseif(TYPE STREQUAL "TEST")
        set_target_properties(
            ${TARGET}
            PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/bin
                FOLDER "Tests"
        )
    endif()
endfunction()

function(set_version_properties TARGET VERSION)
    if(VERSION)
        # SOVERSION is the ABI major; VERSION is the full release string.
        string(REGEX MATCH "^[0-9]+" SOVERSION "${VERSION}")
        set_target_properties(
            ${TARGET}
            PROPERTIES VERSION ${VERSION} SOVERSION ${SOVERSION}
        )
    endif()
endfunction()
