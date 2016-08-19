#pragma once

#include <iostream>
#include <fstream>


namespace configuration {

  struct MockManager {
    typedef int handler;
  };

  
  template<typename DataManager, typename CommunicationManager>
  struct ConfigurationService {

    ConfigurationService(typename DataManager::handler& _dh,
                         typename CommunicationManager::handler& _ch) :
      dh(_dh), ch(_ch) { }

    bool UploadConfig(const std::string& config) { return false; };
    bool UploadConfig(const std::ostream& os) { return false; };

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
  };


}
