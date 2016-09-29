#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <map>
#include <functional>
#include <algorithm>

#include <zmq.hpp>
#include <zmq_addon.hpp>

#include <basic_communicator.hpp>
#include <basic_data.hpp>


namespace configuration {


  namespace communicator {

    
    struct ZmqCommunicator : public Communicator {
      static int MaxStoredMessages;
      static int NotificationTimeout;
      int publisher_connection_status;
      int subscriber_connection_status;

      zmq::message_t srequest; 
      
      ZmqCommunicator(const std::string& server,
                      const int& publisher_port=5555,
                      const int& subscriber_port=5554,                      
                      std::ostream& logger=std::cerr) 
        : keep_listening(true),
        context(std::make_shared<zmq::context_t>(1)),
          publisher(std::make_shared<zmq::socket_t>(*context, ZMQ_PUB)),
          subscriber(std::make_shared<zmq::socket_t>(*context, ZMQ_SUB)),
          log(logger),
          last_notify_time(std::chrono::system_clock::now()),
        device_server(server),
        zmq_publisher_port(publisher_port),
        zmq_subscriber_port(subscriber_port)
      {


        std::string connect_to=(std::string("tcp://")+
                                device_server+
                                ":"+std::to_string(zmq_publisher_port) );
        try{
          publisher->connect(connect_to);
        }
        catch (std::exception& e) {
          std::cout << e.what() << std::endl;
        }
        
        connect_to=(std::string("tcp://")+
                    device_server+
                    ":"+std::to_string(zmq_subscriber_port) );
        std::cout << connect_to << std::endl;
        try{
          subscriber->connect(connect_to);
        }
        catch (std::exception& e) {
          std::cout << e.what() << std::endl;
        }
        result_subscriber = std::async(std::launch::async,
				       &ZmqCommunicator::Listener,this,
                                       default_got_message,
                                       default_got_error,
                                       default_unsubscribed 
                                       );
      }

      ~ZmqCommunicator() {
        keep_listening=false;
	result_subscriber.get();
      }

      void Disconnect() {
        keep_listening=false;
	result_subscriber.get();
      }
      
      bool Notify() override {
        zmq::message_t prequest (1024);
        bool is_ok = true;
        for(auto& msg : updates) {
	  try {
            std::string s=msg.first;
            s+=" : "+msg.second;
            memcpy ((void *) prequest.data (), s.c_str(), s.size());
            publisher->send(prequest);
          }
          catch (std::exception& e){
            std::cout << "publisher failure: " << e.what() << std::endl;
            is_ok = false;
          }
        }

        last_notify_time = std::chrono::system_clock::now();
        updates.clear();
        return is_ok;
      }

      bool Subscribe(const std::string& filter) {
        if( subscriptions.find(filter) == subscriptions.end())
          subscriber->setsockopt(ZMQ_SUBSCRIBE,filter.c_str(), filter.length());
        else {
          log << "Already listening for " << filter << ": command ignored" << std::endl;
          return false;
        }
        return true;
      }
      
      bool Subscribe(const std::string& filter,
                     std::function<void(const std::string&,const std::string&)>, // got message
                     std::function<void(const std::string&,const int&)>,
                     std::function<void(const std::string&)>
                     ) override {
        if( subscriptions.find(filter) == subscriptions.end())
          subscriber->setsockopt(ZMQ_SUBSCRIBE,filter.c_str(), filter.length());
        else {
          log << "Already listening for " << filter << ": command ignored" << std::endl;
          return false;
        }
        return true;
      }

      
      bool Unsubscribe(const std::string& filter,
                       std::function<void(const std::string&,const int&)> got_error = default_got_error
                       ) override {
        bool is_ok = true;

        if( subscriptions.find(filter) != subscriptions.end())
          subscriber->setsockopt(ZMQ_UNSUBSCRIBE,filter.c_str(), filter.length());
        else {
          log << "Not subscribed to " << filter << ": command ignored" << std::endl;
          return false;
        }

        return is_ok;
      }

      std::set<std::string> ListTopics() { return subscriptions; }


      // bool Reconnect() {

      //   publisher->connect(redis_server, redis_port,
      //                     std::bind(utils::redis_connection_callback,
      //                               std::placeholders::_1,
      //                               std::ref(publisher_connection_status)));
      //   subscriber->connect(redis_server, redis_port,
      //                      std::bind(utils::redis_connection_callback,
      //                                std::placeholders::_1,
      //                                std::ref(subscriber_connection_status)));

      //   return ( (publisher_connection_status != redox::Redox::CONNECTED) &&
      //            (subscriber_connection_status != redox::Redox::CONNECTED) ) ;
      // }
      
      // int NumMessages() const { return updates.size(); }
      // int NumRecvMessages() const { return total_recv_messages; }
      bool keep_counting = true;
    private:
      bool keep_listening;
      std::shared_ptr<zmq::context_t> context;
      std::shared_ptr<zmq::socket_t> publisher;
      std::shared_ptr<zmq::socket_t> subscriber;
      
      std::ostream& log;
      // unsigned long int total_num_messages = 0;
      unsigned long int total_recv_messages = 0;

      std::chrono::system_clock::time_point last_notify_time;

      std::string device_server;
      int zmq_subscriber_port;
      int zmq_publisher_port;
      std::future<void> result_subscriber;

      std::set<std::string> subscriptions;

      // void TimedNotification() {
      //   while(this->keep_counting) {
      //     std::this_thread::sleep_for(std::chrono::seconds(NotificationTimeout));
      //     log << NotificationTimeout << "s elapsed, auto-notification will occur\n";
      //     Notify();
      //   }
      //   log << "TimedNotification terminated\n";
      // }
      
      
      void Listener( std::function<void(const std::string&,
                                        const std::string&)>&& f=default_got_message,
                     std::function<void(const std::string&,
                                        const int&)> g = default_got_error,
                     std::function<void(const std::string&)> h = default_unsubscribed
                     ) {
	while(keep_listening) {

	  try {
            subscriber->recv(&srequest);
            f("Received",std::string((char*)srequest.data()).substr(0,srequest.size()));
          }
          catch (std::exception& e){
            g(e.what(),-1);
          }
          //          if(!keep_listening) break;
	}

      }
      

      
    };
    
  }
}
  
