#include <gtest/gtest.h>

#include <configuration.hpp>

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




using CM = configuration::communicator::RedisCommunicator;
using DM = configuration::data::RedisDataManager<CM>;
using ConfigurationService = configuration::ConfigurationManager<DM,CM>;

using namespace configuration;

std::string path;
const char* instrument_file = "sample/example_instrument.js";
const char* new_instrument_file = "sample/example_instrument2.js";
std::string redis_server = "192.168.10.11";
const int redis_port = 6379;

ConfigurationService cs(redis_server.c_str(),redis_port,redis_server.c_str(),redis_port);

TEST (Connection, ConnectionOK) {
  ASSERT_NO_THROW(ConfigurationService(redis_server.c_str(),redis_port,redis_server.c_str(),redis_port));
}
TEST (Connection, ConnectionFail) {
  ASSERT_ANY_THROW(ConfigurationService("localhost",redis_port,redis_server.c_str(),redis_port));
}


TEST (UploadConfig, ValidConfiguration) {

  std::cout << "read config from file: " << (path+instrument_file) << std::endl;

  std::string in = read_config_file((path+instrument_file).c_str()); 
  
  std::cout << in << std::endl;
  {
    CM c(redis_server,redis_port);
    DM d(redis_server,redis_port,c);
    d.Clear();
  }
  
  EXPECT_TRUE(cs.AddConfig(in));
}

TEST (UploadConfig, RecordPresent) {
  std::string in = read_config_file((path+instrument_file).c_str());
  EXPECT_FALSE(cs.AddConfig(in));
}

TEST (UploadConfig, RecordNotPresent) {
  std::string in = read_config_file((path+new_instrument_file).c_str());
  EXPECT_TRUE(cs.AddConfig(in));
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
