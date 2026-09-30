auto_create_executable(server
    DEPENDS net threadpool
    CONSOLE
    OUTPUT_NAME "hive_server"
    VERSION 1.0.0
    CXX_STD 20
)
