#include <configuration.hpp>

namespace configuration {

  
  template<typename DataManager,typename CommunicationManager>
  void ConfigurationManager<DataManager,CommunicationManager>::AddConfig(const std::string& conf) {
    bool success = (*this).dm->AddConfig(conf);
    if(!success)
      log << "Error: can't add config " << conf << std::endl;
  }

  template<typename DataManager,typename CommunicationManager>
  void ConfigurationManager<DataManager,CommunicationManager>::AddConfig(std::ifstream& in) {
    std::string config,buf;
    while(!in.eof()) {
      std::getline(in, buf,'\t');
      config += buf;
    }
    in.close();
    bool success = dm->AddConfig(config);
    if(!success)
      log << "Error: can't add config from file " << std::endl;
  }

  template<typename DataManager,typename CommunicationManager>
  void ConfigurationManager<DataManager,CommunicationManager>::Update(const std::string& key, const std::string& value) {
    bool success = dm->Update(key,value);
    if(!success)
      log << "Error: can't update " << key << std::endl;
  }
  
  template<typename DataManager,typename CommunicationManager>
  void ConfigurationManager<DataManager,CommunicationManager>::Delete(const std::string& key) {
    bool success = dm->Delete(key);
    if(!success)
      log << "Error: can't delete " << key << std::endl;
  }
  
  template<typename DataManager,typename CommunicationManager>
  void ConfigurationManager<DataManager,CommunicationManager>::Notify() {
    bool success = cm->Notify();
    if(!success)
      log << "Error: can't notify " << std::endl;
  }


  ////////////////////////////
  // Function calls
  std::unique_ptr<ConfigurationManager<D,C> > configuration_manager;
  
  int Init(const char* address="localhost",
           const int& port=6379) {
    std::cout << "creating configuration manager instance" << std::endl;
    configuration_manager = make_unique<ConfigurationManager<D,C> >(address,port);
    //ConfigurationManager<D,C> config(address,port);
    return 0;
  }

}
