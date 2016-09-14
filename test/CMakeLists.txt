
include_directories (
  ${PROJECT_SOURCE_DIR}
  ${TEST_SOURCE_DIR}
  ) 

add_executable (communicator_test communicator_test.cc ${PROJECT_SOURCE_DIR}/redis_configuration.cc) 
add_executable (data_test data_manager_test.cc) 
add_executable (configuration_test configuration_test.cc ${PROJECT_SOURCE_DIR}/redis_configuration.cc) 


find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIR})

target_link_libraries( configuration_test
		       ${GTEST_BOTH_LIBRARIES}
		       redox)

target_link_libraries( communicator_test
		       ${GTEST_BOTH_LIBRARIES}
		       redox)

target_link_libraries( data_test
		       ${GTEST_BOTH_LIBRARIES}
		       redox)


add_test(NAME communication_test COMMAND $<TARGET_FILE:communicator_test> WORKING_DIRECTORY ${TEST_SOURCE_DIR})
add_test(NAME data_test COMMAND $<TARGET_FILE:data_test> ${PROJECT_SOURCE_DIR} WORKING_DIRECTORY ${TEST_SOURCE_DIR})
add_test(NAME configuration_test COMMAND $<TARGET_FILE:configuration_test> ${PROJECT_SOURCE_DIR} WORKING_DIRECTORY ${BUILD_SOURCE_DIR})
