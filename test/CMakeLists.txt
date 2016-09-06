
include_directories (
  ${PROJECT_SOURCE_DIR}
  ${TEST_SOURCE_DIR}
  ) 

add_executable (communicator_test communicator_test.cc) 
add_executable (data_test data_manager_test.cc) 

#target_link_libraries( configuration_test
##  ${CMAKE_CURRENT_SOURCE_DIR}/../build/libprova.dylib
#  ${GTEST_LIB}libgtest.a
#  ${GTEST_LIB}libgtest_main.a)
target_link_libraries( communicator_test
  ${GTEST_LIB}libgtest.a
  ${GTEST_LIB}libgtest_main.a
  redox)

target_link_libraries( data_test
#  ${CMAKE_CURRENT_SOURCE_DIR}/../build/libprova.dylib
  ${GTEST_LIB}libgtest.a
  ${GTEST_LIB}libgtest_main.a
  redox)


enable_testing()

add_test(NAME communication_test COMMAND $<TARGET_FILE:communicator_test> WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
add_test(NAME data_test COMMAND $<TARGET_FILE:data_test> WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
