get_filename_component(APP_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)
auto_create_executable(
    ${APP_NAME}
    DEPENDS net threadpool utils
    OUTPUT_NAME "${APP_NAME}"
    VERSION 1.0.0
    CXX_STD 23
)
