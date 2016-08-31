cmake_minimum_required (VERSION 2.6)
set(PROJECT_NAME_STR ConfigurationService)
project(${PROJECT_NAME_STR} CXX)

# The version number.
set (ConfigurationService_VERSION_MAJOR 0)
set (ConfigurationService_VERSION_MINOR 1)

set (CMAKE_CXX_FLAGS "-std=c++11 -pthread ${CMAKE_CXX_FLAGS}")


## add the binary tree to the search path for include files
include_directories("${PROJECT_SOURCE_DIR}/rapidjson/include")

include( CTest )

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/rapidjson)

enable_testing()
add_custom_target(build_and_test ${CMAKE_CTEST_COMMAND} -V)
add_subdirectory (test)
