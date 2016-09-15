#include <redis_utils.hpp>


namespace configuration {

  namespace utils {
    // struct typelist
    // {
    //   typedef int SADD_t; //reply of type 3
    //   typedef int DEL_t; //reply of type 3

    //   typedef std::string TYPE_t; //reply of type 2

    //   typedef std::string GET_t; //reply of type 2
    //   typedef std::string SET_t; //reply of type 2
    //   typedef std::string VALUE_t; //reply of type 2
      
    //   typedef std::vector<std::string> KEYS_t; //reply of type 1 (or 5?)
    //   typedef std::vector<std::string> LIST_t; //output of ExecRedisCmd
      
    // };
    
    
    // template<typename Output>
    // bool ExecRedisCmd(redox::Redox& r, const std::string& s) {
    //   auto& c = r.commandSync<Output>(redox::Redox::strToVec(s));
    //   if( !c.ok() ) {
    //     std::cerr << "Error: " << c.lastError() << std::endl;
    //     return false;
    //   }
    //   return true;
    // }
    // template<typename Output>
    // bool ExecRedisCmd(redox::Redox& r, const std::string& s, Output& out) {
    //   auto& c = r.commandSync<Output>(redox::Redox::strToVec(s));
    //   if( !c.ok() ) {
    //     std::cerr << "Error: " << c.lastError() << std::endl;
    //     return false;
    //   }
    //   out = c.reply();
    //   //      std::cout << "Reply.size = " << out.size()  << std::endl;
    //   return true;
    // }

    // template<typename Output>
    // bool ExecRedisCmd(redox::Redox& r, const std::string& s, Output* out) {
    //   auto& c = r.commandSync<Output>(redox::Redox::strToVec(s));
    //   if( !c.ok() ) {
    //     std::cerr << "Error: " << c.lastError() << std::endl;
    //     return false;
    //   }
    //   (*out) = c.reply();
    //   //      std::cout << "Reply.size = " << out.size()  << std::endl;
    //   return true;
    // }


    bool TryConnect(redox::Redox& r,
                    const std::string& redis_server,
                    const int& redis_port,
                    const int& Retry) {
      bool IsConnected = false;
      int counter = 0;
      IsConnected = r.connect(redis_server, redis_port);
      std::this_thread::sleep_for(std::chrono::seconds(1));

      // if( !IsConnected) {
      //   std::cout << "Connection failed, retry " << ++counter << std::endl;
      //   IsConnected = r.connect(redis_server, redis_port);
      //   std::this_thread::sleep_for(std::chrono::seconds(1));
      //   if( !IsConnected) {
      //     std::cout << "Connection failed, retry " << ++counter << std::endl;
      //     IsConnected = r.connect(redis_server, redis_port);
      //     std::this_thread::sleep_for(std::chrono::seconds(1));
                
      //     if( !IsConnected) {
      //       std::cout << "Connection failed, retry " << ++counter << std::endl;
      //       IsConnected = r.connect(redis_server, redis_port);
      //       std::this_thread::sleep_for(std::chrono::seconds(1));
                  
      //     }
      //   }
      // }
      // while( !IsConnected && counter < 10 ) {
      //   std::cout << "Connection failed, retry " << ++counter << std::endl;
      //   std::this_thread::sleep_for(std::chrono::seconds(1));
      //   IsConnected = r.connect(redis_server, redis_port);
      // }
      return IsConnected;
    }



    void redis_connection_callback(int status, int& output) {
      if( status == redox::Redox::CONNECTED)
        std::cerr << "Connected to redis" << status << std::endl;
      else {
        if( status == redox::Redox::DISCONNECTED)
          std::cerr << "Disconnected from redis" << std::endl;
        else {
          std::cerr << "Connection error: " << status << std::endl;
        }
      }
      output = status;
      return;
    }

    
  } // namespace utils
  

}

