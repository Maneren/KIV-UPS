include(${CMAKE_CURRENT_LIST_DIR}/CommonUtils.cmake)

# Function to create a standardized test executable
function(create_test_executable TARGET_NAME)
    # Parse optional arguments
    cmake_parse_arguments(TEST
        "GTEST;CATCH2;CUSTOM;EXCLUDE_FROM_ALL"
        "FRAMEWORK;WORKING_DIRECTORY"
        "SOURCES;PRIVATE_DEPS;COMPILE_DEFS;INCLUDE_DIRS"
        ${ARGN}
    )

    # Auto-detect sources if not provided
    if(NOT TEST_SOURCES)
        file(GLOB_RECURSE TEST_SOURCES "*.cpp" "*.c")
        if(NOT TEST_SOURCES)
            message(FATAL_ERROR "No test sources found for ${TARGET_NAME}")
        endif()
    endif()

    # Create test executable
    if(TEST_EXCLUDE_FROM_ALL)
        add_executable(${TARGET_NAME} EXCLUDE_FROM_ALL ${TEST_SOURCES})
    else()
        add_executable(${TARGET_NAME} ${TEST_SOURCES})
    endif()

    # Set include directories
    target_include_directories(${TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/src
    )
    add_include_directories_if_provided(${TARGET_NAME} PRIVATE "${TEST_INCLUDE_DIRS}")

    # Set test-specific compile options and sanitizers
    set_test_compile_options(${TARGET_NAME})
    set_test_sanitizer_options(${TARGET_NAME})

    # Set C++ standard
    set_cxx_standard(${TARGET_NAME} PRIVATE)

    # Add compile definitions
    target_compile_definitions(${TARGET_NAME} PRIVATE
        TESTING_ENABLED=1
        $<$<CONFIG:Debug>:DEBUG_TESTS=1>
    )
    add_compile_definitions_with_visibility(${TARGET_NAME} PRIVATE "${TEST_COMPILE_DEFS}")

    # Link project dependencies
    if(TEST_PRIVATE_DEPS)
        target_link_libraries(${TARGET_NAME} PRIVATE ${TEST_PRIVATE_DEPS})
    endif()

    # Handle test frameworks
    if(TEST_GTEST OR TEST_FRAMEWORK STREQUAL "gtest")
        setup_gtest(${TARGET_NAME})
    elseif(TEST_CATCH2 OR TEST_FRAMEWORK STREQUAL "catch2")
        setup_catch2(${TARGET_NAME})
    endif()

    # Set test properties
    set_output_directories(${TARGET_NAME} TEST)
endfunction()

# Helper function to setup Google Test
function(setup_gtest TARGET_NAME)
    find_package(GTest QUIET)

    if(TARGET GTest::GTest)
        target_link_libraries(${TARGET_NAME} PRIVATE
            GTest::GTest
            GTest::Main
        )
    else()
        # Try to find local googletest
        if(EXISTS ${CMAKE_SOURCE_DIR}/test/external/googletest/CMakeLists.txt)
            add_subdirectory(${CMAKE_SOURCE_DIR}/test/external/googletest EXCLUDE_FROM_ALL)
            target_link_libraries(${TARGET_NAME} PRIVATE gtest gtest_main)
        else()
            message(FATAL_ERROR "GoogleTest not found. Install it or run 'git submodule update --init --recursive'")
        endif()
    endif()

    # Enable GTest discovery
    include(GoogleTest)
    gtest_discover_tests(${TARGET_NAME})
endfunction()

# Helper function to setup Catch2
function(setup_catch2 TARGET_NAME)
    find_package(Catch2 QUIET)

    if(TARGET Catch2::Catch2)
        target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2WithMain)
    else()
        # Try to find local catch2
        if(EXISTS ${CMAKE_SOURCE_DIR}/test/external/catch2/CMakeLists.txt)
            add_subdirectory(${CMAKE_SOURCE_DIR}/test/external/catch2 EXCLUDE_FROM_ALL)
            target_link_libraries(${TARGET_NAME} PRIVATE Catch2::Catch2WithMain)
        else()
            message(FATAL_ERROR "Catch2 not found. Install it or add as submodule")
        endif()
    endif()

    # Enable Catch2 discovery
    include(Catch)
    catch_discover_tests(${TARGET_NAME})
endfunction()
