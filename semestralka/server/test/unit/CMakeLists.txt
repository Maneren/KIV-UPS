create_test_executable(net_tests
    SOURCES net/address_tests.cpp net/stream_tests.cpp
    PRIVATE_DEPS net
    GTEST
)

add_dependencies(all_tests net_tests)
