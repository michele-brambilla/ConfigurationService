#include <iostream>
#include <vector>
#include <string>

#include "redox.hpp"

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/error/en.h>

using namespace std;
using redox::Redox;
using redox::Command;


redox::Redox rdx;

struct typelist
{
  typedef int SADD_t; //reply of type 3
  typedef std::string GET_t; //reply of type 2
  typedef std::string SET_t; //reply of type 2
  typedef std::vector<std::string> KEYS_t; //reply of type 1 (or 5?)
};


template<typename Output>
bool ExecRedisCmd(redox::Redox& r,const std::string& s) {
  auto& c = r.commandSync<Output>(redox::Redox::strToVec(s));
  if( !c.ok() ) {
    std::cerr << c.lastError() << std::endl;
    return false;
  }
  return true;
}
template<typename Output>
bool ExecRedisCmd(redox::Redox& r,const std::string& s, Output& out) {
  auto& c = r.commandSync<Output>(redox::Redox::strToVec(s));
  if( !c.ok() ) {
    std::cerr << c.lastError() << std::endl;
    return false;
  }
  out = c.reply();
  return true;
}

typelist::KEYS_t ReturnValue(const std::string& key) {
  typelist::KEYS_t result;
  std::string s = {"SMEMBERS "};
  s+=key;
  // std::cout << "ExecRedisCmd<typelist::KEYS_t>(rdx,s,result) = " << ExecRedisCmd<typelist::KEYS_t>(rdx,s,result) << std::endl;
  if( ExecRedisCmd<typelist::KEYS_t>(rdx,s,result) ) {
    for (const std::string& r : result)
      std::cout << r << "\t";
  }
  else {
    typelist::GET_t r;
    s="GET "+key;
    //    std::cout << "ExecRedisCmd<typelist::GET_t>(rdx,s,result) = " << ExecRedisCmd<typelist::GET_t>(rdx,s,r) << std::endl;
    if( !ExecRedisCmd<typelist::GET_t>(rdx,s,r) )
      throw std::runtime_error("Wrong data type");
    result.push_back(r);
  }
  return result;
}

bool KeyExists(const std::string& key) {
  std::string s="KEYS "+key;
  typelist::KEYS_t result;
  ExecRedisCmd<typelist::KEYS_t>(rdx,s,result);
  // std::cout << ExecRedisCmd<typelist::KEYS_t>(rdx,s,result) << std::endl;
  // std::cout << "size of result: " << result.size() << std::endl;
  return true;
}

bool scan(rapidjson::Value::MemberIterator first,
          rapidjson::Value::MemberIterator last,
          std::string prefix="") {
  
  bool is_ok = true;
  std::string separator="";
  if(prefix!="")
    separator=":";
  
  for( auto& it = first; it != last; ++it) {
    is_ok &= (!KeyExists(prefix+separator+std::string(it->name.GetString())) );
    if(it->value.IsObject()) {
      std::string s = "SADD "+prefix+separator+std::string(it->name.GetString())+std::string(" ");
      for( auto v = it->value.MemberBegin();
           v != it->value.MemberEnd(); ++v) {
        auto k = s+v->name.GetString()+std::string(" ");
        //      std::cout << "\t" << s << std::endl;
        // if( is_ok )
        ExecRedisCmd<typelist::SADD_t>(rdx,s);
        // else
        //   std::cout << "exising key, not added" << std::endl;
        //        prefix += separator+std::string(it->name.GetString());
        is_ok &= scan(it->value.MemberBegin(),
                      it->value.MemberEnd(),
                      prefix+separator+std::string(it->name.GetString()));
      }
    }
    else {
      std::string s = "SET "+prefix+separator+std::string(it->name.GetString())+std::string(" ");
      s+= it->value.GetString();
      // if(is_ok)
      ExecRedisCmd<typelist::SET_t>(rdx,s);
      //     else
      // std::cout << "exising key, not added" << std::endl;
      
    }
  }
  return is_ok;
}



const char* instrument_file = "../sample/example_instrument.js";

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





int main() {


  //  Redox rdx;
  if( !rdx.connect("192.168.10.11") )
    throw runtime_error("Can't connect to REDIS server");

  
  // auto got_reply = [](Command<vector<string> >& c) {
  //   cerr << "Command has error code " << c.status() << endl;
  //   cout << c.cmd() << ": " <<  endl;
  //   return c.ok();
  // };
  
  // rdx.command<vector<string>>({"SMEMBERS", "instrument1"},
  //                             [](Command<vector<string>>& c){
  //                               if(!c.ok()) return;
  //                               cout << "Last 5 elements as a vector: ";
  //                               for (const string& s : c.reply()) cout << s << " ";
  //                               cout << endl;
  //                             }
  // );


  // auto& c = rdx.commandSync<std::vector<std::string> >({"KEYS", "instrument1"});
  // cerr << "Command has error code " << c.status() << endl;
  // cout << c.cmd() << ": " << endl;
  // for ( auto& s : c.reply()) cout << s << " ";
  // cout << endl;
                                
  // // // for(int i = 0; i < 10; i++) 
  // //   rdx.command<string>({"GET", "instrument1:sources:motor2:address"}, got_reply);
  
  // // // Do useful work
  // // this_thread::sleep_for(chrono::milliseconds(10));
  
  // // rdx.command<string>({"LADD", "one", "a", "b", "c", "d"}, got_reply);

  // // std::string message;
  // // rdx.command<std::string>({"SMEMBERS","instrument1"},got_reply);

  // // rdx.command<vector<string> >({"LRANGE", "two", "0", "-1"},got_reply
  // //                              );

  // //  rdx.wait();
  // rdx.disconnect();


  std::cout << std::endl;

  rapidjson::Document t;
  rapidjson::ParseResult ok = t.Parse(read_config_file(instrument_file).c_str());
  if (!ok) {
    std::cerr << "JSON parse error: " << rapidjson::GetParseError_En(ok.Code())
              << "( " << ok.Offset()
              << ")\n";
    throw std::runtime_error("Error: invalid configuration") ;
  }

  if(t.IsObject())
    scan(t.MemberBegin(),t.MemberEnd());
  else
    throw std::runtime_error("Can't parse ../sample/example_instrument.js");

  if( KeyExists("instrument1:sources:motor4")) {
    auto r =  ReturnValue("instrument1:sources:motor4");
    for( auto v : r )
      std::cout << v << std::endl;
  }

  
  // typelist::KEYS_t r = ReturnValue("instrument1:user:experiment:sources:motor1:type");
  // std::cout << r.size() << std::endl;
  // r = ReturnValue("instrument1:user:experiment:sources:motor1");
  // r.pop_back();
  // std::cout << r.size() << std::endl;

  
  return 0;
}