include(${CMAKE_CURRENT_LIST_DIR}/CommonUtils.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/ExecutableTemplate.cmake)
include(GoogleTest)

function(create_test_executable TARGET)
    cmake_parse_arguments(
        TEST
        ""
        "CXX_STD"
        "SOURCES;DEPENDS;COMPILE_DEFS;INCLUDE_DIRS"
        ${ARGN}
    )

    if(NOT TEST_SOURCES)
        file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS *.cpp *.c)
        if(NOT TEST_SOURCES)
            message(FATAL_ERROR "No test sources for ${TARGET}")
        endif()
    endif()

    # gmock ships with googletest, but is a distinct target (and a separate
    # package on some distros), so only link it when it is actually available.
    set(TEST_FRAMEWORK_DEPS GTest::gtest GTest::gtest_main)
    if(TARGET GTest::gmock)
        list(APPEND TEST_FRAMEWORK_DEPS GTest::gmock)
    endif()

    create_executable(
        ${TARGET}
        "${TEST_SOURCES}"
        TEST
        CXX_STD ${TEST_CXX_STD}
        DEPENDS ${TEST_FRAMEWORK_DEPS} ${TEST_DEPENDS}
        COMPILE_DEFS
            ${TEST_COMPILE_DEFS}
            TESTING_ENABLED=1
            $<$<CONFIG:Debug>:DEBUG_TESTS=1>
        INCLUDE_DIRS ${TEST_INCLUDE_DIRS}
    )

    if(USE_PCH)
        # gtest is the single most expensive header in test TUs (~1.4s parse)
        target_precompile_headers(${TARGET} PRIVATE <gtest/gtest.h>)
    endif()

    gtest_discover_tests(${TARGET} PROPERTIES LABELS "gtest")
endfunction()
