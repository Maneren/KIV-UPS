get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
auto_create_library(${LIB_NAME}
    PUBLIC_DEPS utils expected
)
