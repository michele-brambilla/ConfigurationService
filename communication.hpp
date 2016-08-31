#pragma once

#include <iostream>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>


namespace configuration {

  namespace comm {

    struct CommunicationManager {
      CommunicationManager() { };
      bool NotifyNewConfig(std::string config) { return false; }
      bool GetConfig(const std::string config_name,
                     std::string& config) {
        return false;
      }
      
    private:
      bool NotifyFailure() const { return true; }
    };

    struct MockCommunicationManager : CommunicationManager {
      MockCommunicationManager() { };
      bool NotifyNewConfig(std::string config) { return false; }
      bool GetConfig(const std::string config_name,
                     std::string& config) {
        return false;
      }
      
    private:
      bool NotifyFailure() const { return true; }
    };

  
  }
}
