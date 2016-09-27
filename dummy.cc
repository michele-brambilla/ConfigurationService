#include <iostream>
#include <vector>
#include <string>

#include <future>

// #include "redox.hpp"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>

#include <configuration.hpp>

static const char* kTypeNames[] = 
  { "Null", "False", "True", "Object", "Array", "String", "Number" };


template<typename T>
void redis_json_scan(T& member,std::string);

// using namespace std;
// using redox::Redox;
// using redox::Command;


redox::Redox rdx;

// struct typelist
// {
//   typedef int SADD_t; //reply of type 3
//   typedef std::string GET_t; //reply of type 2
//   typedef std::string SET_t; //reply of type 2
//   typedef std::vector<std::string> KEYS_t; //reply of type 1 (or 5?)
// };


template<typename Output>
bool ExecRedisCmd(const std::string& s) {
  auto& c = rdx.commandSync<Output>(redox::Redox::strToVec(s));
  if( !c.ok() ) {
    std::cerr << c.lastError() << std::endl;
    return false;
  }
  return true;
}
// template<typename Output>
// bool ExecRedisCmd(redox::Redox& r,const std::string& s, Output& out) {
//   auto& c = r.commandSync<Output>(redox::Redox::strToVec(s));
//   if( !c.ok() ) {
//     std::cerr << c.lastError() << std::endl;
//     return false;
//   }
//   out = c.reply();
//   return true;
// }

// typelist::KEYS_t ReturnValue(const std::string& key) {
//   typelist::KEYS_t result;
//   std::string s = {"SMEMBERS "};
//   s+=key;
//   // std::cout << "ExecRedisCmd<typelist::KEYS_t>(rdx,s,result) = " << ExecRedisCmd<typelist::KEYS_t>(rdx,s,result) << std::endl;
//   if( ExecRedisCmd<typelist::KEYS_t>(rdx,s,result) ) {
//     for (const std::string& r : result)
//       std::cout << r << "\t";
//   }
//   else {
//     typelist::GET_t r;
//     s="GET "+key;
//     //    std::cout << "ExecRedisCmd<typelist::GET_t>(rdx,s,result) = " << ExecRedisCmd<typelist::GET_t>(rdx,s,r) << std::endl;
//     if( !ExecRedisCmd<typelist::GET_t>(rdx,s,r) )
//       throw std::runtime_error("Wrong data type");
//     result.push_back(r);
//   }
//   return result;
// }

// bool KeyExists(const std::string& key) {
//   std::string s="KEYS "+key;
//   typelist::KEYS_t result;
//   ExecRedisCmd<typelist::KEYS_t>(rdx,s,result);
//   // std::cout << ExecRedisCmd<typelist::KEYS_t>(rdx,s,result) << std::endl;
//   // std::cout << "size of result: " << result.size() << std::endl;
//   return true;
// }

// bool scan(rapidjson::Value::MemberIterator first,
//           rapidjson::Value::MemberIterator last,
//           std::string prefix="") {
  
//   bool is_ok = true;
//   std::string separator="";
//   if(prefix!="")
//     separator=":";
  
//   for( auto& it = first; it != last; ++it) {
//     is_ok &= (!KeyExists(prefix+separator+std::string(it->name.GetString())) );
//     if(it->value.IsObject()) {
//       std::string s = "SADD "+prefix+separator+std::string(it->name.GetString())+std::string(" ");
//       for( auto v = it->value.MemberBegin();
//            v != it->value.MemberEnd(); ++v) {
//         auto k = s+v->name.GetString()+std::string(" ");
//         //      std::cout << "\t" << s << std::endl;
//         // if( is_ok )
//         ExecRedisCmd<typelist::SADD_t>(rdx,s);
//         // else
//         //   std::cout << "exising key, not added" << std::endl;
//         //        prefix += separator+std::string(it->name.GetString());
//         is_ok &= scan(it->value.MemberBegin(),
//                       it->value.MemberEnd(),
//                       prefix+separator+std::string(it->name.GetString()));
//       }
//     }
//     else {
//       std::string s = "SET "+prefix+separator+std::string(it->name.GetString())+std::string(" ");
//       s+= it->value.GetString();
//       // if(is_ok)
//       ExecRedisCmd<typelist::SET_t>(rdx,s);
//       //     else
//       // std::cout << "exising key, not added" << std::endl;
      
//     }
//   }
//   return is_ok;
// }



const char* instrument_file = "sample/example_instrument.js";

// std::string read_config_file(const char* s) {
//   std::ifstream in;
//   in.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
//   try {
//     in.open(s);
//   }
//   catch (std::ifstream::failure e) {
//     std::cout << "Exception opening/reading file: " << e.what() << std::endl;
//   }

//   std::string config,buf;
//   while(!in.eof()) {
//      std::getline(in, buf,'\t');
//      config += buf;
//   }
//   in.close();
//   return config;
// }



const char* redis_server = "192.168.10.11";
const int redis_port = 6379;

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



