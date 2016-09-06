#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <redis_utils.hpp>

#include<basic_data.hpp>



namespace configuration {

  using namespace container;

  
  namespace data {
    
    /////////////////////
    // Redis data manager
    struct RedisDataManager  : public DataManager
    {
      typedef RedisDataManager self_t;

      RedisDataManager(const std::string& redis_server,
                       const int& redis_port,
                       std::ostream& logger=std::cerr) : rdx(std::cout,redox::log::Level::Fatal), log(logger) {
        if( !rdx.connect(redis_server, redis_port) ) {
          log << "Can't connect to REDIS server\n";
          throw std::runtime_error("Can't connect to REDIS server");
        }
      };

      void Dump(std::ostream& os=std::cout) {
        std::cout << "DUMP " << std::endl;
        utils::typelist::KEYS_t result;
        std::string s = {"KEYS *"};
        if( utils::ExecRedisCmd<utils::typelist::KEYS_t>(rdx,s,result) )
          for( auto r : result) {
            os << r << std::endl;
            for( auto v : ReturnValue(r) )
              os << "\t" << v;
            os << "\n";
          }
        else
          throw std::runtime_error("No keys in db");
      }

      
      void Clear() { rdx.command<std::string>({"FLUSHALL"}); }

      redox::Redox& redox() { return rdx; } 
      
    private:

      redox::Redox rdx;
      std::ostream& log;
      //      MockContainer container;
      std::vector<std::pair<std::string,std::string> > updates;

      bool KeyExists (const std::string& key) override {
        std::string s="KEYS "+key;
        utils::typelist::KEYS_t result;
        utils::ExecRedisCmd<utils::typelist::KEYS_t>(rdx,s,result);
        return result.size()>0;
      }


      bool AddToHash(const std::string& key,
                     const std::vector<std::string>& value) override {
        bool is_ok = true;
        for(auto& v : value)
          is_ok &= AddToHash(key,v);
        return is_ok;
      }
      bool AddToHash(const std::string& key, const std::string& value) override {
        updates.push_back(std::pair<std::string,std::string>(key,"a"));
        return rdx.set(key,value);
      }
      
      bool RemoveFromParent(const std::string& parent,const std::string& name) override {
        bool is_ok = true;
        is_ok &= utils::ExecRedisCmd<utils::typelist::DEL_t>(rdx,std::string("SREM ")+parent+" "+name);
        // if parents gets empty, delete
        int nelem;
        utils::ExecRedisCmd<int>(rdx,std::string("SCARD ")+parent,nelem);
        if (nelem == 0)
          is_ok &= (RemoveKey(parent) > 0);
        
        return is_ok;
      }
      int RemoveChildren(const std::string& key) override {
        utils::typelist::KEYS_t children_list;
        std::string cmd="KEYS "+key+":*";
        utils::ExecRedisCmd<utils::typelist::KEYS_t>(rdx,cmd,children_list);
        for( auto& c : children_list )
          utils::ExecRedisCmd<utils::typelist::DEL_t>(rdx,"DEL "+c);
        return children_list.size();
      }

      bool RemoveKey(const std::string& key) override {
        updates.push_back(std::pair<std::string,std::string>(key,"d"));   
        std::string key_type;
        utils::ExecRedisCmd<std::string>(rdx,std::string("TYPE ")+key,key_type);
        bool is_ok = true;
        
        if(key_type == "set") {
          // remove children
          int nelem;
          utils::ExecRedisCmd<int>(rdx,std::string("SCARD ")+key,nelem);
          is_ok &= ( (RemoveChildren(key)>0 && nelem >0) ? true : false);
        }
        
        // remove from parent
        std::size_t found = key.find_last_of(":");
        // std::string parent_key=key.substr(0,found);
        // std::string parent_value=key.substr(found+1);
        is_ok &= RemoveFromParent(key.substr(0,found),key.substr(found+1));
        is_ok &= utils::ExecRedisCmd<int>(rdx,std::string("DEL ")+key);
        return is_ok;
        
      }

      
      bool UpdateHashValue(const std::string& key,const std::string& value) override {
        std::string cmd = std::string("SET ")+key+" "+value;
        return utils::ExecRedisCmd<std::string>(rdx,cmd);
      }


      bool AddToParent(const std::string& key,const std::string& value) override {
        std::string cmd=std::string("SADD ")+key+" "+value;
        return utils::ExecRedisCmd<int>(rdx,cmd);;
      }

      bool UpdateParent(const std::string& key) override {
        bool is_ok = true;
        std::size_t found = key.find_last_of(":");
        std::string parent_key=key.substr(0,found);
        std::string parent_value=key.substr(found+1);
        if( !KeyExists(parent_key) ) {
          is_ok &= UpdateParent(parent_key);
          is_ok &= AddToParent(parent_key,parent_value);
          return is_ok;
        }

        return AddToParent(parent_key,parent_value);
      }

      
      utils::typelist::LIST_t ReturnValue(const std::string& key) override {
        utils::typelist::LIST_t result;
        if( !KeyExists(key) ) {
          log << "Key " << key << " doesn't exists\n";
          return result;
        }
        
        std::string key_type;
        utils::ExecRedisCmd<std::string>(rdx,std::string("TYPE ")+key,key_type);

        if(key_type == "set") {
          utils::ExecRedisCmd<utils::typelist::LIST_t>(rdx,std::string("SMEMBERS ")+key,result);
          // removes eventual empty items
          result.erase( std::remove( result.begin(), result.end(), "" ), result.end() );
        }
        else
          if(key_type == "string") {
            std::string tmp;
            utils::ExecRedisCmd<utils::typelist::VALUE_t>(rdx,std::string("GET ")+key,tmp);
            result.push_back(tmp);
          }
          else {
            //            throw std::runtime_error("Type "+key_type+" not supported");
            log << "Type "+key_type+" not supported";
          }
            
        return result;
      }


      bool scan(rapidjson::Value::MemberIterator first,
                rapidjson::Value::MemberIterator last,
                std::string prefix="") override {
  
        bool is_ok = true;
        std::string separator="";
        if(prefix!="")
          separator=":";
  
        for( auto& it = first; it != last; ++it) {
          is_ok &= (!KeyExists(prefix+separator+std::string(it->name.GetString())) );
          if(it->value.IsObject()) {
            std::string s = "SADD "+ prefix+separator+std::string(it->name.GetString())+std::string(" ");
            for( auto v = it->value.MemberBegin();
                 v != it->value.MemberEnd(); ++v)
              is_ok &= utils::ExecRedisCmd<utils::typelist::SADD_t>(rdx,s+v->name.GetString());
            
            is_ok &= scan(it->value.MemberBegin(),
                          it->value.MemberEnd(),
                          prefix+separator+std::string(it->name.GetString()));
          }
          else {
            std::string s = "SET "+prefix+separator+std::string(it->name.GetString())+std::string(" ");
            s+= it->value.GetString();
            if(is_ok)
              utils::ExecRedisCmd<utils::typelist::SET_t>(rdx,s);
            else
              log << "Key " << std::string(it->value.GetString()) << " exists, not added\n";

          }
        }
        return is_ok;
      }



    };

    
  }
}

