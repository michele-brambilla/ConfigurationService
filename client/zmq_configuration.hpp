#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <map>
#include <set>
#include <functional>
#include <algorithm>

#include <basic_communicator.hpp>
#include <basic_data.hpp>

namespace zmq {
  class context_t;
  class socket_t;
}

namespace configuration {


  namespace communicator {

    
    struct ZmqCommunicator : public Communicator {
      static int MaxStoredMessages;
      static int NotificationTimeout;
      int publisher_connection_status;
      int subscriber_connection_status;

      ZmqCommunicator() = default;
      ZmqCommunicator(const std::string&,
                      const int& =5555,
                      const int& =5554,
                      std::ostream& =std::cerr);
      
      ~ZmqCommunicator() {
        keep_listening=false;
	result_subscriber.get();
      }

      void Disconnect() {
        keep_listening=false;
	result_subscriber.get();
      }
      
      bool Notify() override;

      bool Subscribe(const std::string&);
      
      bool Subscribe(const std::string&,
                     std::function<void(const std::string&,const std::string&)>,
                     std::function<void(const std::string&,const int&)>,
                     std::function<void(const std::string&)>
                     ) override;
      
      bool Unsubscribe(const std::string&,
                       std::function<void(const std::string&,const int&)> = default_got_error
                       ) override;
      
      std::set<std::string> ListTopics() { return subscriptions; }

      bool keep_counting = true;
    private:

      bool keep_listening;
      std::shared_ptr<zmq::context_t> context;
      std::shared_ptr<zmq::socket_t> publisher;
      std::shared_ptr<zmq::socket_t> subscriber;
      
      std::ostream& log;
      unsigned long int total_recv_messages = 0;
      std::chrono::system_clock::time_point last_notify_time;
      std::string device_server;
      int zmq_subscriber_port;
      int zmq_publisher_port;
      std::future<void> result_subscriber;

      std::set<std::string> subscriptions;
     
      void Listener( std::function<void(const std::string&,
                                        const std::string&)>&&,
                     std::function<void(const std::string&,
                                        const int&)>,
                     std::function<void(const std::string&)>
                     );
      
    };
    
  }
}
  
