cmake_minimum_required (VERSION 2.6)
set(PROJECT_NAME_STR ConfigurationService)
project(${PROJECT_NAME_STR} CXX)

# The version number.
set (ConfigurationService_VERSION_MAJOR 0)
set (ConfigurationService_VERSION_MINOR 1)


set (CMAKE_CXX_FLAGS "-std=c++11 -pthread -O3 -Wno-unsupported-friend")
set (CMAKE_CXX_FLAGS_DEBUG "-std=c++11 -O0 -pthread -Wall -ggdb")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
## add the binary tree to the search path for include files
include_directories(${PROJECT_SOURCE_DIR}
  ${PROJECT_SOURCE_DIR}/rapidjson/include
  ${PROJECT_SOURCE_DIR}/rapidjson
  ${PROJECT_SOURCE_DIR}/redox/include
  ${PROJECT_SOURCE_DIR}/cppzmq)


MESSAGE( STATUS "TEST_SOURCE_DIR:            " ${TEST_SOURCE_DIR} )
MESSAGE( STATUS "BUILD_SOURCE_DIR:           " ${BUILD_SOURCE_DIR} )
MESSAGE( STATUS "PROJECT_SOURCE_DIR:         " ${PROJECT_SOURCE_DIR} )
MESSAGE( STATUS "CMAKE_CTEST_COMMAND:        " ${CMAKE_CTEST_COMMAND} )

add_subdirectory("${PROJECT_SOURCE_DIR}/redox")
link_directories("${PROJECT_SOURCE_DIR}/redox/build")

add_library( configmanager configuration.cc
  zmq_configuration.cc
  redis_configuration.cc
  redis_utils.cc
  basic_communicator.cc
  basic_data.cc
  container.cc )

add_executable (Service Service.cc) 

target_link_libraries( Service redox configmanager)
  
include( CTest )

enable_testing()
add_custom_target(build_and_test ${CMAKE_CTEST_COMMAND} -V)
add_subdirectory (test)
