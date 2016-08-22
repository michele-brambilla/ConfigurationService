#pragma once

#include <iostream>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>


namespace configuration {

struct MockManager {
typedef int handler;
};

  
  template<typename DataManager, typename CommunicationManager>
  struct ConfigurationService {

    ConfigurationService(typename DataManager::handler& _dh,
                         typename CommunicationManager::handler& _ch) :
      dh(_dh), ch(_ch) { }

    bool UploadConfig(std::string& config) {
      if( !is_valid_config(config) ) return false;

      return false
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
    
    std::ostream& DumpConfig(std::istream& os) {
      static std::ostream out(nullptr);
      return out;
    }

    bool SubscribeToKey(const std::string& key) { return false; };

    std::string const GetKeyValue(const std::string& key) {
      return std::string("");
    }

    std::string const UpdateKeyValue(const std::string& key) {
      return std::string("");
    }

  private:
    typename DataManager::handler& dh;
    typename CommunicationManager::handler& ch;
    rapidjson::Document d;
    
    bool is_valid_config(std::string config) { // tested
      return !d.Parse(config.c_str()).HasParseError();
    }
  };


}
