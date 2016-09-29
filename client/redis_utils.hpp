#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <chrono>

#include <redox.hpp>



namespace configuration {

  namespace utils {
    struct typelist
    {
      typedef int SADD_t; //reply of type 3
      typedef int DEL_t; //reply of type 3

      typedef std::string TYPE_t; //reply of type 2

      typedef std::string GET_t; //reply of type 2
      typedef std::string SET_t; //reply of type 2
      typedef std::string VALUE_t; //reply of type 2
      
      typedef std::vector<std::string> KEYS_t; //reply of type 1 (or 5?)
      typedef std::vector<std::string> LIST_t; //output of ExecRedisCmd
      
    };
    
    
    template<typename Output>
    bool ExecRedisCmd(redox::Redox& r, const std::string& s) {
      auto& c = r.commandSync<Output>(redox::Redox::strToVec(s));
      if( !c.ok() ) {
        std::cerr << "Error: " << c.lastError() << std::endl;
        return false;
      }
      return true;
    }
    template<typename Output>
    bool ExecRedisCmd(redox::Redox& r, const std::string& s, Output& out) {
      auto& c = r.commandSync<Output>(redox::Redox::strToVec(s));
      if( !c.ok() ) {
        std::cerr << "Error: " << c.lastError() << std::endl;
        return false;
      }
      out = c.reply();
      //      std::cout << "Reply.size = " << out.size()  << std::endl;
      return true;
    }

    template<typename Output>
    bool ExecRedisCmd(redox::Redox& r, const std::string& s, Output* out) {
      auto& c = r.commandSync<Output>(redox::Redox::strToVec(s));
      if( !c.ok() ) {
        std::cerr << "Error: " << c.lastError() << std::endl;
        return false;
      }
      (*out) = c.reply();
      //      std::cout << "Reply.size = " << out.size()  << std::endl;
      return true;
    }


    
    bool TryConnect(redox::Redox&,
                    const std::string&,
                    const int&,
                    const int& Retry = 10);

    void redis_connection_callback(int,int&);
    void redis_connection_advanced(int,const bool&,std::function<void()>);
    
  } // namespace utils
  
    
}

