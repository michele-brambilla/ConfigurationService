#pragma once

#include <iostream>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>


namespace configuration {

  struct MockManager {
    typedef int handler;
    MockManager() { };

    std::string AddConfig(std::string config) {
      return std::string("");
    }

    bool NotifyNewConfig(std::string config) {
      return false;
    }
    
  };


  struct MockDataManager : MockManager {
    MockDataManager() { };
    std::string AddConfig(std::string config) { return std::string(""); }
    
  private:

    bool ConfigAlreadyPresent() const { return true; }
  };

  
  struct MockCommunicationManager : MockManager {
    MockCommunicationManager() { };
    bool NotifyNewConfig(std::string config) { return false; }
    bool GetConfig(const std::string config_name,
                   std::string& config) {
      return false;
    }
    
  private:
    bool NotifyFailure() const { return true; }
  };

  
  /////////////////////////////////////
  /// struct ConfigurationService
  template<typename DataManager, typename CommunicationManager>
  struct ConfigurationService {

    ConfigurationService(DataManager& _dh,
                         CommunicationManager& _ch) :
      dh(_dh), ch(_ch) { }

    bool UploadConfig(std::string& config) {
      if( !is_valid_config(config) ) return false;

      std::string config_name = dh.AddConfig(config);
      if ( !config_name.empty() ) return false;
      if ( !ch.NotifyNewConfig(config_name) ) return false;
      
      return false;
    };

    
    bool UploadConfig(std::ifstream& is) { // tested
      std::string config,buf;
      while(!is.eof()) {
        std::getline(is, buf,'\t');
        config += buf;
      }
      is.close();
      
      return UploadConfig(config);
    };
    
    std::ostream& DumpConfig(std::ostream& os = std::cout) {
      static std::ostream out(nullptr);
      return os;
    }

    bool SubscribeToKey(const std::string& configmore,
                        const std::string& key) { return false; };

    std::string const GetKeyValue(const std::string& key) {
      return std::string("");
    }

    std::string const UpdateKeyValue(const std::string& key) {
      return std::string("");
    }

  private:
    DataManager& dh;
    CommunicationManager& ch;
    rapidjson::Document d;
    
    bool is_valid_config(std::string config) { // tested
      return !d.Parse(config.c_str()).HasParseError();
    }
  };


}
