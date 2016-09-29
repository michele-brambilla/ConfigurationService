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


        // publisher->connect(std::string("tcp://*:")+std::to_string(zmq_publisher_port));

        std::string connect_to=(std::string("tcp://")+
                                device_server+
                                ":5554");
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

      // void PingPong() {
      //   zmq::message_t prequest (6);
      //   memcpy ((void *) prequest.data (), "Hello", 5);
      //   std::cout << "Sending Hello ";
      //   std::cout.flush();

      //   publisher->send(prequest);
      //   std::cout << "..." << std::endl;
        
      //   std::cout << "Waiting for receiving" << std::endl;
      //        }

      void Disconnect() {
        keep_listening=false;
	result_subscriber.get();
      }
      
      // bool Notify() override {
      //   std::string cmd = "PUBLISH ";
      //   int nclients;
      //   bool is_ok = true;
      //   for(auto& msg : updates) {
      //     is_ok &= utils::ExecRedisCmd<int>(*publisher,
      //                                       cmd+msg.first+" "+msg.second,
      //                                       &nclients);
      //     if( nclients == 0 )
      //       log << msg.first << ": no connected clients\n";
      //   }
      //   last_notify_time = std::chrono::system_clock::now();
      //   updates.clear();
      //   return is_ok;
      // }

      // // bool Publish(const std::string& key,const std::string& status) override {
      // //   updates.insert(std::pair<std::string,std::string>(key,status) );
      // //   if(updates.size() > MaxStoredMessages) {
      // //     log << "Max number of stored messages ("+std::to_string(MaxStoredMessages)+") reached, proceed to notify"
      // //         << std::endl;
      // //     return Notify();
      // //   }
      // //   total_num_messages++;
      // //   return true;
      // // }

      bool Subscribe(const std::string& filter) {

	subscriber->setsockopt(ZMQ_SUBSCRIBE,filter.c_str(), filter.length());
        
        return true;
      }
      
      bool Subscribe(const std::string& filter,
                     std::function<void(const std::string&,const std::string&)>, // got message
                     std::function<void(const std::string&,const int&)>,
                     std::function<void(const std::string&)>
                     ) override {
        
        subscriber->setsockopt(ZMQ_SUBSCRIBE,filter.c_str(), filter.length());

        return true;
      }

      
      // bool Unsubscribe(const std::string& key,
      //                  std::function<void(const std::string&,const int&)> got_error = default_got_error
      //                  ) override {
      //   bool is_ok = true;
      //   std::size_t found = key.find("*");

      //   if ( found != std::string::npos) {
      //     std::string short_key(key);
      //     short_key.pop_back();
      //     auto list = subscriber->psubscribedTopics();
      //     if ( list.find(short_key) == list.end() )
      //       return false;
      //     subscriber->punsubscribe(short_key.substr(0,found-1),got_error);
      //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
      //     list = subscriber->psubscribedTopics();
      //     if ( list.find(short_key) == list.end() )
      //       return false;

      //   }
      //   else {
      //     subscriber->unsubscribe(key,got_error);
      //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
      //     for( auto& s : subscriber->subscribedTopics())
      //       if( s == key )
      //         is_ok = false;
      //   }

      //   return is_ok;
      // }

      // std::set<std::string> ListTopics() {
      //   std::set<std::string> s = subscriber->subscribedTopics();
      //   s.insert(subscriber->psubscribedTopics().begin(),
      //            subscriber->psubscribedTopics().end()
      //            );
      //   return s;
      // }

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
      // unsigned long int total_recv_messages = 0;

      std::chrono::system_clock::time_point last_notify_time;

      std::string device_server;
      int zmq_subscriber_port;
      int zmq_publisher_port;
      std::future<void> result_subscriber;

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
  
