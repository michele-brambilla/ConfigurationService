#include <gtest/gtest.h>

#include <configuration.hpp>


const char* instrument_file = "../sample/example_instrument.js";

std::ifstream open_config_file(const char* s) {
  std::ifstream in;
  try {
    in.open(instrument_file);
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



typedef configuration::MockDataManager DM;
typedef configuration::MockCommunicationManager CM;
DM d;
CM c;

using namespace configuration;



TEST (UploadConfig, ValidFile) {
  std::ifstream in = open_config_file(instrument_file); 
  
  ConfigurationService<DM,CM> cs(d,c);
  EXPECT_TRUE(cs.UploadConfig(in));
}

TEST (UploadConfig, ValidString) {
  std::string config = read_config_file(instrument_file);
  
  ConfigurationService<DM,CM> cs(d,c);

  EXPECT_TRUE(cs.UploadConfig(config));
}

TEST (UploadConfig, RecordPresent) {
  std::string config = read_config_file(instrument_file);
  
  ConfigurationService<DM,CM> cs(d,c);
  EXPECT_TRUE(cs.UploadConfig(config));
}

TEST (UploadConfig, RecordNotPresent) {
  std::string config = read_config_file(instrument_file);
 
  ConfigurationService<DM,CM> cs(d,c);
  EXPECT_TRUE(cs.UploadConfig(config));
}




int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
