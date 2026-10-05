create_test_executable(
    hive_tests
    SOURCES hive/message_tests.cpp
    DEPENDS hive
    CXX_STD 20
)

create_test_executable(
    net_tests
    SOURCES net/address_tests.cpp net/stream_tests.cpp
    DEPENDS net
    CXX_STD 20
)

create_test_executable(
    threadpool_tests
    SOURCES threadpool/threadpool_tests.cpp
    DEPENDS threadpool utils
    CXX_STD 23
)

create_test_executable(
    utils_tests
    SOURCES utils/serde_tests.cpp
    DEPENDS utils
    CXX_STD 20
)

add_dependencies(all_tests hive_tests net_tests threadpool_tests utils_tests)
