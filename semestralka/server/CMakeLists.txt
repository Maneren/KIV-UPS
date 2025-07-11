cmake_minimum_required(VERSION 3.15)

set(PROJECT "hive-server")
project(${PROJECT})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
set(CMAKE_INSTALL_MESSAGE LAZY)

add_subdirectory(src)
add_subdirectory(apps)

option(BUILD_TESTS "Build test executables" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
