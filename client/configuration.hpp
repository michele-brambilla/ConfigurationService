#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <utility>

#include <redis_configuration.hpp>
#include <zmq_configuration.hpp>

using C=configuration::communicator::RedisCommunicator;
using D=configuration::data::RedisDataManager;


#if __cplusplus == 201402L // C++14

using std::make_unique ;

#else // C++11

template < typename T, typename... CONSTRUCTOR_ARGS >
std::unique_ptr<T> make_unique( CONSTRUCTOR_ARGS&&... constructor_args )
{ return std::unique_ptr<T>( new T( std::forward<CONSTRUCTOR_ARGS>(constructor_args)... ) ); }

#endif // __cplusplus == 201402L

namespace configuration {

  enum { NOT_YET_CONNECTED = 0, // Starting state
         CONNECTED = 1,         // Successfully connected
         DISCONNECTED = 2,      // Successfully disconnected
         CONNECT_ERROR = 3,     // Error connecting
         DISCONNECT_ERROR = 4,  // Disconnected on error
         INIT_ERROR = 5         // Failed to init data structures
  };
  
  void default_got_message(const std::string&,const std::string&);
  void default_got_error(const std::string &,const int &);
  void default_unsubscribed(const std::string &);
  
  
  template<typename DataManager, typename CommunicationManager>
  struct ConfigurationManager {
    //    ConfigurationManager() : cm(nullptr), dm(nullptr) { };
    
    ConfigurationManager(const char* data_server="localhost",
                         const int& data_port=6379,
                         const char* comm_server="localhost",
                         const int& comm_port=6379,
                         std::ostream& logger=std::cerr) : log(logger) {
      cm = make_unique<CommunicationManager>(comm_server,comm_port);
      dm = make_unique<DataManager>(data_server,data_port,cm->updates,log);
      log << "CommunicationManager addr = " << cm.get() << std::endl;
      log << "DataManager addr = " << dm.get() << std::endl;
    }

    bool Disconnect() {
      return ( (dm->connection_status            != DISCONNECTED) ||
               (cm->publisher_connection_status  != DISCONNECTED) ||
               (cm->subscriber_connection_status != DISCONNECTED) );

    }

    bool ConnectionStatus() {
      return ( (dm->connection_status != CONNECTED) || (dm->connection_status != DISCONNECTED) ||
               (cm->publisher_connection_status != CONNECTED) || (cm->publisher_connection_status != DISCONNECTED) ||
               (cm->subscriber_connection_status != CONNECTED) || (cm->subscriber_connection_status != DISCONNECTED) );
    }

    
    bool AddConfig(const std::string& conf) {
      std::cout << "AddConfig:\n" << conf << std::endl;
      bool success = dm->AddConfig(conf);
      if(!success)
        log << "Error: can't add config " << std::endl;
      return success;
    }

    std::vector<std::string> Query(const std::string& key) {
      if(key == "*") {
        dm->Dump();
        return std::vector<std::string>();
      }
      
      std::vector<std::string> data = dm->Query(key);
      if(data.size() == 0)
        log << "Warning: empty key (" << key << ")" << std::endl;
      return data;
    }
    
    bool Update(const std::string& key, const std::string& value) {
      bool success = dm->Update(key,value);
      if(!success)
        log << "Error: can't update " << key << std::endl;
      return success;
    }
      
    bool Delete(const std::string& key) {
      bool success = dm->Delete(key);
      if(!success)
        log << "Error: can't delete " << key << std::endl;
      return success;
    }

    bool Notify() {
      bool success = cm->Notify();
      if(!success)
        log << "Error: can't notify " << std::endl;
      return success;
    }

    bool Subscribe(const std::string& key,
                     std::function<void(const std::string&,const std::string&)> got_message=default_got_message,
                     std::function<void(const std::string&,const int&)> got_error=default_got_error,
                     std::function<void(const std::string&)> unsubscribed=default_unsubscribed
                     ) {
      return cm->Subscribe(key,got_message,got_error,unsubscribed);
    }
    
    const int& DataConnectionStatus() { return dm->connection_status; };
    const int& PublisherConnectionStatus() { return cm->publisher_connection_status; };
    const int& SubscriberConnectionStatus() { return cm->subscriber_connection_status; };
    
  private:
    std::unique_ptr<CommunicationManager> cm;
    std::unique_ptr<DataManager> dm;
    std::ostream& log;

  };
  

}

