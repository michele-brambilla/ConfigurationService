#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <map>
#include <functional>

#include <redis_utils.hpp>

#include <basic_communicator.hpp>

namespace configuration {

  namespace communicator {

    
    struct RedisCommunicator : public Communicator {
      static int MaxStoredMessages;
      static int NotificationTimeout;
      
      RedisCommunicator(const std::string& redis_server,
                        const int& redis_port=6379,
                        std::ostream& logger=std::cerr,
                        redox::log::Level redox_loglevel=redox::log::Level::Fatal) 
        : publisher(logger,redox_loglevel), log(logger),
          last_notify_time(std::chrono::system_clock::now())
          //          t(std::thread(&RedisCommunicator::TimedNotification,this))
      {
        if( !publisher.connect(redis_server, redis_port) ) {
          log << "Publisher can't connect to REDIS\n";
          throw std::runtime_error("Can't connect to REDIS server");
        }
        if( !subscriber.connect(redis_server, redis_port) ) {
          log << "Subscriber can't connect to REDIS\n";
          throw std::runtime_error("Can't connect to REDIS server");
        }
      }

      ~RedisCommunicator() {
        keep_counting = false;
        if(t.joinable())
          t.join();
      }

      bool Notify() override {
        std::string cmd = "PUBLISH ";
          int nclients;
        bool is_ok = true;
        for(auto& msg : updates) {
          is_ok &= utils::ExecRedisCmd<int>(publisher,
                                            cmd+msg.first+" "+msg.second,
                                            nclients);
          if( nclients == 0 )
            log << msg.first << ": no connected clients\n";
        }
        last_notify_time = std::chrono::system_clock::now();
        updates.clear();
        return is_ok;
      }

      bool Publish(const std::string& key,const std::string& status) override {
        updates[key]=status;
        if(updates.size() > MaxStoredMessages) {
          log << "Max number of stored messages ("+std::to_string(MaxStoredMessages)+") reached, proceed to notify"
              << std::endl;
          return Notify();
        }
        total_num_messages++;
        return true;
      }
      

      bool Subscribe(const std::string& key) override {
        bool is_ok = true;

        auto got_message = [&](const std::string& topic, const std::string& msg) {
          total_recv_messages++;
          this->log << topic << ": " << msg << std::endl;
        };
        auto  subscribed = [&](const std::string& topic) {
          this->log << "> Subscribed to " << topic << std::endl;
        };
        auto unsubscribed = [&](const std::string& topic) {
          this->log << "> Unsubscribed from " << topic << std::endl;
        };
        auto got_error = [&](const std::string& topic, const int& id_error) {
          this->log << "> Subscription topic " << topic << " error: " << id_error << std::endl;
          is_ok = false;
        };

        if ( (key).find("*")!=std::string::npos)
          subscriber.psubscribe(key, got_message, subscribed, unsubscribed, got_error);
        else
          subscriber.subscribe(key, got_message, subscribed, unsubscribed, got_error);
        
        return is_ok;
      }

      bool Subscribe(const std::string& key,
                     std::function<void(const std::string&,const std::string&)> got_message
                     ) override {

        bool is_ok = true;

        auto subscribed = [&](const std::string& topic) {
          this->log << "> Subscribed to " << topic << std::endl;
        };
        auto unsubscribed = [&](const std::string& topic) {
          this->log << "> Unsubscribed from " << topic << std::endl;
        };
        auto got_error = [&](const std::string& topic, const int& id_error) {
          this->log << "> Subscription topic " << topic << " error: " << id_error << std::endl;
          is_ok = false;
        };

        if ( (key).find("*")!=std::string::npos)
          subscriber.psubscribe(key, got_message, subscribed, unsubscribed, got_error);
        else
          subscriber.subscribe(key, got_message, subscribed, unsubscribed, got_error);
        
        return is_ok;
      }

      bool Subscribe(const std::string& key,
                     std::function<void(const std::string&,const std::string&)> got_message,
                     std::function<void(const std::string&)> unsubscribed,
                     std::function<void(const std::string&,const int&)> got_error
                     ) override {

        auto subscribed = [&](const std::string& topic) {
          this->log << "> Subscribed to " << topic << std::endl;
        };

        if ( (key).find("*")!=std::string::npos)
          subscriber.psubscribe(key, got_message, subscribed, unsubscribed, got_error);
        else
          subscriber.subscribe(key, got_message, subscribed, unsubscribed, got_error);

        std::set<std::string> topic_list = subscriber.subscribedTopics();	
        if( topic_list.find(key) == topic_list.end() )
          return false;
        
        return true;
      }

      bool Unsubscribe(const std::string& key) override {
        bool is_ok = true;
        auto got_error = [&](const std::string& topic, const int& id_error) {
          this->log << "> Subscription topic " << topic << " error: " << id_error << std::endl;
          is_ok = false;
        };

        //////////
        // more cases to be considered...
        if ( key.find("*")!=std::string::npos) {
          subscriber.punsubscribe(key,got_error);
          is_ok = false;
        }
        else {
          subscriber.unsubscribe(key,got_error);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          // test not subscribed to topic anymore
          for( auto& s : subscriber.subscribedTopics())
            if( s == key )
              is_ok = false;
        }
        
        return is_ok;
      }
      
      bool Unsubscribe(const std::string& key,
                       std::function<void(const std::string&,const int&)> got_error
                       ) override {
        bool is_ok = true;
        std::size_t found = key.find("*");

        if ( found != std::string::npos) {
          std::string short_key(key);
          short_key.pop_back();
          auto list = subscriber.psubscribedTopics();
          if ( list.find(short_key) == list.end() )
            return false;
          subscriber.punsubscribe(short_key.substr(0,found-1),got_error);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          list = subscriber.psubscribedTopics();
          if ( list.find(short_key) == list.end() )
            return false;

        }
        else {
          subscriber.unsubscribe(key,got_error);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          for( auto& s : subscriber.subscribedTopics())
            if( s == key )
              is_ok = false;
        }

        return is_ok;
      }

      std::set<std::string> ListTopics() {
        std::set<std::string> s = subscriber.subscribedTopics();
        s.insert(subscriber.psubscribedTopics().begin(),
                 subscriber.psubscribedTopics().end()
                 );
        return s;
      }

      int NumMessages() const { return updates.size(); }
      int NumRecvMessages() const { return total_recv_messages; }
      bool keep_counting = true;
    private:
      redox::Redox publisher;
      redox::Subscriber subscriber;
      
      std::ostream& log;
      unsigned long int total_num_messages = 0;
      unsigned long int total_recv_messages = 0;

      std::chrono::system_clock::time_point last_notify_time;
      std::thread t;
      
      void TimedNotification() {
        while(this->keep_counting) {
          std::this_thread::sleep_for(
                                      std::chrono::seconds(NotificationTimeout));
          // std::this_thread::sleep_until(last_notify_time+
          //                               std::chrono::seconds(NotificationTimeout));
          log << NotificationTimeout << "s elapsed, auto-notification will occur\n";
          //          log << "value of keep counting = " << keep_counting << "\n";
          Notify();
        }
        log << "TimedNotification terminated\n";
      }
      
    };
    int RedisCommunicator::MaxStoredMessages = 100;
    int RedisCommunicator::NotificationTimeout = 1;
    
  }
}
  
