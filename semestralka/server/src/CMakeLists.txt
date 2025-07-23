# Handle external dependencies first
set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE OFF)

# Add external dependencies
file(GLOB_RECURSE EXTERNAL_CMAKE_FILES ./external/*/CMakeLists.txt)
set(PROCESSED_DIRS "")
foreach(CMAKE_FILE ${EXTERNAL_CMAKE_FILES})
    get_filename_component(DIR ${CMAKE_FILE} DIRECTORY)

    # Check if this directory or a parent has already been processed
    set(SHOULD_PROCESS TRUE)
    foreach(PROCESSED_DIR ${PROCESSED_DIRS})
        # Check if current dir is subdirectory of already processed dir
        string(FIND ${DIR} ${PROCESSED_DIR} PARENT_MATCH)
        if(PARENT_MATCH EQUAL 0)
            set(SHOULD_PROCESS FALSE)
            break()
        endif()
    endforeach()

    if(SHOULD_PROCESS)
        add_subdirectory(${DIR})
        list(APPEND PROCESSED_DIRS ${DIR})

        # Make external includes available globally
        if(EXISTS ${DIR}/include)
            include_directories(${DIR}/include)
        endif()
    endif()
endforeach()

# Auto-discover and build all library subdirectories
file(GLOB LIB_DIRS LIST_DIRECTORIES TRUE ./*)
foreach(DIR ${LIB_DIRS})
    get_filename_component(DIR_NAME ${DIR} NAME)
    if(IS_DIRECTORY ${DIR} AND EXISTS ${DIR}/CMakeLists.txt AND NOT DIR_NAME STREQUAL "external")
        add_subdirectory(${DIR})
    endif()
endforeach()

# Export all created libraries for use by executables
get_directory_property(ALL_TARGETS BUILDSYSTEM_TARGETS)
set(PROJECT_LIBRARIES "")
foreach(TARGET ${ALL_TARGETS})
    get_target_property(TARGET_TYPE ${TARGET} TYPE)
    if(TARGET_TYPE STREQUAL "STATIC_LIBRARY" OR TARGET_TYPE STREQUAL "SHARED_LIBRARY" OR TARGET_TYPE STREQUAL "INTERFACE_LIBRARY")
        list(APPEND PROJECT_LIBRARIES ${TARGET})
    endif()
endforeach()

# Make the library list available to parent scope
set(PROJECT_LIBRARIES ${PROJECT_LIBRARIES} PARENT_SCOPE)

# Install headers
file(GLOB INCLUDE_DIRS LIST_DIRECTORIES TRUE include/*)
if(INCLUDE_DIRS)
    install(DIRECTORY ${INCLUDE_DIRS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
