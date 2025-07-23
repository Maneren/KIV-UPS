function(set_compiler_and_linker_flags TARGET TEST)
    target_compile_options(${TARGET} PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
    )

    set(CXX_GNU_DEBUG_INFO -g3 -gdwarf-5 -Og -fno-omit-frame-pointer)
    set(CXX_SANITIZERS -fsanitize=address,undefined,leak)
    if(TEST)
        target_compile_options(${TARGET} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:-Wconversion ${CXX_GNU_DEBUG_INFO}>
            $<$<CXX_COMPILER_ID:MSVC>:/W4 /Od /Zi>
        )
        target_link_options(${TARGET} PRIVATE
            $<$<CXX_COMPILER_ID:GNU,Clang>:${CXX_SANITIZERS}>
        )
    else()
        target_compile_options(${TARGET} PRIVATE
            $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:GNU,Clang>:${CXX_GNU_DEBUG_INFO}>>
            $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:GNU,Clang>:${CXX_SANITIZERS}>>
            $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:MSVC>:/Od /Zi>>
            $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:GNU,Clang>:-O3 -DNDEBUG>>
            $<$<CONFIG:Release>:$<$<CXX_COMPILER_ID:MSVC>:/O2 /DNDEBUG>>
        )
        target_link_options(${TARGET} PRIVATE
            $<$<CONFIG:Debug>:$<$<CXX_COMPILER_ID:GNU,Clang>:${CXX_SANITIZERS}>>
        )
    endif()
endfunction()

function(link_dependencies TARGET VISIBILITY DEPENDENCIES)
    if(DEPENDENCIES)
        target_link_libraries(${TARGET} ${VISIBILITY} ${DEPENDENCIES})
    endif()
endfunction()

function(add_include_directories TARGET VISIBILITY DIRS)
    if(DIRS)
        target_include_directories(${TARGET} ${VISIBILITY} ${DIRS})
    endif()
endfunction()

function(set_output_directories TARGET TYPE)
    if(TYPE STREQUAL "LIBRARY")
        set_target_properties(${TARGET} PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    elseif(TYPE STREQUAL "EXECUTABLE")
        set_target_properties(${TARGET} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    elseif(TYPE STREQUAL "TEST")
        set_target_properties(${TARGET} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/test/bin
            FOLDER "Tests")
    endif()
endfunction()

function(set_version_properties TARGET VERSION)
    set_target_properties(${TARGET} PROPERTIES
        VERSION ${VERSION}
        SOVERSION ${VERSION})
endfunction()
