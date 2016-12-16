set(TEST_SOURCE_DIR ${PROJECT_SOURCE_DIR}/test)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/tests)

include_directories (
  ${PROJECT_SOURCE_DIR}
  ${TEST_SOURCE_DIR}
  ) 

add_executable (communicator_test communicator_test.cc) 
add_executable (data_test data_manager_test.cc) 
add_executable (configuration_test configuration_test.cc) 


find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

target_link_libraries( configuration_test
		       ${GTEST_BOTH_LIBRARIES}
		       redox
                       configmanager)

target_link_libraries( communicator_test
		       ${GTEST_BOTH_LIBRARIES}
		       redox
                       configmanager)

target_link_libraries( data_test
		       ${GTEST_BOTH_LIBRARIES}
		       redox
                       configmanager)

MESSAGE( STATUS "TEST_SOURCE_DIR:            " ${TEST_SOURCE_DIR} )
MESSAGE( STATUS "BUILD_SOURCE_DIR:           " ${BUILD_SOURCE_DIR} )
MESSAGE( STATUS "PROJECT_SOURCE_DIR:         " ${PROJECT_SOURCE_DIR} )
MESSAGE( STATUS "CMAKE_BINARY_DIR:           " ${CMAKE_BINARY_DIR})
MESSAGE( STATUS "CMAKE_CURRENT_BINARY_DIR: " ${CMAKE_CURRENT_BINARY_DIR} )

#add_test(NAME communication_test COMMAND $<TARGET_FILE:communicator_test> WORKING_DIRECTORY ${TEST_SOURCE_DIR})
#add_test(NAME data_test COMMAND $<TARGET_FILE:data_test> ${PROJECT_SOURCE_DIR} WORKING_DIRECTORY ${TEST_SOURCE_DIR})
#add_test(NAME configuration_test COMMAND $<TARGET_FILE:configuration_test> WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME data_test WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/data_test)
add_test(NAME communicator_test WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/communicator_test)
add_test(NAME configuration_test WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${CMAKE_CURRENT_BINARY_DIR}/configuration_test)