void redis_connection_callback(int status, int& output) {
  if( status == redox::Redox::CONNECTED)
    std::cout << "Dummy connection status: " << status << std::endl;
  if( status == redox::Redox::DISCONNECTED)
    std::cout << "Dummy disconnected " << std::endl;
  output = status;
  return;
}




void dummy_connection_callback(int status) {
  if( status == redox::Redox::CONNECTED)
    std::cout << "Dummy connection status: " << status << std::endl;
  if( status == redox::Redox::DISCONNECTED)
    std::cout << "Dummy disconnected " << std::endl;
  return;
}


void counting_got_message(const std::string & t,const std::string & c,int& count)  {
  std::cout << t << c << std::endl;
  count++;
  return;
}


int multiply(const int& a, const int& b)
{
    return a * b;
}


void goodbye(const std::string& s,const std::string& r)
{
  std::cout << "Goodbye " << s << " " << r << '\n';
}

bool is_connected (int x) {
  return (x == configuration::CONNECTED);
}


void connect (std::shared_ptr<RedisCommunicator>& cm) {
  cm = std::make_shared<RedisCommunicator>("192.168.10.1",6379);
  //  return  ( cm->publisher_connection_status == configuration::CONNECTED);
}

int main() {

  // int status = -1;
  // std::shared_ptr<redox::Redox> conn = std::make_shared<redox::Redox>();

  // int connection_status;
  // if( !conn->connect("localhost", 6379,std::bind(configuration::utils::redis_connection_callback,
  //                                                std::placeholders::_1,
  //                                                std::ref(connection_status)) ) ) {
  //   conn = std::make_shared<redox::Redox>();
  //   conn->connect("192.168.10.11", 6379,std::bind(configuration::utils::redis_connection_callback,
  //                                                std::placeholders::_1,
  //                                                 std::ref(connection_status)) );
  // }

  // conn->disconnect();

  // std::shared_ptr<RedisCommunicator> cm; //= make_unique<RedisCommunicator>("192.168.10.1",6379);
  // auto fut = std::async(std::launch::async,connect,std::ref(cm));

  // if (fut.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {
  //   throw std::runtime_error("connection timeout\n");
  // }

  //  ZmqCommunicator zc("192.168.10.1",6379);
  
  
  // while (fut.wait_for(std::chrono::seconds(1)) != std::future_status::ready) {
  //   std::cout << "... still not ready\n";
  //   if (fut.wait_for(std::chrono::seconds(5)) != std::future_status::ready) {

  //   }
  // }
  
  // std::chrono::milliseconds span (100);
  // while (fut.wait_for(span)==std::future_status::timeout)
  //   std::cout << '.';
  // //  bool x = fut.get();
   
  // if( cm->publisher_connection_status != configuration::CONNECTED )
  //   throw std::runtime_error("DataManager not connected\n");




  
  
  // std::shared_ptr<redox::Redox> rdx = std::make_shared<redox::Redox>() ;
  // if( !rdx->connect("localhost") )
  //   std::cout << "connecting to " << rdx->connect("192.168.10.11") << std::endl;

  
  // std::this_thread::sleep_for(std::chrono::seconds(1));  
  // rdx.disconnect();
  // std::this_thread::sleep_for(std::chrono::seconds(1));  
  
  if( !rdx.connect("localhost",16379) )
    std::cout << "connecting to " << rdx.connect("192.168.10.11") << std::endl;

  

  
  // int n;
  // auto f = std::bind(counting_got_message,
  //                    std::placeholders::_1,
  //                    std::placeholders::_2,
  //                    std::ref(n));
  // f("a","b");
  // int y = 5;
  // auto h = std::bind(multiply, std::cref(y), std::placeholders::_1);
  // for (int i = 0; i < 10; i++)
  //   {
  //     std::cout << "5 * " << i << " = " << h(i) << std::endl;
  //   }

  // std::string str("World");
  // std::function<void(const std::string&)>  g = std::bind(goodbye, std::ref(str) , std::placeholders::_1);
  // g("crudele");
  
  // RedisCommunicator cm(redis_server,redis_port);
  // std::cout << cm.Subscribe("message:*") << std::endl;
  
  // cm.Publish("message:1","three");
  // cm.Disconnect();
  // std::cout << cm.Reconnect() << std::endl;

  // //  std::function<void(int)>
  
  //RedisDataManager<RedisCommunicator> dm(redis_server,redis_port,cm);


  // while(1) {
  //   dm.Dump();

  //   std::this_thread::sleep_for(std::chrono::seconds(5));
    
  // }


  // configuration::ConfigurationManager<D,C> config(redis_server,redis_port,
  //                                                 redis_server,redis_port);


  

  // // config.AddConfig(read_config_file("../sample/example_instrument.js"));

  // std::cout << config.Subscribe("message:*") << std::endl;
  // std::cout << (config.SubscriberConnectionStatus()==configuration::CONNECTED ? "ok" : "bad") << std::endl;
  

  
  //  configuration::Init(redis_server,redis_port);
  
  
  // redox::Redox rdx;
  // // rdx.connect(redis_server,redis_port,dummy_connection_callback);

  // // std::this_thread::sleep_for(std::chrono::seconds(1));


  
  // int connection_status;
  // rdx.connect(redis_server,redis_port,
  //             std::bind(redis_connection_callback,std::placeholders::_1, std::ref(connection_status)));
  // std::cout << connection_status << std::endl;
  // rdx.disconnect();
  // std::cout << connection_status << std::endl;
    
  // bool ok = false;
  // int counter = 0;
  // while( (ok = rdx.connect("localhost") ) == false  ) {
  //   std::this_thread::sleep_for(std::chrono::seconds(1));
  //   rdx.disconnect();
  //   std::this_thread::sleep_for(std::chrono::seconds(1));
  //   std::cout << counter++ << std::endl;
  // }
  
  // if( !ok )
  //   throw std::runtime_error("Can't connect to REDIS server");
  
  // // auto got_reply = [](Command<vector<string> >& c) {
  // //   cerr << "Command has error code " << c.status() << endl;
  // //   cout << c.cmd() << ": " <<  endl;
  // //   return c.ok();
  // // };
  
  // // rdx.command<vector<string>>({"SMEMBERS", "instrument1"},
  // //                             [](Command<vector<string>>& c){
  // //                               if(!c.ok()) return;
  // //                               cout << "Last 5 elements as a vector: ";
  // //                               for (const string& s : c.reply()) cout << s << " ";
  // //                               cout << endl;
  // //                             }
  // // );


  // // auto& c = rdx.commandSync<std::vector<std::string> >({"KEYS", "instrument1"});
  // // cerr << "Command has error code " << c.status() << endl;
  // // cout << c.cmd() << ": " << endl;
  // // for ( auto& s : c.reply()) cout << s << " ";
  // // cout << endl;
                                
  // // // // for(int i = 0; i < 10; i++) 
  // // //   rdx.command<string>({"GET", "instrument1:sources:motor2:address"}, got_reply);
  
  // // // // Do useful work
  // // // this_thread::sleep_for(chrono::milliseconds(10));
  
  // // // rdx.command<string>({"LADD", "one", "a", "b", "c", "d"}, got_reply);

  // // // std::string message;
  // // // rdx.command<std::string>({"SMEMBERS","instrument1"},got_reply);

  // // // rdx.command<vector<string> >({"LRANGE", "two", "0", "-1"},got_reply
  // // //                              );

  // // //  rdx.wait();
  // // rdx.disconnect();


  // std::cout << std::endl;

  rapidjson::Document t;
  rapidjson::ParseResult ok = t.Parse(read_config_file(instrument_file).c_str());
  if (!ok) {
    std::cerr << "JSON parse error: " << rapidjson::GetParseError_En(ok.Code())
              << "( " << ok.Offset()
              << ")\n";
    throw std::runtime_error("Error: invalid configuration") ;
  }
  


        
  if(t.IsObject()) {
    for (auto& itr : t.GetObject()) {
    //   if( itr.value.IsObject() )
    //     for (auto& m : itr.value.GetObject() )
    //       if( m.value.IsObject() )
    //         for (auto& n : m.value.GetObject() )
        
    //       printf("Type of member %s is %s\n",
    //              n.name.GetString(), kTypeNames[n.value.GetType()]);
      redis_json_scan(itr,std::string());
    }
  }
  else
    throw std::runtime_error("Can't parse ../sample/example_instrument.js");
  


  // if( KeyExists("instrument1:sources:motor4")) {
  //   auto r =  ReturnValue("instrument1:sources:motor4");
  //   for( auto v : r )
  //     std::cout << v << std::endl;
  // }

  
  // // typelist::KEYS_t r = ReturnValue("instrument1:user:experiment:sources:motor1:type");
  // // std::cout << r.size() << std::endl;
  // // r = ReturnValue("instrument1:user:experiment:sources:motor1");
  // // r.pop_back();
  // // std::cout << r.size() << std::endl;

  
  return 0;
}


template<typename T>
void redis_json_scan(T& member,std::string prefix) {
  if( prefix.size() == 0)
    prefix=std::string(member.name.GetString());
  else
    prefix+=":"+std::string(member.name.GetString());
  
  std::cout << prefix << std::endl;
  for (auto& next : member.value.GetObject()) {
    if( next.value.IsObject() ) {
      std::string cmd = {"SADD "+prefix+" "+next.name.GetString()};
      ExecRedisCmd<int>(cmd);
      std::cout << cmd << std::endl;
      redis_json_scan(next,prefix);
    }
    else {
      std::string cmd = {"HSET "+prefix+" "+next.name.GetString()+" "+next.value.GetString()};
      ExecRedisCmd<int>(cmd);
      std::cout << "\tHSET " << prefix << " " << next.name.GetString() << " " << next.value.GetString() << std::endl;
    }
  }
}
