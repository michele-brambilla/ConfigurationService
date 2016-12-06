#include <configuration.hpp>

  
void configuration::default_got_message(const std::string & t,const std::string & c)  {
  std::cerr << "Using function callback > "<< t << "\t" << c << std::endl;
}
void configuration::default_got_error(const std::string & t,const int & v)  {
  std::cerr << "Error function callback: > "<< t << "\t->\t" << v << std::endl;
}
void configuration::default_unsubscribed(const std::string & t)  {
  std::cerr << "Unsubscribe function callback < "<< t << std::endl;
}


namespace configuration {

  ////////////////////////////
  // Function calls
  std::unique_ptr<ConfigurationManager<D,C> > configuration_manager;
  
  int Init(const char* address="localhost",
           const int& port=6379) {
    std::cout << "creating configuration manager instance" << std::endl;
    configuration_manager = make_unique<ConfigurationManager<D,C> >(address,port);
    return 0;
  }

  int Add(const std::string& conf) {
    return configuration_manager->AddConfig(conf);
  }
  
  std::vector<std::string> Query(const std::string& key) {
    return configuration_manager->Query(key);
  }
  
  bool Update(const std::string& key, const std::string& value) {
    return  configuration_manager->Update(key,value);
  }
  
  bool Delete(const std::string& key) {
    return configuration_manager->Delete(key);
  }
  
  bool Subscribe(const std::string& key,
                 std::function<void(const std::string&,const std::string&)> got_message=default_got_message,
                 std::function<void(const std::string&,const int&)> got_error=default_got_error,
                 std::function<void(const std::string&)> unsubscribed=default_unsubscribed
                 ) {
    return configuration_manager->Subscribe(key,got_message,got_error,unsubscribed);
  }

  
}
