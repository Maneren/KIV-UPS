cmake_minimum_required(VERSION 3.10)
set(EXECUTABLE hive_server)
project(${EXECUTABLE} LANGUAGES CXX)

file(GLOB SOURCE_FILES *.cpp)
add_executable(${EXECUTABLE} ${SOURCE_FILES})
install(TARGETS ${EXECUTABLE} DESTINATION ${PROJECT_BIN_DIR})

target_compile_options(
  ${EXECUTABLE}
  PRIVATE
  "$<$<NOT:$<CONFIG:RELEASE>>:-fsanitize=address>"
  "$<$<CONFIG:DEBUG,RELWITHDEBINFO>:SHELL:${PROJECT_CXX_FLAGS_DEBUGINFO}>"
  "$<$<CONFIG:DEBUG,RELWITHDEBINFO>:SHELL:${PROJECT_CXX_FLAGS_WARNINGS}>"
)
target_link_options(
  ${EXECUTABLE}
  PRIVATE
  "$<$<NOT:$<CONFIG:RELEASE>>:-fsanitize=address>"
)

file(GLOB DIRS LIST_DIRECTORIES TRUE ./*)
foreach(DIR ${DIRS})
  if(EXISTS ${DIR}/CMakeLists.txt)
    add_subdirectory(${DIR})
    get_filename_component(lib_name ${DIR} NAME)
    target_link_libraries(${EXECUTABLE} PRIVATE ${lib_name})
  endif()
endforeach()

set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE OFF)

file(GLOB EXTERNAL LIST_DIRECTORIES TRUE ./external/*)
foreach(DIR ${EXTERNAL})
  if(EXISTS ${DIR}/CMakeLists.txt)
    add_subdirectory(${DIR})
    if (EXISTS ${DIR}/include)
      include_directories(${DIR}/include)
    endif()
  endif()
endforeach()

file(GLOB INCLUDE_DIRS LIST_DIRECTORIES TRUE include/*)
install(DIRECTORY ${INCLUDE_DIRS} DESTINATION ${PROJECT_INCLUDE_DIR})

include_directories(include)
