#include <gtest/gtest.h>

#include <configuration.hpp>

static const int num_test_msg=10;


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


void counting_got_message(const std::string &,const std::string &,int& count)  {
  count++;
}




using CM = configuration::communicator::RedisCommunicator;
using DM = configuration::data::RedisDataManager<CM>;
using Configuration = configuration::ConfigurationManager<DM,CM>;


const char* instrument_file = "sample/example_instrument.js";
const char* new_instrument_file = "sample/example_instrument2.js";

class ConfigurationService : public ::testing::Test {
  
protected:
  
  virtual void SetUp() {
    cs = std::make_shared<Configuration>(redis_server.c_str(),redis_port,
					 redis_server.c_str(),redis_port);
  }
  
  std::shared_ptr<Configuration> cs;

public:
  static std::string path;
  static std::string redis_server;
  static int redis_port;

};

std::string ConfigurationService::path         = "../";
std::string ConfigurationService::redis_server = "localhost";
int ConfigurationService::redis_port   = 6379;
//configuration::communicator::RedisCommunicator::NotificationTimeout = 1;


TEST_F (ConfigurationService,ConnectionOK) {
  ASSERT_NO_THROW(Configuration(redis_server.c_str(),redis_port,redis_server.c_str(),redis_port));
}
// TEST_F (ConfigurationService,ConnectionFail) {
//   ASSERT_ANY_THROW(Configuration("localhost",redis_port,redis_server.c_str(),redis_port));
// }


TEST_F (ConfigurationService,ValidConfiguration) {
  std::string in = read_config_file((path+instrument_file).c_str()); 
  {
    CM c(redis_server,redis_port);
    DM d(redis_server,redis_port,c);
    d.Clear();
  }
  EXPECT_TRUE(cs->AddConfig(in));
}

TEST_F (ConfigurationService,RecordPresent) {
  std::string in = read_config_file((path+instrument_file).c_str());
  EXPECT_FALSE(cs->AddConfig(in));
}

TEST_F (ConfigurationService,RecordNotPresent) {
  std::string in = read_config_file((path+new_instrument_file).c_str());
  EXPECT_TRUE(cs->AddConfig(in));
}


TEST_F (ConfigurationService,QueryKeyValue) {
  // an existing hash key must return a vector containing one element
  std::string key("instrument1:sources:motor2:type");
  EXPECT_EQ( cs->Query(key).size(), 1 );
  EXPECT_EQ( cs->Query("instrument1:sources:motor4:type")[0], std::string("ca-motor") );
  EXPECT_NE( cs->Query("instrument1:sources:motor4:type")[0], std::string("new-ca-motor") );
  
   //  a non-exising hash key must return an empty vector
  EXPECT_EQ( cs->Query("instrument1:sources:motor4:notype").size(), 0 );
}

TEST_F (ConfigurationService,QueryMultipleValues) {
  std::vector<std::string> expect_failure = { "type","address","something else"};
  std::vector<std::string> expect_success = { "type","address"};
  std::vector<std::string> content;

  // a non-exising hash key must return an empty vector
  content = cs->Query("instrument1:sources:nomotor");
  EXPECT_TRUE( content.size() == 0 );

  content = cs->Query("instrument1:sources:motor4");
  // an existing set key must return a vector containing at least one element
  EXPECT_TRUE( content.size() != 0  );
  // test set length and content
  EXPECT_EQ( content.size(), expect_success.size() );

  for( auto& s : expect_success)
    EXPECT_TRUE( std::any_of(content.begin(), content.end(), [&](std::string c){return c==s;}) );

  EXPECT_FALSE( content.size() == expect_failure.size() );
  bool is_true = true;
  for( auto& s : expect_failure)
    is_true &= std::any_of(content.begin(), content.end(), [&](std::string c){return c==s;});

  EXPECT_FALSE( is_true);
}


TEST_F (ConfigurationService,UpdateKeyValue) {
  // test update success
  EXPECT_TRUE( cs->Update("instrument1:sources:motor4:type","new-ca-motor") );
  // test new value
  EXPECT_NE( cs->Query("instrument1:sources:motor4:type")[0], std::string("ca-motor") );
  EXPECT_EQ( cs->Query("instrument1:sources:motor4:type")[0], std::string("new-ca-motor") );
  // revert
  EXPECT_TRUE( cs->Update("instrument1:sources:motor4:type","ca-motor") );
  EXPECT_EQ( cs->Query("instrument1:sources:motor4:type")[0], std::string("ca-motor") );
  
}

