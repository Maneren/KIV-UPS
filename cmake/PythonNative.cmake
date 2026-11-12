include_guard(GLOBAL)

include(FetchContent)

execute_process(
    COMMAND "${UV}" python find
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/apps/hive_client"
    OUTPUT_VARIABLE HIVE_PYTHON
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE HIVE_PYTHON_RESULT
)

if(NOT HIVE_PYTHON_RESULT EQUAL 0)
    message(FATAL_ERROR "Could not resolve the hive_client Python interpreter")
endif()

set(Python_EXECUTABLE
    "${HIVE_PYTHON}"
    CACHE FILEPATH
    "Python interpreter used by the hive_client native module"
    FORCE
)
find_package(Python REQUIRED COMPONENTS Interpreter Development.Module)

FetchContent_Declare(
    nanobind
    GIT_REPOSITORY https://github.com/wjakob/nanobind
    GIT_TAG v3.1.0
    GIT_SHALLOW ON
    GIT_SUBMODULES "ext/robin_map"
    GIT_SUBMODULES_RECURSE TRUE
)
FetchContent_MakeAvailable(nanobind)
