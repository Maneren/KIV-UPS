get_filename_component(APP_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

add_subdirectory(native)
create_python_app(${APP_NAME})
add_dependencies(${APP_NAME} hive_native)
