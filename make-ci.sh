#!/usr/bin/env bash
# Build script for continuous integration
set -ev
env | sort

# Install packages
sudo apt-get update
sudo apt-get install -y libhiredis-dev libev-dev libgtest-dev redis-server
sudo apt-get install -y libzmq3-dev

cd client

echo "data_addr : localhost" > configuration_service.config
echo " data_port : 6379" >> configuration_service.config
echo "comm_addr : localhost" >> configuration_service.config
echo "comm_port : 6379" >> configuration_service.config


# Make gtest
git clone https://github.com/google/googletest
cd googletest
mkdir build
cd build
cmake -DBUILD_GMOCK=OFF -DBUILD_GTEST=ON -DBUILD_SHARED_LIBS=ON ..
time make -j2
cd ../..
sudo cp googletest/build/googletest/libg* /usr/local/lib/
sudo cp -r googletest/googletest/include/gtest /usr/local/include

# Make
mkdir build && cd build
cmake .. && make
ctest -V
#test/communicator_test 2>/dev/null | tee
#test/configuration_test 2>/dev/null | tee
#test/data_test 2>/dev/null | tee

