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

      RedisDataManager(const std::string& redis_server, const int& redis_port) : rdx(std::cout,redox::log::Level::Fatal) {
        if( !rdx.connect(redis_server, redis_port) )
          throw std::runtime_error("Can't connect to REDIS server");
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

      utils::typelist::KEYS_t Query(const std::string& key) { return ReturnValue(key); }
      bool Update(const std::string& key, const std::string& value) {
        if( !KeyExists(key) ) {
          bool is_ok = AddToHash(key,value);
          return (is_ok & UpdateParent(key) );
        }
        else {
          updates.push_back(std::pair<std::string,std::string>(key,"u"));          
          return UpdateHashValue(key,value);
        }        
      }
        
      redox::Redox& redox() { return rdx; } 
      
    private:

      redox::Redox rdx;
      MockContainer container;
      std::vector<std::pair<std::string,std::string> > updates;

      bool KeyExists(const std::string& key) {
        std::string s="KEYS "+key;
        utils::typelist::KEYS_t result;
        utils::ExecRedisCmd<utils::typelist::KEYS_t>(rdx,s,result);
        return result.size()>0;
      }

      bool IsSet(const std::string& key) { return container[key].size() > 1; }
      
      void NotifyKeyNew(const std::string& key) { }; // new key is added to the configuration
      void NotifyValueUpdate(const std::string& key) { }; // the value of the key has been updated

      void AddToKeyList(const MockContainer::key_type& key, const std::vector<std::string>& value) {

        std::vector<std::string> l={"SADD",key};
        l.insert(l.end(),value.begin(),value.end());
        // the db has to be updated after the add, so use sync
        auto& c = rdx.commandSync<std::string>(l);
        std::cout << c.cmd() << std::endl;
        std::cout << c.lastError() << std::endl;
        // computing time vs comm time...
        // for( auto& v : value)
        //   rdx.command<std::string>({"SADD",key,v});
        updates.push_back(std::pair<std::string,std::string>(key,"a"));
      }
      
      bool AddToHash(const MockContainer::key_type& key, const std::vector<std::string>& value) {
        bool is_ok = true;
        for(auto& v : value)
          is_ok &= AddToHash(key,v);
        return is_ok;
      }
      bool AddToHash(const std::string& key, const std::string& value) {
        updates.push_back(std::pair<std::string,std::string>(key,"a"));
        return rdx.set(key,value);
      }


      
      bool RemoveFromParent(const std::string& key,const std::string& value) {
        MockContainer::value_type::iterator it = std::find(container[key].begin(),container[key].end(),value);
        if( it != container[key].end()) {
          updates.push_back(std::pair<std::string,std::string>(key,"u"));
          container[key].erase(it);
          return true;
        }
        return false;
      }
      int RemoveKey(const std::string& key) {
        updates.push_back(std::pair<std::string,std::string>(key,"d"));   
        return container.erase(key);
      }
      int RemoveChildren(const std::string& key) {
        int nelem = 0;
        MockContainer::iterator first=container.find(key);
        MockContainer::iterator last=container.find(key);
        while(last->first.substr(0, key.size()) == key) {
          updates.push_back(std::pair<std::string,std::string>(last->first,"d"));
          ++last;
          ++nelem;
        }
        container.erase(first,last);
        return nelem;
      }

      bool UpdateHashValue(const std::string& key,const std::string& value) {
        std::string cmd = std::string("SET ")+key+" "+value;
        return utils::ExecRedisCmd<std::string>(rdx,cmd);
      }


      bool AddToParent(const std::string& key,const std::string& value) {
        std::string cmd=std::string("SADD ")+key+" "+value;
        return utils::ExecRedisCmd<int>(rdx,cmd);;
      }

      bool UpdateParent(const std::string& key) {
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

      
      utils::typelist::LIST_t ReturnValue(const std::string& key) {
        utils::typelist::LIST_t result;
        if( !KeyExists(key) ) return result;

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
          else
            throw std::runtime_error("Type "+key_type+" not supported");

        return result;
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
            std::string s = "SADD "+ prefix+separator+std::string(it->name.GetString())+std::string(" ");
            for( auto v = it->value.MemberBegin();
                 v != it->value.MemberEnd(); ++v)
              s += v->name.GetString()+std::string(" ");

            if( is_ok )
              utils::ExecRedisCmd<utils::typelist::SADD_t>(rdx,s);
            // else
            //   std::cout << "exising key, not added" << std::endl;
            is_ok &= scan(it->value.MemberBegin(),
                          it->value.MemberEnd(),
                          prefix+separator+std::string(it->name.GetString()));
          }
          else {
            std::string s = "SET "+prefix+separator+std::string(it->name.GetString())+std::string(" ");
            s+= it->value.GetString();
            if(is_ok)
              utils::ExecRedisCmd<utils::typelist::SET_t>(rdx,s);
            // else
            //   std::cout << "exising key, not added" << std::endl;

          }
        }
        return is_ok;
      }



    };

    
  }
}

