create_test_executable(net_tests
    SOURCES net/address_tests.cpp net/stream_tests.cpp
    PRIVATE_DEPS net
    GTEST
)

create_test_executable(utils_tests
    SOURCES utils/serde_tests.cpp
    PRIVATE_DEPS utils
    GTEST
)

add_dependencies(all_tests net_tests utils_tests)
