#include <gtest/gtest.h>

#include <configuration.hpp>

std::string redis_server="192.168.10.11";
static const int redis_port=6379;
static const int num_test_msg=10;

int count_test_msg;
void got_message(const std::string & t,const std::string & c)  {
  std::cerr << "Using function callback > "<< t << "\t" << c << std::endl;
  std::cerr << "Message number > "<< "\t" << count_test_msg << std::endl;
  count_test_msg++;
}
void got_error(const std::string & t,const int & v)  {
  std::cerr << "Error function callback: > "<< t << "\t->\t" << v << std::endl;
}
void unsubscribe(const std::string & t)  {
  std::cerr << "Unsubscribe function callback < "<< t << std::endl;
}

typedef configuration::communicator::RedisCommunicator CM;
using namespace configuration::communicator;

TEST (CommunicatorManager, CreateCommunicator) {
  CM cm(redis_server,redis_port);
  cm.keep_counting = false;
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
  for(int i = 0; i < 10*configuration::communicator::Communicator::MaxStoredMessages;++i) {
    EXPECT_TRUE( cm.Publish(std::string("message:")+std::to_string(i),status[i%3]) );
  }
  EXPECT_NE(cm.NumMessages(),10*configuration::communicator::Communicator::MaxStoredMessages);
}


TEST (CommunicatorManager, SubscribeSingleTopic) {
  CM publisher(redis_server,redis_port);
  CM listener(redis_server,redis_port);
  EXPECT_TRUE( listener.Subscribe("message:1") );
  // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  
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
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  
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

TEST (CommunicatorManager, SubscribeFunctionGotCallback) {
  CM publisher(redis_server,redis_port);
  CM listener(redis_server,redis_port);

  count_test_msg=0;
  EXPECT_TRUE( listener.Subscribe("message:1",got_message) );
  
  // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::seconds(1));

  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher.Publish(std::string("message:1"),status[i%3]);
    publisher.Publish(std::string("message:2"),status[i%3]);
    publisher.Publish(std::string("message:3"),status[i%3]);
    publisher.Notify();
  }
  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(count_test_msg,num_test_msg);

}

TEST (CommunicatorManager, SubscribeFunctionCallbacks) {
  CM publisher(redis_server,redis_port);
  CM listener(redis_server,redis_port);

  count_test_msg=0;
  EXPECT_TRUE(listener.Subscribe("message:1",got_message,got_error,unsubscribe) );
  
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
  EXPECT_EQ(num_test_msg,count_test_msg);

}

TEST (CommunicatorManager, SubscribeMemberGotCallback) {
  CM publisher(redis_server,redis_port);
  CM listener(redis_server,redis_port);

  configuration::communicator::ReceivedMessageCb got_cb;

  //  EXPECT_TRUE( listener.Subscribe("message:1",got_cb.f) );

  auto f1 = std::bind(&configuration::communicator::ReceivedMessageCb::got_message, &got_cb, std::placeholders::_1, std::placeholders::_2);
  EXPECT_TRUE( listener.Subscribe("message:1", f1) );
  
  // // just ensure enough time to connect
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
  EXPECT_EQ(got_cb.num_msg,num_test_msg);
  
}

TEST (CommunicatorManager, SubscribeMembersCallback) {
  CM publisher(redis_server,redis_port);
  CM listener(redis_server,redis_port);

  configuration::communicator::ReceivedMessageCb got_cb;
  configuration::communicator::UnsubscribeCb uns_cb;
  configuration::communicator::SubscribeErrorCb err_cb;

  auto f1 = std::bind(&configuration::communicator::ReceivedMessageCb::got_message, &got_cb, std::placeholders::_1, std::placeholders::_2);
  auto f2 = std::bind(&configuration::communicator::SubscribeErrorCb::got_error,    &err_cb, std::placeholders::_1, std::placeholders::_2);
  auto f3 = std::bind(&configuration::communicator::UnsubscribeCb::unsubscribed,    &uns_cb, std::placeholders::_1);

  EXPECT_TRUE( listener.Subscribe("message:1",f1,f2,f3) );
  
  // // just ensure enough time to connect
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
  EXPECT_EQ(got_cb.num_msg,num_test_msg);
  
}

TEST (CommunicatorManager, UnsubscribeMember) {

  CM publisher(redis_server,redis_port);
  CM listener(redis_server,redis_port);

  listener.Subscribe("message:1") ;
  listener.Subscribe("message:2") ;
  listener.Subscribe("message:3") ;
  
  // // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  std::vector<std::string> status = {"a","u","d"};
  // for(int i = 0; i < num_test_msg;++i) {
  //   publisher.Publish(std::string("message:1"),status[i%3]);
  //   publisher.Publish(std::string("message:2"),status[i%3]);
  //   publisher.Publish(std::string("message:3"),status[i%3]);
  //   publisher.Notify();
  // }
  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_TRUE( listener.Unsubscribe("message:1") );
    
}

//////////////////
// FIXME
// TEST (CommunicatorManager, UnsubscribeMembers) {
//   CM publisher(redis_server,redis_port);
//   CM listener(redis_server,redis_port);

//   configuration::communicator::ReceivedMessageCb got_cb;
//   configuration::communicator::UnsubscribeCb uns_cb;
//   configuration::communicator::SubscribeErrorCb err_cb;

//   listener.Subscribe("message:*",got_cb.f,err_cb.f,uns_cb.f);
  
//   // // just ensure enough time to connect
//   std::this_thread::sleep_for(std::chrono::milliseconds(10));

//   std::vector<std::string> status = {"a","u","d"};
//   for(int i = 0; i < num_test_msg;++i) {
//     publisher.Publish(std::string("message:1")+std::to_string(i),status[i%3]);
//     publisher.Publish(std::string("message:1")+std::to_string(i),status[i%3]);
//     publisher.Publish(std::string("message:2")+std::to_string(i),status[i%3]);
//     publisher.Notify();
//   }
//   // after 1 sec you can be confident that all messages arrived
//   std::this_thread::sleep_for(std::chrono::seconds(1));

//   for( auto s : listener.ListTopics() )
//     std::cout << s << std::endl;
  
//   std::this_thread::sleep_for(std::chrono::seconds(1));
//   EXPECT_TRUE( listener.Unsubscribe("message:*",err_cb.f) );

//   for( auto s : listener.ListTopics() )
//     std::cout << s << std::endl;

// }




int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

   // hack for automated testing
  {
    redox::Redox rdx;
    if( !rdx.connect(redis_server.c_str()) )
      redis_server="localhost";
  }
  
  return RUN_ALL_TESTS();
}
