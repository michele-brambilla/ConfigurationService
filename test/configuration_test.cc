#include <gtest/gtest.h>

#include <configuration.hpp>

typedef configuration::MockManager DM;
typedef configuration::MockManager CM;

using namespace configuration;

TEST (UploadConfig, ValidString) {
  int d,c;
  ConfigurationService<DM,CM> cs(d,c);

  EXPECT_TRUE(cs.UploadConfig(""));
}

TEST (UploadConfig, ValidFile) {
  int d,c;
  ConfigurationService<DM,CM> cs(d,c);
  try {
    std::ifstream in("example_config.js");
    in.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  }
  catch (std::ifstream::failure e) {
    std::cout << "Exception opening/reading file";
  }
  
  EXPECT_TRUE(cs.UploadConfig(""));
}


TEST (UploadConfig, RecordPresent) {
  int d,c;
  ConfigurationService<DM,CM> cs(d,c);
  EXPECT_TRUE(cs.UploadConfig(""));
}

TEST (UploadConfig, RecordNotPresent) {
  int d,c;
  ConfigurationService<DM,CM> cs(d,c);
  EXPECT_TRUE(cs.UploadConfig(""));
}




int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
