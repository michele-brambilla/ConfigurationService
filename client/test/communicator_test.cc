#include <gtest/gtest.h>

#include <configuration.hpp>

int count_test_msg = 0;


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

class CommunicatorManager : public ::testing::Test {
  
protected:
  
  virtual void SetUp() {
    cm = std::make_shared<CM>(redis_server.c_str(),redis_port);
    publisher = std::make_shared<CM>(redis_server.c_str(),redis_port);
    listener = std::make_shared<CM>(redis_server.c_str(),redis_port);

  }
  
  std::shared_ptr<CM> cm;
  std::shared_ptr<CM> publisher;
  std::shared_ptr<CM> listener;

public:
  static std::string redis_server;
  static std::string path;
  static int redis_port;

  const int num_test_msg=10;
};

std::string CommunicatorManager::redis_server = "localhost";
std::string CommunicatorManager::path = "../";
int CommunicatorManager::redis_port   = 6379;



TEST_F (CommunicatorManager, CreateCommunicator) {
  cm->keep_counting = false;
}

TEST_F (CommunicatorManager, CommunicatorEmpty) {
  EXPECT_EQ(cm->NumMessages(),0);
}

TEST_F (CommunicatorManager, AddMessage) {
  EXPECT_EQ(cm->NumMessages(),0);
  EXPECT_TRUE( cm->Publish("key","a") );
  EXPECT_EQ(cm->NumMessages(),1);
  //  cm->Notify(); // prevents segmentation fault
}

TEST_F (CommunicatorManager, AddMessages) {
  EXPECT_EQ(cm->NumMessages(),0);
  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i)
    EXPECT_TRUE( cm->Publish(std::to_string(i),status[i%3]) );
  EXPECT_EQ(cm->NumMessages(),num_test_msg);
  //  cm->Notify(); // prevents segmentation fault
}

TEST_F (CommunicatorManager, Notify) {
  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    cm->Publish(std::string("message:")+std::to_string(i),status[i%3]);
  }
  EXPECT_TRUE( cm->Notify() );
  EXPECT_EQ(cm->NumMessages(),0);
}

TEST_F (CommunicatorManager, AutoNotify) {
  EXPECT_EQ(cm->NumMessages(),0);
  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < 10*configuration::communicator::Communicator::MaxStoredMessages;++i) {
    EXPECT_TRUE( cm->Publish(std::string("message:")+std::to_string(i),status[i%3]) );
  }
  EXPECT_NE(cm->NumMessages(),10*configuration::communicator::Communicator::MaxStoredMessages);
  //  EXPECT_TRUE( cm->Notify() );
}


TEST_F (CommunicatorManager, SubscribeSingleTopic) {
  EXPECT_TRUE( listener->Subscribe("message:1") );
  // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  
  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher->Publish(std::string("message:1"),status[i%3]);
    publisher->Publish(std::string("message:2"),status[i%3]);
    publisher->Publish(std::string("message:3"),status[i%3]);
    publisher->Notify();
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));
  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(listener->NumRecvMessages(),num_test_msg);

}

TEST_F (CommunicatorManager, SubscribeMultipleTopics) {
  EXPECT_TRUE( listener->Subscribe("message:*") );
  // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(1));
  
  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher->Publish(std::string("message:1"),status[i%3]);
    publisher->Publish(std::string("message:2"),status[i%3]);
    publisher->Publish(std::string("message:3"),status[i%3]);
    publisher->Notify();
  }
  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(listener->NumRecvMessages(),num_test_msg*3);
  
}

TEST_F (CommunicatorManager, SubscribeFunctionGotCallback) {
  count_test_msg=0;
  EXPECT_TRUE( listener->Subscribe("message:1",got_message) );
  
  // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::seconds(1));

  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher->Publish(std::string("message:1"),status[i%3]);
    publisher->Publish(std::string("message:2"),status[i%3]);
    publisher->Publish(std::string("message:3"),status[i%3]);
    publisher->Notify();
  }
  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(count_test_msg,num_test_msg);

}

TEST_F (CommunicatorManager, SubscribeFunctionCallbacks) {
  count_test_msg=0;
  EXPECT_TRUE(listener->Subscribe("message:1",got_message,got_error,unsubscribe) );
  
  // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher->Publish(std::string("message:1"),status[i%3]);
    publisher->Publish(std::string("message:2"),status[i%3]);
    publisher->Publish(std::string("message:3"),status[i%3]);
    publisher->Notify();
  }

  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(num_test_msg,count_test_msg);

}

TEST_F (CommunicatorManager, SubscribeMemberGotCallback) {
  configuration::communicator::ReceivedMessageCb got_cb;

  //  EXPECT_TRUE( listener->Subscribe("message:1",got_cb.f) );

  auto f1 = std::bind(&configuration::communicator::ReceivedMessageCb::got_message, &got_cb, std::placeholders::_1, std::placeholders::_2);
  EXPECT_TRUE( listener->Subscribe("message:1", f1) );
  
  // // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher->Publish(std::string("message:1"),status[i%3]);
    publisher->Publish(std::string("message:2"),status[i%3]);
    publisher->Publish(std::string("message:3"),status[i%3]);
    publisher->Notify();
  }
  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(got_cb.num_msg,num_test_msg);
  
}

