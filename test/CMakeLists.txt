
include_directories (
  ${PROJECT_SOURCE_DIR}
  ${TEST_SOURCE_DIR}
  ) 

add_executable (configuration_test configuration_test.cc) 

target_link_libraries( configuration_test
#  ${CMAKE_CURRENT_SOURCE_DIR}/../build/libprova.dylib
  ${GTEST_LIB}libgtest.a
  ${GTEST_LIB}libgtest_main.a)


enable_testing()
add_test(NAME UnitTest COMMAND configuration_test WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