TEST_F (ConfigurationService,UpdateMultipleValues) {

  std::vector<std::string> content;

  EXPECT_EQ( cs->Query("instrument1:sources:motor1:address-backup").size() ,0);

  // add field and test size
  EXPECT_TRUE( cs->Update("instrument1:sources:motor1:address-backup","IOC:m1-backup") );
  EXPECT_EQ( cs->Query("instrument1:sources:motor1:address-backup").size(), 1 );

  // add new field to parent
  EXPECT_TRUE( cs->Update("instrument1:x:sources:temp1:address","STC1") );

  content = cs->Query("instrument1:x:sources:temp1");
  EXPECT_TRUE( content.size() > 0);
  EXPECT_FALSE( content.size() > 1);
  
  EXPECT_TRUE( cs->Update("instrument1:x:sources:temp1:type","pv-temp") );
  content = cs->Query("instrument1:x:sources:temp1");
  EXPECT_EQ( content.size(),2);
}


TEST_F (ConfigurationService,DeleteKeyValue) {

  // test key existence
  ASSERT_TRUE( cs->Query("instrument1:sources:motor4:type").size() > 0 );
  // test delete success
  EXPECT_TRUE( cs->Delete("instrument1:sources:motor4:type") );

  // test key removed
  EXPECT_FALSE( cs->Query("instrument1:sources:motor4:type").size() > 0 );

  // test failure in deleting non-existing key
  EXPECT_FALSE( cs->Delete("instrument1:sources:motor4:type") );

}

TEST_F (ConfigurationService,DeleteMultipleValues) {

  ASSERT_TRUE( cs->Query("instrument1:sources").size() > 0 );

  // test delete success
  EXPECT_TRUE( cs->Delete("instrument1:sources") );

  // test missing key
  EXPECT_FALSE( cs->Query("instrument1:sources").size() > 0 );
  // test other keys not affected
  for (auto& key : cs->Query("instrument1") ) {
    EXPECT_TRUE( cs->Query("instrument1:"+key).size() > 0 );
  }
  //  test failure in deleting non-existing key
  EXPECT_FALSE( cs->Delete("instrument1:sources") );

  // test deletion parent when empty
  EXPECT_TRUE( cs->Delete("instrument1:experiment:id") );
  EXPECT_TRUE( cs->Delete("instrument1:experiment:name") );
  EXPECT_FALSE( (cs->Query("instrument1:experiment").size() > 0 ) );
  
}



