#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <future>
#include <set>

#include <basic_communicator.hpp>
#include <basic_data.hpp>

namespace redox {
  class Redox;
  class Subscriber;
}

namespace configuration {


  namespace data {
    
    /////////////////////
    // Redis data manager
    struct RedisDataManager  : public DataManager
    {
      typedef RedisDataManager self_t;
      int connection_status;
      
      RedisDataManager(const std::string&,
                       const int&,
                       container::MultimapContainer&,
                       std::ostream&);
      
      ~RedisDataManager();

      void Disconnect();

      void Dump(std::ostream& = std::cout);
      
      void Clear() override;

      redox::Redox& redox();

    private:

      std::string address;
      int port;
      std::shared_ptr<redox::Redox> rdx;
      std::ostream& log;
      container::MultimapContainer& updates;

      void Reconnect();
      
      bool KeyExists (const std::string&) override;

      bool HExists(const std::string&,const std::string&) override;

      bool SIsMember(const std::string&,const std::string&);

      bool IsHash(const std::string&) override;

      bool AddToHash(const std::string&,
                     const std::vector<std::string>&) override;
      
      bool AddToHash(const std::string&, const std::string&) override;
      
      bool RemoveFromParent(const std::string&,const std::string&) override;

      int RemoveChildren(const std::string&) override;

      bool RemoveKey(const std::string&) override;
      
      bool RemoveHash(const std::string&,const std::string&) override;
      
      bool UpdateHashValue(const std::string&,const std::string&) override;

      bool AddToParent(const std::string&,const std::string&) override;

      bool UpdateParent(const std::string&) override;
      
      std::vector<std::string> ReturnValue(const std::string&) override;
    
      bool json_scan(const std::string& ) override;

      template<typename Iterator>
      bool json_scan_impl(Iterator&,std::string);

    };

  }




  namespace communicator {

    struct RedisCommunicator : public Communicator {
      static int MaxStoredMessages;
      int publisher_connection_status;
      int subscriber_connection_status;
      
      RedisCommunicator(const std::string&,
                        const int& =6379,
                        std::ostream& =std::cerr,
                        int =5);

      ~RedisCommunicator();

      void Disconnect();
      
      bool Notify() override;
      
      bool Subscribe(const std::string&);
      
      bool Subscribe(const std::string&,
                     std::function<void(const std::string&,const std::string&)>,
                     std::function<void(const std::string&,const int&)> = default_got_error,
                     std::function<void(const std::string&)> = default_unsubscribed
                     ) override;
      
      bool Unsubscribe(const std::string&,
                       std::function<void(const std::string&,const int&)> = default_got_error
                       ) override;

      std::set<std::string> ListTopics();
      
      bool Reconnect();
      
      bool keep_counting = true;
      static int NotificationTimeout;
      
    private:
      std::shared_ptr<redox::Redox> publisher;
      std::shared_ptr<redox::Subscriber> subscriber;
      
      std::ostream& log;

      std::string redis_server;
      int redis_port;
      std::thread t;
      
      void AutoNotification() {
        while(this->keep_counting) {
          std::this_thread::sleep_for(std::chrono::seconds(NotificationTimeout));
          log << NotificationTimeout << "s elapsed, auto-notification will occur\n";
          Notify();
        }
        log << "TimedNotification terminated\n";
      }

      
      void count_got_message(const std::string & t,const std::string & c)  {
        std::cerr << "== "<< t << " : " << c <<" ==" << std::endl;
        this->total_recv_messages++;
      } 
      
    };
    
  }
}
  
