#include <gtest/gtest.h>
#include <algorithm>

#include <configuration.hpp>

std::string path;
const char* instrument_file = "sample/example_instrument.js";
const char* new_instrument_file = "sample/example_instrument2.js";

std::string read_config_file(const char* s) {
  std::ifstream in;
  in.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  try {
    in.open(s);
  }
  catch (std::ifstream::failure e) {
    std::cout << "Exception opening/reading file: " << e.what() << std::endl;
  }
  std::string config,buf;
  while(!in.eof()) {
     std::getline(in, buf,'\t');
     config += buf;
  }
  in.close();
  return config;
}

std::string redis_server="192.168.10.11";
static const int redis_port=6379;


typedef configuration::communicator::MockCommunicator CM;
using DM = configuration::data::RedisDataManager<CM>;


CM comm(redis_server.c_str(),redis_port);
DM dm(redis_server.c_str(),redis_port,comm);

using namespace configuration::data;

TEST (DataManager, ValidString) {
  EXPECT_TRUE( dm.IsValidString( read_config_file((path+instrument_file).c_str()) ) );
}

TEST (DataManager, AddConfig) {
  //  DM dm(redis_server,redis_port,comm);

  dm.Clear();
  std::string s(read_config_file((path+instrument_file).c_str()));
  EXPECT_TRUE( dm.AddConfig(s) );
}

TEST (DataManager, AddNewConfig) {
  // DM< FCM> dm(redis_server,redis_port,comm);
  dm.Clear();
  EXPECT_TRUE( dm.AddConfig( read_config_file((path+instrument_file).c_str()) ) );
  // add new config on top of existing one
  EXPECT_FALSE( dm.AddConfig( read_config_file((path+instrument_file).c_str()) ) );
  // EXPECT_TRUE( dm.AddConfig( read_config_file((path+new_instrument_file).c_str()) ) );
}

TEST (DataManager, QueryHash) {
  // DM< FCM> dm(redis_server,redis_port,comm);
   dm.Clear();
   dm.AddConfig( read_config_file((path+instrument_file).c_str()) );
  
  // an existing hash key must return a vector containing one element
  std::string key("instrument1:sources:motor2:type");
  ASSERT_EQ( dm.Query(key).size(), 1 );
  EXPECT_EQ( dm.Query("instrument1:sources:motor4:type")[0], std::string("ca-motor") );
   //  a non-exising hash key must return an empty vector
  EXPECT_EQ( dm.Query("instrument1:sources:motor4:notype").size(), 0 );
}

TEST (DataManager, QuerySet) {
  // DM< FCM> dm(redis_server,redis_port,comm);
  dm.Clear();
  dm.AddConfig( read_config_file((path+instrument_file).c_str()) );
  std::vector<std::string> expect_failure = { "type","address","something else"};
  std::vector<std::string> expect_success = { "type","address"};
  std::vector<std::string> content;

  // a non-exising hash key must return an empty vector
  // content = dm.Query("instrument1:sources:nomotor");
  // EXPECT_TRUE( content.size() == 0 );

  content = dm.Query("instrument1:sources:motor4");

  // an existing set key must return a vector containing at least one element
  EXPECT_TRUE( content.size() != 0  );
  // test set length and content
  EXPECT_EQ( content.size(), expect_success.size() );

  // for( int i=0;i<expect_success.size();++i)
  //   EXPECT_TRUE( std::any_of(content.begin(), content.end(), [&](std::string s){return s==expect_success[i];}) );
  for( auto& s : expect_success)
    EXPECT_TRUE( std::any_of(content.begin(), content.end(), [&](std::string c){return c==s;}) );
  //      EXPECT_TRUE( std::find(content.begin(), content.end(), s) != content.end() );   // --> broken on gcc-4.8

  EXPECT_FALSE( content.size() == expect_failure.size() );
  bool is_true = true;
  for( auto& s : expect_failure)
    is_true &= std::any_of(content.begin(), content.end(), [&](std::string c){return c==s;});

  
  //    is_true &= ( std::find(content.begin(), content.end(), s) != content.end() ); // --> broken on gcc-4.8

  EXPECT_FALSE( is_true);
}

TEST (DataManager, UpdateHash) {
  // DM< FCM> dm(redis_server,redis_port,comm);
  dm.Clear();
  dm.AddConfig( read_config_file((path+instrument_file).c_str()) );
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
  // DM< FCM> dm(redis_server,redis_port,comm);
  dm.Clear();
  dm.AddConfig( read_config_file((path+instrument_file).c_str()) );
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
  // DM< FCM> dm(redis_server,redis_port,comm);
  dm.Clear();
  dm.AddConfig( read_config_file((path+instrument_file).c_str()) );
  // test key existence
  int size = dm.Query("instrument1:sources:motor4:type").size();
  ASSERT_TRUE( size > 0 );
  // test delete success
  EXPECT_TRUE( dm.Delete("instrument1:sources:motor4:type") );

  // test missing key
  EXPECT_FALSE( dm.Query("instrument1:sources:motor4:type").size() > 0 );

  // test success in deleting non-existing key
  EXPECT_FALSE( dm.Delete("instrument1:sources:motor4:type") );
}

TEST (DataManager, DeleteSet) {
  // DM< FCM> dm(redis_server,redis_port,comm);
  dm.Clear();
  dm.AddConfig( read_config_file((path+instrument_file).c_str()) );
  // // test key existence
  // int size = dm.Query("instrument1:sources").size();
  // ASSERT_TRUE( size > 0 );

  // // test delete success
  // EXPECT_TRUE( dm.Delete("instrument1:sources") );

  // // test missing key
  // EXPECT_FALSE( dm.Query("instrument1:sources").size() > 0 );
  // // test other keys not affected
  // for (auto& key : dm.Query("instrument1") ) {
  //   EXPECT_TRUE( dm.Query("instrument1:"+key).size() > 0 );
  // }
  // //  test failure in deleting non-existing key
  // EXPECT_FALSE( dm.Delete("instrument1:sources") );

  // test deletion parent when empty
  EXPECT_TRUE( dm.Delete("instrument1:experiment:id") );
  //  EXPECT_TRUE( dm.Delete("instrument1:experiment:name") );
  // EXPECT_FALSE( (dm.Query("instrument1:experiment").size() > 0 ) );
  
}










int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  if(argc > 1)
    path = std::string(argv[1])+"/";
  else
    path = "../";
  
  // hack for automated testing
  {
    redox::Redox rdx;
    if( !rdx.connect(redis_server.c_str()) )
      redis_server="localhost";
  }

  
  return RUN_ALL_TESTS();
}
