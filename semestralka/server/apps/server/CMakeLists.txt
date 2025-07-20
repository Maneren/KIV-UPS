include(${CMAKE_SOURCE_DIR}/cmake/ExecutableTemplate.cmake)

auto_create_executable(server
    PRIVATE_DEPS hive net threadpool utils
    CONSOLE
    OUTPUT_NAME "hive_server"
    VERSION 1.0.0
)
