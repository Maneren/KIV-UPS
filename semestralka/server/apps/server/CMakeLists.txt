auto_create_executable(server
    PRIVATE_DEPS net threadpool
    CONSOLE
    OUTPUT_NAME "hive_server"
    VERSION 1.0.0
)
