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

static const std::string redis_server="192.168.10.11";
static const int redis_port=6379;
// typedef configuration::data::MockDataManager DM;

typedef configuration::data::RedisDataManager DM;
typedef configuration::communicator::FileCommunicator FCM;


using namespace configuration::data;

TEST (DataManager, ValidString) {
  EXPECT_TRUE( DM(redis_server,redis_port).IsValidString( read_config_file(instrument_file) ) );
}

TEST (DataManager, AddConfig) {
  DM dm(redis_server,redis_port);
  dm.Clear();
  EXPECT_TRUE( dm.IsValidString( read_config_file(instrument_file) ) );
  EXPECT_TRUE( dm.AddConfig( read_config_file(instrument_file) ) );
}

TEST (DataManager, AddNewConfig) {
  DM dm(redis_server,redis_port);
  dm.Clear();
  EXPECT_TRUE( dm.AddConfig( read_config_file(instrument_file) ) );
  // add new config on top of existing one
  EXPECT_FALSE( dm.AddConfig( read_config_file("../sample/example_instrument.js") ) );
  EXPECT_TRUE( dm.AddConfig( read_config_file("../sample/example_instrument2.js") ) );
}


TEST (DataManager, QueryHash) {
  DM dm(redis_server,redis_port);
   dm.Clear();
   dm.AddConfig( read_config_file(instrument_file) );
  
  // an existing hash key must return a vector containing one element
  std::string key("instrument1:sources:motor2:type");
  EXPECT_EQ( dm.Query(key).size(), 1 );
  EXPECT_EQ( dm.Query("instrument1:sources:motor4:type")[0], std::string("ca-motor") );
   //  a non-exising hash key must return an empty vector
  EXPECT_EQ( dm.Query("instrument1:sources:motor4:notype").size(), 0 );
}

TEST (DataManager, QuerySet) {
  DM dm(redis_server,redis_port);
  dm.Clear();
  dm.AddConfig( read_config_file(instrument_file) );
  std::vector<std::string> expect_failure = { "type","address","something else"};
  std::vector<std::string> expect_success = { "type","address"};
  std::vector<std::string> content;

  // a non-exising hash key must return an empty vector
  content = dm.Query("instrument1:sources:nomotor");
  EXPECT_TRUE( content.size() == 0 );

  content = dm.Query("instrument1:sources:motor4");

  // an existing set key must return a vector containing at least one element
  EXPECT_TRUE( content.size() != 0  );
  // test set length and content
  EXPECT_EQ( content.size(), expect_success.size() );
  for( auto& s : expect_success)
    EXPECT_TRUE( std::find(content.begin(), content.end(), s) != content.end() );
  EXPECT_FALSE( content.size() == expect_failure.size() );
  bool is_true = true;
  for( auto& s : expect_failure)
    is_true &= ( std::find(content.begin(), content.end(), s) != content.end() );
  EXPECT_FALSE( is_true);
}

TEST (DataManager, UpdateHash) {
  DM dm(redis_server,redis_port);
  dm.Clear();
  dm.AddConfig( read_config_file(instrument_file) );
  // test original value
  ASSERT_EQ( dm.Query("instrument1:sources:motor4:type")[0], std::string("ca-motor") );
  EXPECT_NE( dm.Query("instrument1:sources:motor4:type")[0], std::string("new-ca-motor") );
  // test update success
  EXPECT_TRUE( dm.Update("instrument1:sources:motor4:type","new-ca-motor") );
  // test new value
  EXPECT_NE( dm.Query("instrument1:sources:motor4:type")[0], std::string("ca-motor") );
  EXPECT_EQ( dm.Query("instrument1:sources:motor4:type")[0], std::string("new-ca-motor") );
}

TEST (DataManager, UpdateSet) {
  DM dm(redis_server,redis_port);
  dm.Clear();
  dm.AddConfig( read_config_file(instrument_file) );
  // add new field to config

  std::vector<std::string> content;
  content = dm.Query("instrument1:sources:motor1:address-backup");
  EXPECT_EQ( content.size(),0);
  
  EXPECT_TRUE( dm.Update("instrument1:sources:motor1:address-backup","IOC:m1-backup") );
  content = dm.Query("instrument1:sources:motor1:address-backup");

  // add new field to parent
  EXPECT_TRUE( dm.Update("instrument1:x:sources:temp1:address","STC1") );

  content = dm.Query("instrument1:x:sources:temp1");
  EXPECT_TRUE( content.size() > 0);
  EXPECT_FALSE( content.size() > 1);
  
  EXPECT_TRUE( dm.Update("instrument1:x:sources:temp1:type","pv-temp") );
  content = dm.Query("instrument1:x:sources:temp1");
  EXPECT_EQ( content.size(),2);
  
  //   //  dm.Dump();
}

TEST (DataManager, DeleteHash) {
  DM dm(redis_server,redis_port);
  dm.Clear();
  dm.AddConfig( read_config_file(instrument_file) );
  // test key existence
  int size = dm.Query("instrument1:sources:motor4:type").size();
  EXPECT_TRUE( size > 0 );
  // test delete success
  EXPECT_TRUE( dm.Delete("instrument1:sources:motor4:type") );

  // test missing key
  EXPECT_FALSE( dm.Query("instrument1:sources:motor4:type").size() > 0 );

//   // test success in deleting non-existing key
//   EXPECT_TRUE( dm.Delete("instrument1:sources:motor4:type") );
}

// TEST (DataManager, DeleteSet) {
//   DM dm(redis_server,redis_port);
//   dm.AddConfig( read_config_file(instrument_file) );
//   // test key existence
//   int size = dm.Query("instrument1:sources").size();
//   EXPECT_TRUE( size > 0 );
//   // test delete success
//   EXPECT_TRUE( dm.Delete("instrument1:sources") );
//   // test missing key
//   EXPECT_FALSE( dm.Query("instrument1:sources").size() > 0 );
//   // test other keys not affected
//   for (auto& key : dm.Query("instrument1") ) {
//     EXPECT_TRUE( dm.Query("instrument1:"+key).size() > 0 );
//   }
//   // test success in deleting non-existing key
//   EXPECT_TRUE( dm.Delete("instrument1:sources") );
//   FCM c;
//   EXPECT_TRUE( dm.Notify<FCM>(c) );
//}


// TEST (DataManager, Updates) {
//   DM dm(redis_server,redis_port);
//   // test updates initially void
//   EXPECT_TRUE( dm.UpdatesList().size() == 0 );
//   dm.AddConfig( read_config_file(instrument_file) );
//   EXPECT_TRUE( dm.Update("instrument1:sources:motor4:type","new-ca-motor") );
//   // test updates existence
//   EXPECT_TRUE( dm.UpdatesList().size() > 0 );
//   // test delete success
//   FCM c;
//   EXPECT_TRUE( dm.Notify<FCM>(c) );
//   // test updates empty after notification
//   EXPECT_FALSE( dm.UpdatesList().size() > 0 );
// }

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
