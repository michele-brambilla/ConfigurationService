#include <gtest/gtest.h>

#include <configuration.hpp>

const char* instrument_file = "../sample/example_instrument.js";


typedef configuration::MockManager DM;
typedef configuration::MockManager CM;

using namespace configuration;

TEST (UploadConfig, ValidFile) {
  std::ifstream in;
  try {
    in.open(instrument_file);
    in.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  }
  catch (std::ifstream::failure e) {
    std::cout << "Exception opening/reading file: " << e.what() << std::endl;
  }
  
  int d,c;
  ConfigurationService<DM,CM> cs(d,c);
  EXPECT_TRUE(cs.UploadConfig(in));
}

// TEST (UploadConfig, ValidString) {
//   int d,c;
//   ConfigurationService<DM,CM> cs(d,c);

//   EXPECT_TRUE(cs.UploadConfig(""));
// }

// TEST (UploadConfig, RecordPresent) {
//   int d,c;
//   ConfigurationService<DM,CM> cs(d,c);
//   EXPECT_TRUE(cs.UploadConfig(""));
// }

// TEST (UploadConfig, RecordNotPresent) {
//   int d,c;
//   ConfigurationService<DM,CM> cs(d,c);
//   EXPECT_TRUE(cs.UploadConfig(""));
// }




int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
