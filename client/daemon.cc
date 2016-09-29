#include<iostream>





int main() {

  try {
    
    
    
    
    
    
    
    
    
    
  }
  catch (std::exception& e) {
    syslog(LOG_ERR | LOG_USER, "Exception: %s", e.what());
    std::cerr << "Exception: " << e.what() << std::endl;
  }
  
  return 0;
}
