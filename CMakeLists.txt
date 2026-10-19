cmake_minimum_required(VERSION 3.31...4.4)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(PROJECT "kiv-ups")
project(${PROJECT} LANGUAGES CXX)

set(CMAKE_INSTALL_PREFIX ${PROJECT_SOURCE_DIR})
set(CMAKE_INSTALL_MESSAGE LAZY)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Feature toggles
option(SANITIZERS "Enable Address/UB/Leak sanitizers" OFF)
option(WERROR "Treat compiler warnings as errors" OFF)
option(THIN_ARCHIVE "Create thin static archives" ON)
option(USE_IPO "Enable interprocedural optimisation for optimised configs" ON)
option(USE_CCACHE "Use ccache/sccache as a compiler launcher" ON)
option(USE_PCH "Use precompiled headers to speed up compilation" ON)
option(BUILD_TESTS "Build test executables" ON)

# Toolchain helpers
include(cmake/CommonUtils.cmake)

# Target templates
include(cmake/ExecutableTemplate.cmake)
include(cmake/LibraryTemplate.cmake)
include(cmake/PythonAppTemplate.cmake)

# Libraries
add_subdirectory(src)
# Apps
add_subdirectory(apps)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