TEST_F (ConfigurationService,CommunicationsSubscribeTopic) {

  CM publisher(redis_server,redis_port);
  int n_recv = 0;
  
  auto f = std::bind(counting_got_message,
                     std::placeholders::_1,
                     std::placeholders::_2,
                     std::ref(n_recv));

  ASSERT_TRUE( cs->Subscribe("message:1",f) );
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  publisher.Publish(std::string("message:1"),"a");
  publisher.Notify();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_EQ(n_recv,1);
  n_recv = 0;
  
  for(int i = 0; i < num_test_msg;++i) {
    publisher.Publish(std::string("message:1"),std::to_string(i));
  }
  publisher.Notify();
  
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_EQ(n_recv,num_test_msg);
  n_recv = 0;
    
  for(int i = 0; i < num_test_msg;++i) {
    publisher.Publish(std::string("message:1"),std::to_string(i));
    publisher.Notify();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_EQ(n_recv,num_test_msg);

  publisher.Disconnect();
}

TEST_F (ConfigurationService,CommunicationsNonSubscribedTopics) {

  CM publisher(redis_server,redis_port);
  int n_recv = 0;
  
  auto f = std::bind(counting_got_message,
                     std::placeholders::_1,
                     std::placeholders::_2,
                     std::ref(n_recv));
  
  ASSERT_TRUE( cs->Subscribe("message:1",f) );
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  publisher.Publish(std::string("message:1"),"a");
  publisher.Publish(std::string("message:2"),"b");
  publisher.Publish(std::string("message:3"),"c");
  publisher.Notify();

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_EQ(n_recv,1);
  n_recv=0;

  for(int i = 0; i < num_test_msg;++i) {
    publisher.Publish(std::string("message:1"),std::to_string(i));
    publisher.Publish(std::string("message:2"),std::to_string(i));
    publisher.Publish(std::string("message:3"),std::to_string(i));
  }
  publisher.Notify();
  
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_EQ(n_recv,num_test_msg);
  n_recv=0;
  
  for(int i = 0; i < num_test_msg;++i) {
    publisher.Publish(std::string("message:1"),std::to_string(i));
    publisher.Publish(std::string("message:2"),std::to_string(i));
    publisher.Publish(std::string("message:3"),std::to_string(i));
    publisher.Notify();
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  EXPECT_EQ(n_recv,num_test_msg);

  publisher.Disconnect();
}


TEST_F (ConfigurationService,CommunicationsSubscribeMultipleTopics) {
  CM publisher(redis_server,redis_port);

  int n_recv = 0;
  auto f = std::bind(counting_got_message,
                     std::placeholders::_1,
                     std::placeholders::_2,
                     std::ref(n_recv));
  
  ASSERT_TRUE( cs->Subscribe("message:*",f) );

  // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher.Publish(std::string("message:1"),status[i%3]);
    publisher.Publish(std::string("message:2"),status[i%3]);
    publisher.Publish(std::string("message:3"),status[i%3]);
  }
  publisher.Notify();

  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(n_recv,num_test_msg*3);
  
}



// TEST_F (ConfigurationService,Communications, Notify) {

//   int n_recv = 0;
//   auto f = std::bind(counting_got_message,
//                      std::placeholders::_1,
//                      std::placeholders::_2,
//                      std::ref(n_recv));
  
//   ASSERT_TRUE( cs->Subscribe("instrument1:sources:motor4:type",f) );
//   std::this_thread::sleep_for(std::chrono::milliseconds(10));

//   ASSERT_TRUE( cs->Update("instrument1:sources:motor4:type","new-ca-motor") );
//   std::this_thread::sleep_for(std::chrono::milliseconds(10));
//   EXPECT_TRUE( cs->Notify() );
//   std::this_thread::sleep_for(std::chrono::milliseconds(10));

//   EXPECT_EQ( n_recv,1 );

//   ASSERT_TRUE( cs->Update("instrument1:sources:motor4:type","IOC:ca-motor") );
//   EXPECT_TRUE( cs->Notify() );
//   std::this_thread::sleep_for(std::chrono::milliseconds(10));

//   EXPECT_EQ( n_recv,2 );
  
// }


// TEST_F (ConfigurationService,Communications, Notify2) {

//   int n_recv = 0;
//   auto f = std::bind(counting_got_message,
//                      std::placeholders::_1,
//                      std::placeholders::_2,
//                      std::ref(n_recv));
  
//   ASSERT_TRUE( cs->Subscribe("instrument1:sources:motor4:*",f) );
//   ASSERT_TRUE( cs->Update("instrument1:sources:motor4:type","new-ca-motor") );
//   ASSERT_TRUE( cs->Update("instrument1:sources:motor4:address","IOC:ca-motor") );
//   EXPECT_TRUE( cs->Notify() );

//   std::this_thread::sleep_for(std::chrono::milliseconds(10));

//   EXPECT_EQ( n_recv,2 );

  
// }

TEST_F (ConfigurationService,Disconnect) {
  EXPECT_EQ( cs->DataConnectionStatus(),       configuration::CONNECTED );
  EXPECT_EQ( cs->PublisherConnectionStatus(),  configuration::CONNECTED );
  EXPECT_EQ( cs->SubscriberConnectionStatus(), configuration::CONNECTED );  
  EXPECT_TRUE( cs->Disconnect() );
}



int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  if(std::string(argv[0]).substr(0,5) == "build")
    ConfigurationService::path = "./";
  
  //std::cout << (ConfigurationService::path+instrument_file) << std::endl;

  for(int i=1;i<argc;++i) {
    size_t found = std::string(argv[i]).find("=");
    if( std::string(argv[i]).substr(0,found) == "--port")
      ConfigurationService::redis_port = std::atoi(std::string(argv[i]).substr(found+1).c_str());
    if( std::string(argv[i]).substr(0,found) ==  "--server")
      ConfigurationService::redis_server = std::string(argv[i]).substr(found+1);
    if( ( std::string(argv[i]).substr(0,found) ==  "--help" ) || 
	(std::string(argv[i]).substr(0,1) == "-h") ) {
      std::cout << "\nOptions: " << "\n"
		<< "\t--server=<address>\n"
		<< "\t--port=<port>\n";
    }

  }

  // // hack for automated testing
  // {
  //   redox::Redox rdx;
  //   if( !rdx.connect(redis_server.c_str()) )
  //     redis_server="localhost";
  // }
      
  return RUN_ALL_TESTS();
}
