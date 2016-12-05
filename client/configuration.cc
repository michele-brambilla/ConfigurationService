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
    //    configuration_manager = make_unique<ConfigurationManager<D,C> >(address,port,);
    //ConfigurationManager<D,C> config(address,port);
    return 0;
  }

}
