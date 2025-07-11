# Include the library template
include(${CMAKE_SOURCE_DIR}/cmake/LibraryTemplate.cmake)

# Handle external dependencies first
set(EXPECTED_BUILD_TESTS OFF)
set(EXPECTED_BUILD_PACKAGE OFF)

# Add external dependencies
file(GLOB EXTERNAL_DIRS LIST_DIRECTORIES TRUE ./external/*)
foreach(DIR ${EXTERNAL_DIRS})
    if(EXISTS ${DIR}/CMakeLists.txt)
        add_subdirectory(${DIR})
        # Make external includes available globally
        if(EXISTS ${DIR}/include)
            include_directories(${DIR}/include)
        endif()
    endif()
endforeach()

# Find system packages (add as needed)
# find_package(fmt QUIET)

# Auto-discover and build all library subdirectories
file(GLOB LIB_DIRS LIST_DIRECTORIES TRUE ./*)
foreach(DIR ${LIB_DIRS})
    # Skip external directory and non-directories
    get_filename_component(DIR_NAME ${DIR} NAME)
    if(IS_DIRECTORY ${DIR} AND EXISTS ${DIR}/CMakeLists.txt AND NOT DIR_NAME STREQUAL "external")
        add_subdirectory(${DIR})
    endif()
endforeach()

# Export all created libraries for use by executables
# This creates a list of all libraries that were built
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
