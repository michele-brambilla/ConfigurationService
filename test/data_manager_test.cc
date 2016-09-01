#include <gtest/gtest.h>

#include <data.hpp>
#include <communication.hpp>


const char* instrument_file = "../sample/example_instrument.js";

std::ifstream open_config_file(const char* s) {
  std::ifstream in;
  try {
    in.open(s);
    in.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  }
  catch (std::ifstream::failure e) {
    std::cout << "Exception opening/reading file: " << e.what() << std::endl;
  }
  return in;
}

std::string read_config_file(const char* s) {
  std::ifstream in = open_config_file(s);
  std::string config,buf;
  while(!in.eof()) {
     std::getline(in, buf,'\t');
     config += buf;
  }
  in.close();
  return config;
}



typedef configuration::data::MockDataManager DM;
typedef configuration::communicator::FileCommunicator CM;


using namespace configuration::data;

TEST (DataManager, ValidString) {
  EXPECT_TRUE( DM().IsValidString( read_config_file(instrument_file) ) );
}

TEST (DataManager, AddConfig) {
  DM dm;
  EXPECT_TRUE( dm.IsValidString( read_config_file(instrument_file) ) );
  EXPECT_TRUE( dm.AddConfig( read_config_file(instrument_file) ) );
}

TEST (DataManager, AddNewConfig) {
  DM dm;
  EXPECT_TRUE( dm.AddConfig( read_config_file(instrument_file) ) );
  //  dm.Dump();
  // add new config on top of existing one
  EXPECT_FALSE( dm.AddConfig( read_config_file("../sample/example_instrument.js") ) );
  EXPECT_TRUE( dm.AddConfig( read_config_file("../sample/example_instrument2.js") ) );
}


TEST (DataManager, QueryHash) {
  DM dm;
  dm.AddConfig( read_config_file(instrument_file) );
  // an existing hash key must return a vector containing one element
  EXPECT_TRUE( dm.Query("instrument1:sources:motor4:type").size() == 1 );
  EXPECT_TRUE( dm.Query("instrument1:sources:motor4:type")[0] == std::string("ca-motor") );
  EXPECT_FALSE( dm.Query("instrument1:sources:motor4:type")[0] == std::string("any-value") );
  // a non-exising hash key must return an empty vector
  EXPECT_TRUE( dm.Query("instrument1:sources:motor4:notype").size() == 0 );
}

TEST (DataManager, QuerySet) {
  DM dm;
  dm.AddConfig( read_config_file(instrument_file) );
  std::vector<std::string> expect_failure = { "type","address","something else"};
  std::vector<std::string> expect_success = { "type","address"};
  std::vector<std::string> content;

  // a non-exising hash key must return an empty vector
  EXPECT_TRUE( dm.Query("instrument1:sources:nomotor").size() == 0 );

  content = dm.Query("instrument1:sources:motor4");

  // an existing set key must return a vector containing at least one element
  EXPECT_TRUE( content.size() != 0  );
  // test set length and content
  EXPECT_TRUE( content.size() == expect_success.size() );
  for( auto& s : expect_success)
    EXPECT_TRUE( std::find(content.begin(), content.end(), s) != content.end() );
  EXPECT_FALSE( content.size() == expect_failure.size() );
  bool is_true = true;
  for( auto& s : expect_failure)
    is_true &= ( std::find(content.begin(), content.end(), s) != content.end() );
  EXPECT_FALSE( is_true);
  
}

TEST (DataManager, UpdateHash) {
  DM dm;
  dm.AddConfig( read_config_file(instrument_file) );
  // test original value
  EXPECT_TRUE( dm.Query("instrument1:sources:motor4:type")[0] == std::string("ca-motor") );
  EXPECT_FALSE( dm.Query("instrument1:sources:motor4:type")[0] == std::string("new-ca-motor") );
  // test update success
  EXPECT_TRUE( dm.Update("instrument1:sources:motor4:type","new-ca-motor") );
  // test new value
  EXPECT_FALSE( dm.Query("instrument1:sources:motor4:type")[0] == std::string("ca-motor") );
  EXPECT_TRUE( dm.Query("instrument1:sources:motor4:type")[0] == std::string("new-ca-motor") );
}

TEST (DataManager, UpdateSet) {
  DM dm;
  dm.AddConfig( read_config_file(instrument_file) );
  // overwrite previous config
  EXPECT_TRUE( dm.Update("instrument1:sources:motor1:type","ca-motor") );
  // add new field to config
  EXPECT_TRUE( dm.Update("instrument1:sources:motor1:address-backup","IOC:m1-backup") );
  // add new field to parent
  EXPECT_TRUE( dm.Update("instrument1:x:sources:temp1:address","STC1") );
  EXPECT_TRUE( dm.Update("instrument1:x:sources:temp1:type","pv-temp") );
  //  dm.Dump();
}

TEST (DataManager, DeleteHash) {
  DM dm;
  dm.AddConfig( read_config_file(instrument_file) );
  // test key existence
  int size = dm.Query("instrument1:sources:motor4:type").size();
  EXPECT_TRUE( size > 0 );
  // test delete success
  EXPECT_TRUE( dm.Delete("instrument1:sources:motor4:type") );
  // test missing key
  EXPECT_FALSE( dm.Query("instrument1:sources:motor4:type").size() > 0 );

  // test success in deleting non-existing key
  EXPECT_TRUE( dm.Delete("instrument1:sources:motor4:type") );
}

TEST (DataManager, DeleteSet) {
  DM dm;
  dm.AddConfig( read_config_file(instrument_file) );
  // test key existence
  int size = dm.Query("instrument1:sources").size();
  EXPECT_TRUE( size > 0 );
  // test delete success
  EXPECT_TRUE( dm.Delete("instrument1:sources") );
  // test missing key
  EXPECT_FALSE( dm.Query("instrument1:sources").size() > 0 );
  // test other keys not affected
  for (auto& key : dm.Query("instrument1") ) {
    EXPECT_TRUE( dm.Query("instrument1:"+key).size() > 0 );
  }
  // test success in deleting non-existing key
  EXPECT_TRUE( dm.Delete("instrument1:sources") );
  CM c;
  EXPECT_TRUE( dm.Notify<CM>(c) );
}


TEST (DataManager, Updates) {
  DM dm;
  // test updates initially void
  EXPECT_TRUE( dm.UpdatesList().size() == 0 );
  dm.AddConfig( read_config_file(instrument_file) );
  EXPECT_TRUE( dm.Update("instrument1:sources:motor4:type","new-ca-motor") );
  // test updates existence
  EXPECT_TRUE( dm.UpdatesList().size() > 0 );
  // test delete success
  CM c;
  EXPECT_TRUE( dm.Notify<CM>(c) );
  // test updates empty after notification
  EXPECT_FALSE( dm.UpdatesList().size() > 0 );
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
