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
      
      ZmqCommunicator(const std::string& server,
                      const int& port=5555,
                      std::ostream& logger=std::cerr) 
        : context(std::make_shared<zmq::context_t>(1)),
          publisher(std::make_shared<zmq::socket_t>(*context, ZMQ_PUSH)),
          subscriber(std::make_shared<zmq::socket_t>(*context, ZMQ_PULL)),
          log(logger),
          last_notify_time(std::chrono::system_clock::now()),
          zmq_server(server),
          zmq_port(port)
      {
        publisher->connect(std::string("tcp://localhost:5555"));//+std::to_string(zmq_port));
        subscriber->connect(std::string("tcp://127.0.0.1:5555"));//+std::to_string(zmq_port));
        
        // publisher->connect(redis_server, redis_port,
        //                    std::bind(utils::redis_connection_callback,
        //                              std::placeholders::_1,
        //                              std::ref(publisher_connection_status)));
        // subscriber->connect(redis_server, redis_port,
        //                     std::bind(utils::redis_connection_callback,
        //                               std::placeholders::_1,
        //                               std::ref(subscriber_connection_status)));
        
        // if( (publisher_connection_status != redox::Redox::CONNECTED) ||
        //     (subscriber_connection_status != redox::Redox::CONNECTED) ) {
        //   log << "Communicator can't connect to REDIS\n";
        //   throw std::runtime_error("Can't connect to REDIS server");
        // }
      }

      ~ZmqCommunicator() {
        // subscriber->disconnect();
        // publisher->disconnect();
        // keep_counting = false;
        // if(t.joinable())
        //   t.join();
      }

      void PingPong() {
        
        zmq::message_t prequest (6);
        memcpy ((void *) prequest.data (), "Hello", 5);
        std::cout << "Sending Hello ";
        std::cout.flush();


        // std::async(std::launch::async,
        //                       [&](){publisher->send(prequest);});
        publisher->send(prequest);
        std::cout << "..." << std::endl;
        
        std::cout << "Waiting for receiving" << std::endl;
          
        zmq::message_t srequest;
        subscriber->recv(&srequest);

        std::cout << "Received " << (char*)srequest.data() << std::endl;

      }

      // void Disconnect() {
      //   publisher->disconnect();
      //   subscriber->disconnect();
      //   if(  (publisher_connection_status != redox::Redox::DISCONNECTED) ||
      //        (subscriber_connection_status != redox::Redox::DISCONNECTED) ) {
      //     throw std::runtime_error("Can't disconnect from REDIS server: error "+
      //                              std::to_string(publisher_connection_status)+","+
      //                              std::to_string(subscriber_connection_status) );
      //   }
      // }
      
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

      // bool Subscribe(const std::string& key) {
      //   bool is_ok = true;
        
      //   auto got_message = [&](const std::string& topic, const std::string& msg) {
      //     total_recv_messages++;
      //     this->log << topic << ": " << msg << std::endl;
      //   };
      //   auto  subscribed = [&](const std::string& topic) {
      //     this->log << "> Subscribed to " << topic << std::endl;
      //   };
      //   auto unsubscribed = [&](const std::string& topic) {
      //     this->log << "> Unsubscribed from " << topic << std::endl;
      //   };
      //   auto got_error = [&](const std::string& topic, const int& id_error) {
      //     this->log << "> Subscription topic " << topic << " error: " << id_error << std::endl;
      //     is_ok = false;
      //   };
        
      //   if ( (key).find("*")!=std::string::npos)
      //     subscriber->psubscribe(key, got_message, subscribed, unsubscribed, got_error);
      //   else
      //     subscriber->subscribe(key, got_message, subscribed, unsubscribed, got_error);
      //   std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
      //   return is_ok;
      // }
      
      // bool Subscribe(const std::string& key,
      //                std::function<void(const std::string&,const std::string&)> got_message, // got message
      //                std::function<void(const std::string&,const int&)> got_error = default_got_error,
      //                std::function<void(const std::string&)> unsubscribed = default_unsubscribed
      //                ) override {

      //   log << "called subscribe (3) " << std::endl;
      //   auto subscribed = [&](const std::string& topic) {
      //     this->log << "> Subscribed to " << topic << std::endl;
      //   };

      //   if ( (key).find("*")!=std::string::npos) {
      //     log << "called psubscribe " << std::endl;
      //     subscriber->psubscribe(key, got_message, subscribed, unsubscribed, got_error);
      //   }
      //   else {
      //     subscriber->subscribe(key, got_message, subscribed, unsubscribed, got_error);
      //     std::this_thread::sleep_for(std::chrono::milliseconds(10));
      //     std::set<std::string> topic_list = subscriber->subscribedTopics();	
      //     if( topic_list.find(key) == topic_list.end() )
      //       return false;
      //   }
        
      //   return true;
      // }

      
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
      std::shared_ptr<zmq::context_t> context;
      std::shared_ptr<zmq::socket_t> publisher;
      std::shared_ptr<zmq::socket_t> subscriber;
      
      std::ostream& log;
      // unsigned long int total_num_messages = 0;
      // unsigned long int total_recv_messages = 0;

      std::chrono::system_clock::time_point last_notify_time;
      std::thread t;

      std::string zmq_server;
      int zmq_port;

      // void TimedNotification() {
      //   while(this->keep_counting) {
      //     std::this_thread::sleep_for(std::chrono::seconds(NotificationTimeout));
      //     log << NotificationTimeout << "s elapsed, auto-notification will occur\n";
      //     Notify();
      //   }
      //   log << "TimedNotification terminated\n";
      // }
      
      
      

      
    };
    
  }
}
  