#pragma once

#include <iostream>
#include <fstream>
#include <ctime>

namespace configuration {

  namespace communicator {


    struct MockCommunicator {
      bool Send(std::vector<std::pair<std::string,std::string> >& updates) {
        std::cout << "Configuration changes:\n";
        while (!updates.empty()) {
          std::cout << "\t("     << updates.back().second
                    << ")\t" << updates.back().first
                    << std::endl;
          updates.pop_back();
        }
        return true;
      }
    };
    
    struct FileCommunicator {
      FileCommunicator(const std::string& ofname="notification.changes") {
        of.exceptions ( std::ofstream::failbit | std::ofstream::badbit );
        try {
          of.open (ofname,std::ofstream::app);
        }
        catch (std::ifstream::failure e) {
          std::cerr << "Exception opening/reading/closing file\n";
        }

      }
      ~FileCommunicator() {
        of.close();
      }
      
      bool Send(std::vector<std::pair<std::string,std::string> >& updates) {
        std::time_t result = std::time(nullptr);
        of << std::asctime(std::localtime(&result)) // << std::endl
          ;
        while (!updates.empty()) {
          of << "\t("     << updates.back().second
             << ")\t" << updates.back().first
             << std::endl;
          updates.pop_back();
        }
        of << std::endl;
        return true;
      }
      
      private:
      std::ofstream of;
    };
    
    // struct CommunicationManager {
    //   CommunicationManager() { };
    //   bool NotifyNewConfig(std::string config) { return false; }
    //   bool GetConfig(const std::string config_name,
    //                  std::string& config) {
    //     return false;
    //   }
      
    // private:
    //   bool NotifyFailure() const { return true; }
    // };

    // struct MockCommunicationManager : CommunicationManager {
    //   MockCommunicationManager() { };
    //   bool NotifyNewConfig(std::string config) { return false; }
    //   bool GetConfig(const std::string config_name,
    //                  std::string& config) {
    //     return false;
    //   }
      
    // private:
    //   bool NotifyFailure() const { return true; }
    // };

  
  }
}
