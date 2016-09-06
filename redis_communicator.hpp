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

    
    struct RedisCommunicator : protected Communicator {
      static const int MaxStoredMessages = 10000;
      
      RedisCommunicator(const std::string& redis_server,
                        const int& redis_port,
                        std::ostream& logger=std::cerr,
                        redox::log::Level redox_loglevel=redox::log::Level::Fatal) 
        : publisher(logger,redox_loglevel), subscriber(logger,redox_loglevel), log(logger) {
        if( !publisher.connect(redis_server, redis_port) ) {
          log << "Publisher can't connect to REDIS\n";
          throw std::runtime_error("Can't connect to REDIS server");
        }
        if( !subscriber.connect(redis_server, redis_port) ) {
          log << "Subscriber can't connect to REDIS\n";
          throw std::runtime_error("Can't connect to REDIS server");
        }
      }
      
      bool operator()(const std::string& key,const std::string& status) override {
        updates[key]=status;
        if(updates.size() > MaxStoredMessages) {
          log << "Max number of stored messages ("+std::to_string(MaxStoredMessages)+") reached, proceed to notify"
              << std::endl;
          return Notify();
        }
        total_num_messages++;
        return true;
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
        updates.clear();
        return is_ok;
      }

      bool Subscribe(const std::string& key) override {
        std::vector<std::string> result;
        bool is_ok = true;

        auto got_message = [](const std::string& topic, const std::string& msg) {
          std::cout << topic << ": " << msg << std::endl;
        };
        
        auto  subscribed = [](const std::string& topic) {
          std::cout << "> Subscribed to " << topic << std::endl;
        };
        
        auto unsubscribed = [](const std::string& topic) {
          std::cout << "> Unsubscribed from " << topic << std::endl;
        };
        
        subscriber.psubscribe(key+"*", got_message, subscribed);

        return false;
      }

      bool Subscribe(const std::string& key, std::function< void(const std::string &)> cb) override {
        std::vector<std::string> result;
        bool is_ok = true;
        
        auto got_message = [](const std::string& topic, const std::string& msg) {
          std::cout << topic << ": " << msg << std::endl;
        };
        
        auto  subscribed = [](const std::string& topic) {
          std::cout << "> Subscribed to " << topic << std::endl;
        };
        
        auto unsubscribed = [](const std::string& topic) {
          std::cout << "> Unsubscribed from " << topic << std::endl;
        };
        
        subscriber.psubscribe(key+"*", got_message, subscribed);
        
        return false;
      }

      
      int NumMessages() const { return updates.size(); }
      
    private:
      redox::Redox publisher;
      redox::Subscriber subscriber;

      std::ostream& log;
      unsigned long int total_num_messages = 0;

      
    };

  }
}
  
