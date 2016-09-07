#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <functional>

namespace configuration {
  namespace communicator {

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
  
