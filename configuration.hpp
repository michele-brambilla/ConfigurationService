#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <utility>

#include <redis_configuration.hpp>

using C=configuration::communicator::RedisCommunicator;
using D=configuration::data::RedisDataManager<C>;


#if __cplusplus == 201402L // C++14

using std::make_unique ;

#else // C++11

template < typename T, typename... CONSTRUCTOR_ARGS >
std::unique_ptr<T> make_unique( CONSTRUCTOR_ARGS&&... constructor_args )
{ return std::unique_ptr<T>( new T( std::forward<CONSTRUCTOR_ARGS>(constructor_args)... ) ); }

#endif // __cplusplus == 201402L

namespace configuration {

  static const int NOT_YET_CONNECTED = 0; // Starting state
  static const int CONNECTED = 1;         // Successfully connected
  static const int DISCONNECTED = 2;      // Successfully disconnected
  static const int CONNECT_ERROR = 3;     // Error connecting
  static const int DISCONNECT_ERROR = 4;  // Disconnected on error
  static const int INIT_ERROR = 5;        // Failed to init data structures
  
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
      dm = make_unique<DataManager>(data_server,data_port,*cm);
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
      bool success = dm->AddConfig(conf);
      if(!success)
        log << "Error: can't add config " << std::endl;
      return success;
    }

    std::vector<std::string> Query(const std::string& key) {
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
      return false;
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


  // std::unique_ptr<ConfigurationManager<D,C> > configuration_manager;
  int Init(const char*, const int&);
  // int Init(const char* address="localhost",
  //          const int& port=6379) {
  //   std::cout << "creating configuration manager instance" << std::endl;
  //   configuration_manager = make_unique<ConfigurationManager<D,C> >(address,port);
  //   //ConfigurationManager<D,C> config(address,port);
  //   return 0;
  // }

  



  

}


// namespace configuration {

//   struct MockManager {
//     typedef int handler;
//     MockManager() { };

//     std::string AddConfig(std::string config) {
//       return std::string("");
//     }

//     bool NotifyNewConfig(std::string config) {
//       return false;
//     }
    
//   };


//   struct MockDataManager : MockManager {
//     MockDataManager() { };
//     std::string AddConfig(std::string config) { return std::string(""); }
    
//   private:

//     bool ConfigAlreadyPresent() const { return true; }
//   };

  
//   struct MockCommunicationManager : MockManager {
//     MockCommunicationManager() { };
//     bool NotifyNewConfig(std::string config) { return false; }
//     bool GetConfig(const std::string config_name,
//                    std::string& config) {
//       return false;
//     }
    
//   private:
//     bool NotifyFailure() const { return true; }
//   };

  
//   /////////////////////////////////////
//   /// struct ConfigurationService
//   template<typename DataManager, typename CommunicationManager>
//   struct ConfigurationService {

//     ConfigurationService(DataManager& _dh,
//                          CommunicationManager& _ch) :
//       dh(_dh), ch(_ch) { }

//     bool UploadConfig(std::string& config) {
//       if( !is_valid_config(config) ) return false;

//       std::string config_name = dh.AddConfig(config);
//       if ( !config_name.empty() ) return false;
//       if ( !ch.NotifyNewConfig(config_name) ) return false;
      
//       return false;
//     };

    
//     bool UploadConfig(std::ifstream& is) { // tested
//       std::string config,buf;
//       while(!is.eof()) {
//         std::getline(is, buf,'\t');
//         config += buf;
//       }
//       is.close();
      
//       return UploadConfig(config);
//     };
    
//     std::ostream& DumpConfig(std::ostream& os = std::cout) {
//       static std::ostream out(nullptr);
//       return os;
//     }

//     bool SubscribeToKey(const std::string& configmore,
//                         const std::string& key) { return false; };

//     std::string const GetKeyValue(const std::string& key) {
//       return std::string("");
//     }

//     std::string const UpdateKeyValue(const std::string& key) {
//       return std::string("");
//     }

//   private:
//     DataManager& dh;
//     CommunicationManager& ch;
//     rapidjson::Document d;
    
//     bool is_valid_config(std::string config) { // tested
//       return !d.Parse(config.c_str()).HasParseError();
//     }
//   };


// }
