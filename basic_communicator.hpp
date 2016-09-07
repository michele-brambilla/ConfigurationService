#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <functional>

namespace configuration {
  namespace communicator {

    ////////////////
    // Base class for message receive callbacks
    struct ReceivedMessageCb {
      ReceivedMessageCb() : num_msg(0) {
        f = [this](std::string const & x, std::string const & y) {
          this->got_message(x, y);
        };
      };
      
      std::function<void(std::string const &, std::string const &)> f;
      virtual void got_message(const std::string & t,const std::string & c)  {
        std::cout << "method: "<< t << "\t" << c << std::endl;
        std::cout << "message number: "<< num_msg << std::endl;
        num_msg++;
      }
      int num_msg;
    };
    
    ////////////////
    // Base class for unsubscribe callbacks
    struct UnsubscribeCb {
      UnsubscribeCb() {
        f = [this](std::string const & x) {
          this->unsubscribed(x);
        };
      };      
      std::function<void(std::string const &)> f;
      virtual void unsubscribed(const std::string & t)  {
        std::cout << "method: "<< t << std::endl;
      }

    };


    struct SubscribeErrorCb {
      SubscribeErrorCb() {
        f = [this](std::string const & x, int const & y) {
          this->got_error(x, y);
        };
      };
      
      std::function<void(std::string const &, int const &)> f;
      virtual void got_error(const std::string & t,const int & v)  {
        std::cout << "method: "<< t << "\t" << v << std::endl;
      }
    };

    
    struct Communicator {

      //////////////
      /// Informs that key has changed its value according to status: 
      /// - u: value or content has been updated
      /// - a: key is a brand new key
      /// - d: the key has been deleted
      virtual bool Publish(const std::string&,const std::string&) { return false; }

      /////////////
      /// Sends all the messages stored since last notify
      virtual bool Notify() { return false; }

      virtual bool Subscribe(const std::string&) { return false; }

      // callback on message received
      virtual bool Subscribe(const std::string&,
                             std::function< void(const std::string &,const std::string &) >
                             ) { return false; }

      // callback on message receive, unsubscription, subscription error
      virtual bool Subscribe(const std::string&,
                             std::function< void(const std::string &,const std::string &) >,
                             std::function< void(const std::string & ) >,
                             std::function< void(const std::string &,const int &) >
                             ) { return false; }

    protected:
      std::map<std::string,std::string> updates;

    };

  }
}
  
