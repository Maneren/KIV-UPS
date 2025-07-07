cmake_minimum_required(VERSION 3.10)
set(LIBRARY hive)
project(${LIBRARY} LANGUAGES CXX)

file(GLOB_RECURSE SOURCE_FILES *.cpp *.h)

add_library(${LIBRARY} STATIC ${SOURCE_FILES})

install(TARGETS ${LIBRARY} DESTINATION ${PROJECT_LIB_DIR})

target_compile_options(
  ${LIBRARY}
  PUBLIC
  "$<$<CONFIG:DEBUG,RELWITHDEBINFO>:SHELL:${PROJECT_CXX_FLAGS_DEBUGINFO}>"
)
target_compile_options(
  ${LIBRARY}
  PRIVATE
  "$<$<CONFIG:DEBUG,RELWITHDEBINFO>:SHELL:${PROJECT_CXX_FLAGS_WARNINGS}>"
)

target_link_libraries(${LIBRARY} PUBLIC expected)

target_include_directories(${LIBRARY} PUBLIC ../include/)