TEST_F (CommunicatorManager, SubscribeMembersCallback) {
  configuration::communicator::ReceivedMessageCb got_cb;
  configuration::communicator::UnsubscribeCb uns_cb;
  configuration::communicator::SubscribeErrorCb err_cb;

  auto f1 = std::bind(&configuration::communicator::ReceivedMessageCb::got_message, &got_cb, std::placeholders::_1, std::placeholders::_2);
  auto f2 = std::bind(&configuration::communicator::SubscribeErrorCb::got_error,    &err_cb, std::placeholders::_1, std::placeholders::_2);
  auto f3 = std::bind(&configuration::communicator::UnsubscribeCb::unsubscribed,    &uns_cb, std::placeholders::_1);

  EXPECT_TRUE( listener->Subscribe("message:1",f1,f2,f3) );
  
  // // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher->Publish(std::string("message:1"),status[i%3]);
    publisher->Publish(std::string("message:2"),status[i%3]);
    publisher->Publish(std::string("message:3"),status[i%3]);
    publisher->Notify();
  }
  // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_EQ(got_cb.num_msg,num_test_msg);
  
}

TEST_F (CommunicatorManager, UnsubscribeMember) {

  listener->Subscribe("message:1") ;
  listener->Subscribe("message:2") ;
  listener->Subscribe("message:3") ;
  
  // // just ensure enough time to connect
  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  std::vector<std::string> status = {"a","u","d"};
  for(int i = 0; i < num_test_msg;++i) {
    publisher->Publish(std::string("message:1"),status[i%3]);
    publisher->Publish(std::string("message:2"),status[i%3]);
    publisher->Publish(std::string("message:3"),status[i%3]);
    publisher->Notify();
  }
 // after 1 sec you can be confident that all messages arrived
  std::this_thread::sleep_for(std::chrono::seconds(1));
  EXPECT_TRUE( listener->Unsubscribe("message:1") );
    
}

//////////////////
// FIXME
// TEST_F (CommunicatorManager, UnsubscribeMembers) {
//   CM publisher(redis_server,redis_port);
//   CM listener(redis_server,redis_port);

//   configuration::communicator::ReceivedMessageCb got_cb;
//   configuration::communicator::UnsubscribeCb uns_cb;
//   configuration::communicator::SubscribeErrorCb err_cb;

//   listener->Subscribe("message:*",got_cb.f,err_cb.f,uns_cb.f);
  
//   // // just ensure enough time to connect
//   std::this_thread::sleep_for(std::chrono::milliseconds(10));

//   std::vector<std::string> status = {"a","u","d"};
//   for(int i = 0; i < num_test_msg;++i) {
//     publisher->Publish(std::string("message:1")+std::to_string(i),status[i%3]);
//     publisher->Publish(std::string("message:1")+std::to_string(i),status[i%3]);
//     publisher->Publish(std::string("message:2")+std::to_string(i),status[i%3]);
//     publisher->Notify();
//   }
//   // after 1 sec you can be confident that all messages arrived
//   std::this_thread::sleep_for(std::chrono::seconds(1));

//   for( auto s : listener->ListTopics() )
//     std::cout << s << std::endl;
  
//   std::this_thread::sleep_for(std::chrono::seconds(1));
//   EXPECT_TRUE( listener->Unsubscribe("message:*",err_cb.f) );

//   for( auto s : listener->ListTopics() )
//     std::cout << s << std::endl;

// }




int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  if(std::string(argv[0]).substr(0,5) == "build")
    CommunicatorManager::path = "./";
  else {
    int found = std::string(argv[0]).find("/build/test/communicator_test");
    if (found != std::string::npos)
      CommunicatorManager::path = std::string(argv[0]).substr(0,found+1);      
  }
 
  //////////////////
  // Reads info from configuration file
  std::ifstream in;
  in.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  try {
    in.open(CommunicatorManager::path+"configuration_service.config");
  }
  catch(std::ifstream::failure& e) {
    throw std::runtime_error(e.what());
  }

  std::string next, sep,value;
  do {
    in >> next >> sep >> value;
    if( next == "data_addr" ) CommunicatorManager::redis_server = value;
    if( next == "data_port" ) std::istringstream(value) >> CommunicatorManager::redis_port;
  } while(!in.eof() );


  for(int i=1;i<argc;++i) {
    size_t found = std::string(argv[i]).find("=");
    if( std::string(argv[i]).substr(0,found) == "--port")
      CommunicatorManager::redis_port = std::atoi(std::string(argv[i]).substr(found+1).c_str());
    if( std::string(argv[i]).substr(0,found) ==  "--server")
      CommunicatorManager::redis_server = std::string(argv[i]).substr(found+1);
    if( ( std::string(argv[i]).substr(0,found) ==  "--help" ) || 
	(std::string(argv[i]).substr(0,1) == "-h") ) {
      std::cout << "\nOptions: " << "\n"
		<< "\t--server=<address>\n"
		<< "\t--port=<port>\n";
    }
    
  }
  
  std::cout  << "\t--server=" << CommunicatorManager::redis_server << "\t"
	     << "\t--port="   << CommunicatorManager::redis_port << "\n";

  //  // hack for automated testing
  // {
  //   redox::Redox rdx;
  //   if( !rdx.connect(redis_server.c_str()) )
  //     redis_server="localhost";
  // }
  
  return RUN_ALL_TESTS();
}
