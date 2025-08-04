get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
auto_create_library(${LIB_NAME} CXX_STD 20
    PUBLIC_DEPS utils raylib_cpp
)
