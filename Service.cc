#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdlib.h> // two arguments to command line option

#include <configuration.hpp>

using namespace std;

using namespace configuration::data;
using namespace configuration::communicator;

std::string read_config_file(const char* s) {
  std::ifstream in;
  in.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  try {
    in.open(s);
  }
  catch (std::ifstream::failure e) {
    std::cout << "Exception opening/reading file: " << e.what() << std::endl;
  }
  std::string config,buf;
  while(!in.eof()) {
     std::getline(in, buf,'\t');
     config += buf;
  }
  in.close();
  return config;
}



std::string data_addr = "192.168.10.11";
int data_port = 6379;

std::string comm_addr = "192.168.10.11";
int comm_port = 6379;

typedef RedisCommunicator CM;
typedef RedisDataManager<CM> DM;


void parse(int,char**);
void help();

int main(int argc, char **argv) {

  parse(argc,argv);
  configuration::ConfigurationManager<DM,CM> cm(data_addr.c_str(),data_port,
                                                comm_addr.c_str(),comm_port);

  configuration::utils::typelist::KEYS_t key_list;
    std::string action,value;
  int sep;

  
  do {
    std::cout << " > ";
    std::cin >> action;

    switch(action[0]) {
    case 'a':
      std::cin >> value;
      cm.AddConfig(read_config_file(value.c_str() ));
      break;
    case 'q':
      std::cin >> value;
      key_list = cm.Query(value);
      for ( auto& k : key_list)
        std::cout << k << std::endl;
      break;
    case 'd':
      std::cin >> value;
      cm.Delete(value);
      break;
    case 'u':
      std::cin >> value;
      sep=value.find('=');
      cm.Update(value.substr(0,sep),value.substr(sep+1,std::string::npos));
      break;
    case 'n':
      cm.Notify();
      break;
    case 's':
      std::cin >> value;
      cm.Subscribe(value);
      break;
    case 'e':
      return 0;
      break;
    default:
      std::cout << "option not available" << std::endl;
      break;      
    }
    std::cout << std::endl;
    
  } while(action!="exit");
  
  
  
  return 0;
}




void parse(int argc, char **argv) {

  int c,found;
  while( (c = getopt(argc, argv, "d:c:h")) != -1) {
    
    switch (c) {
    case 'd':
      found = std::string(optarg).find(':');
      data_addr = std::string(optarg).substr(0,found);
      if( found != std::string::npos)
        std::istringstream ( std::string(optarg).substr(found+1,std::string::npos)) >> data_port;
      break;
    case 'c':
      found = std::string(optarg).find(':');
      comm_addr = std::string(optarg).substr(0,found);
      if( found != std::string::npos)
        std::istringstream ( std::string(optarg).substr(found+1,std::string::npos)) >> comm_port;
      break;
    case 'h':
      help();
      /* getopt_long already printed an error message. */
      break;
    }
  }
}




void help() {
  std::cout << "Command line interface for the configuration service." << std::endl
            << " " << std::endl
            << " Options: " << std::endl
            << "-d <address>[:<port>]\t"    << "connects to the data server at address <address>:<port> (default "
            << data_addr << ":" << data_port << ")" << std::endl
            << "-c <address>[:<port>]\t"    << "connects to the communicatinos server at address <address>:<port> (default "
            << comm_addr << ":" << comm_port << ")" << std::endl
            << "-h\t\t\tthis help" << std::endl
            << std::endl;

  std::cout << "USAGE:\n" << std::endl;
  
  exit(0);
}
