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
      static const int MaxStoredMessages = 10000;
      
      RedisCommunicator(const std::string& redis_server,
                        const int& redis_port,
                        std::ostream& logger=std::cerr,
                        redox::log::Level redox_loglevel=redox::log::Level::Fatal) 
        : publisher(logger,redox_loglevel), log(logger) {
        if( !publisher.connect(redis_server, redis_port) ) {
          log << "Publisher can't connect to REDIS\n";
          throw std::runtime_error("Can't connect to REDIS server");
        }
        if( !subscriber.connect(redis_server, redis_port) ) {
          log << "Subscriber can't connect to REDIS\n";
          throw std::runtime_error("Can't connect to REDIS server");
        }
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
                     std::function<void(const std::string&,const std::string&)> f_got) override {

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
          subscriber.psubscribe(key, f_got, subscribed, unsubscribed, got_error);
        else
          subscriber.subscribe(key, f_got, subscribed, unsubscribed, got_error);
        
        return is_ok;
      }

      
      
      int NumMessages() const { return updates.size(); }
      int NumRecvMessages() const { return total_recv_messages; }
      
    private:
      redox::Redox publisher;
      redox::Subscriber subscriber;

      std::ostream& log;
      unsigned long int total_num_messages = 0;
      unsigned long int total_recv_messages = 0;

    };

  }
}
  
