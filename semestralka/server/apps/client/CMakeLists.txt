include(${CMAKE_SOURCE_DIR}/cmake/ExecutableTemplate.cmake)

auto_create_executable(client
    PRIVATE_DEPS hive net raylib raylib_cpp raygui utils
    CONSOLE
    OUTPUT_NAME "hive_client"
    VERSION 1.0.0
)
