add_python_tests(
    hive_client
    PROJECT_DIR "${CMAKE_SOURCE_DIR}/apps/hive_client"
    TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/hive_client"
)

add_dependencies(all_tests hive_client)
