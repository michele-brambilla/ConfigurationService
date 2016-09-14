cmake_minimum_required (VERSION 2.6)
set(PROJECT_NAME_STR ConfigurationService)
project(${PROJECT_NAME_STR} CXX)

# The version number.
set (ConfigurationService_VERSION_MAJOR 0)
set (ConfigurationService_VERSION_MINOR 1)


set (CMAKE_CXX_FLAGS "-std=c++11 -pthread -O3 -Wno-unsupported-friend")
set (CMAKE_CXX_FLAGS_DEBUG "-std=c++11 -O0 -pthread -Wall")

add_subdirectory("${PROJECT_SOURCE_DIR}/redox")

## add the binary tree to the search path for include files
include_directories("${PROJECT_SOURCE_DIR}/rapidjson/include")
include_directories("${PROJECT_SOURCE_DIR}/redox/include")
link_directories("${PROJECT_SOURCE_DIR}/redox/build")

add_executable (dummy dummy.cc configuration.cc redis_configuration.cc) 
target_link_libraries( dummy redox)
  
include( CTest )

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/rapidjson)

enable_testing()
add_custom_target(build_and_test ${CMAKE_CTEST_COMMAND} -V)
add_subdirectory (test)
