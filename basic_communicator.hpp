#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <functional>
#include <thread>
#include <map>
#include <future>

namespace configuration {
  namespace communicator {


    inline void default_got_message(const std::string & t,const std::string & c)  {
      std::cerr << "== "<< t << " : " << c <<" ==" << std::endl;
    }    
    inline void default_got_error(const std::string & t,const int & v)  {
      std::cerr << "== "<< t << " : " << v << " ==" << std::endl;
    }
    inline void default_unsubscribed(const std::string & t)  {
      std::cerr << "== "<< t << " ==" << std::endl;
    }
    
    
    ////////////////
    // Base class for message receive callbacks
    struct ReceivedMessageCb {
      ReceivedMessageCb() : num_msg(0) { };
      virtual void got_message(const std::string & t,const std::string & c)  {
        std::cout << "== "<< t << " : " << c <<" ==" << std::endl;
        std::cout << "message number: "<< num_msg << std::endl;
        num_msg++;
      }
      int num_msg;
    };
    
    ////////////////
    // Base class for unsubscribe callbacks
    struct UnsubscribeCb {
      UnsubscribeCb() {  };      
      virtual void unsubscribed(const std::string & t)  {
        std::cout << "== "<< t << " ==" << std::endl;
      }

    };


    struct SubscribeErrorCb {
      SubscribeErrorCb() { };
      
      virtual void got_error(const std::string & t,const int & v)  {
        std::cout << "== "<< t << " : " << v << " ==" << std::endl;
      }
    };

    
    struct Communicator {
      static int MaxStoredMessages;
      static int NotificationTimeout;
      Communicator(std::ostream& logger=std::cerr) : log(logger) { }

      //////////////
      /// Informs that key has changed its value according to status: 
      /// - u: value or content has been updated
      /// - a: key is a brand new key
      /// - d: the key has been deleted
      bool Publish(const std::string& key,const std::string& status) {
        updates.insert(std::pair<std::string,std::string>(key,status) );
        if(updates.size() > MaxStoredMessages) {
          log << "Max number of stored messages ("+std::to_string(MaxStoredMessages)+") reached, proceed to notify"
              << std::endl;
          return Notify();
        }
        total_num_messages++;
        return true;
      }
      
      /////////////
      /// Sends all the messages stored since last notify
      virtual bool Notify() { return false; }

      virtual bool Subscribe(const std::string&,
                             std::function< void(const std::string &,const std::string &) > = default_got_message,
                             std::function< void(const std::string &,const int &) > = default_got_error,         // got error
                             std::function< void(const std::string & ) > = default_unsubscribed                    // unsubscribe
                             ) { return false; }

      // virtual bool Unsubscribe(const std::string&) { return false; }
      virtual bool Unsubscribe(const std::string&,std::function< void(const std::string &,const int &) >) { return false; }

      const int NumMessages() const { return updates.size(); }
      const int NumRecvMessages() const { return total_recv_messages; }
      
    protected:
      std::multimap<std::string,std::string> updates;
      unsigned long int total_num_messages = 0;
      unsigned long int total_recv_messages = 0;

      std::ostream& log;


      void count_got_message(const std::string & t,const std::string & c, unsigned long int& n)  {
        std::cerr << "== "<< t << " : " << c <<" ==" << std::endl;
        this->total_recv_messages++;
      }
      
    };




    struct MockCommunicator : public Communicator {

      MockCommunicator(const std::string&, const int&) { };
      
    };

  }
}
  
