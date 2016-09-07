#include <gtest/gtest.h>

#include <redis_communicator.hpp>

static const std::string redis_server="192.168.10.11";
static const int redis_port=6379;
static const int num_test_msg=10;


void got_message(const std::string & t,const std::string & c)  {
  std::cout << "Using function callback > "<< t << "\t" << c << std::endl;
}

struct DummyCb {
  DummyCb() : num_msg(0) { };
  void got_message(const std::string & t,const std::string & c) const {
    std::cout << t << "\t" << c << std::endl;
    // topic = t;
    // key = k;
    // num_msg++;
  }
  std::string topic;
  std::string content;
  int num_msg;
};



typedef configuration::communicator::RedisCommunicator CM;

using namespace configuration::communicator;

TEST (CommunicatorManager, CreateCommunicator) {
  CM cm(redis_server,redis_port);
}

TEST (CommunicatorManager, CommunicatorEmpty) {
  CM cm(redis_server,redis_port);
  EXPECT_EQ(cm.NumMessages(),0);
}


TEST (CommunicatorManager, AddMessage) {
  CM cm(redis_server,redis_port);
  EXPECT_EQ(cm.NumMessages(),0);
  EXPECT_TRUE( cm.Publish("key","a") );
  EXPECT_EQ(cm.NumMessages(),1);
}


TEST (CommunicatorManager, AddMessages) {
  CM cm(redis_server,redis_port);
  EXPECT_EQ(cm.NumMessages(),0);
  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i)
    EXPECT_TRUE( cm.Publish(std::to_string(i),status[i%3]) );
  EXPECT_EQ(cm.NumMessages(),num_test_msg);
}

TEST (CommunicatorManager, Notify) {
  CM cm(redis_server,redis_port);
  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    cm.Publish(std::string("message:")+std::to_string(i),status[i%3]);
  }
  EXPECT_TRUE( cm.Notify() );
  EXPECT_EQ(cm.NumMessages(),0);
}


TEST (CommunicatorManager, AutoNotify) {
  CM cm(redis_server,redis_port);
  EXPECT_EQ(cm.NumMessages(),0);
  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < 10*CM::MaxStoredMessages;++i) {
    EXPECT_TRUE( cm.Publish(std::string("message:")+std::to_string(i),status[i%3]) );
  }
  EXPECT_NE(cm.NumMessages(),10*CM::MaxStoredMessages);
}


TEST (CommunicatorManager, SubscribeSingleTopic) {
  CM publisher(redis_server,redis_port);
  CM listener(redis_server,redis_port);
  EXPECT_TRUE( listener.Subscribe("message:1") );
  // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher.Publish(std::string("message:1"),status[i%3]);
    publisher.Publish(std::string("message:2"),status[i%3]);
    publisher.Publish(std::string("message:3"),status[i%3]);
    publisher.Notify();
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(listener.NumRecvMessages(),num_test_msg);

}



TEST (CommunicatorManager, SubscribeMultipleTopics) {
  CM publisher(redis_server,redis_port);
  CM listener(redis_server,redis_port);
  EXPECT_TRUE( listener.Subscribe("message:*") );
  // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher.Publish(std::string("message:1"),status[i%3]);
    publisher.Publish(std::string("message:2"),status[i%3]);
    publisher.Publish(std::string("message:3"),status[i%3]);
    publisher.Notify();
  }
  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(listener.NumRecvMessages(),num_test_msg*3);
  
}



TEST (CommunicatorManager, SubscribeFunctionCallback) {
  CM publisher(redis_server,redis_port);
  CM listener(redis_server,redis_port);

  DummyCb got_cb;
  
  EXPECT_TRUE( listener.Subscribe("message:1",got_message) );
  
  // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher.Publish(std::string("message:1"),status[i%3]);
    publisher.Publish(std::string("message:2"),status[i%3]);
    publisher.Publish(std::string("message:3"),status[i%3]);
    publisher.Notify();
  }
  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  // EXPECT_EQ(listener.NumRecvMessages(),num_test_msg);

}



int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
