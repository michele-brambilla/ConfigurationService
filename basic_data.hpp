#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <redox.hpp>

#include<types.hpp>



namespace configuration {

  //  using namespace container;
  
  namespace data {
    

    struct DataManager {
      typedef DataManager self_t;

      DataManager() { };

      bool AddConfig(const std::string& conf) {
        rapidjson::Document t;
        t.Parse(conf.c_str());
        if( t.HasParseError() ) { throw std::runtime_error("Error: invalid configuration"); }
        return scan(t.MemberBegin(),t.MemberEnd());
      }

      std::vector<std::string> Query(const std::string& key) { return ReturnValue(key); }
      
      bool Update(const std::string& key, const std::string& value) {
        if( !KeyExists(key) ) {
          bool is_ok = AddToHash(key,value);
          return (is_ok & UpdateParent(key) );
        }
        else {
          return UpdateHashValue(key,value);
        }        

      }
      
      bool Delete(const std::string& key) {
        if( !KeyExists(key) ) {
          std::cerr << "Key " << key << " doesn't exists" << std::endl;
          return false;
        }
        return RemoveKey(key);
      }

      bool IsValidString(std::string s) {
        return !rapidjson::Document().Parse(s.c_str()).HasParseError();
      }

      virtual void Clear() { }
    private:

      virtual bool KeyExists(const std::string&) { return false; }

      virtual bool IsSet(const std::string&) { return 0; }

      virtual void NotifyKeyNew(const std::string&) { };
      
      virtual void NotifyValueUpdate(const std::string&) { };

      virtual void AddToKeyList(const std::string&, const std::vector<std::string>&) { }

      virtual bool AddToHash(const key_type&, const value_type&) {return false; }

      virtual bool AddToHash(const std::string&, const std::string&) { return false; };

      virtual bool RemoveFromParent(const std::string&, const std::string&) { return false; }

      virtual bool RemoveKey(const std::string&) { return false; }

      virtual int RemoveChildren(const std::string&) { return 0; }

      virtual std::vector<std::string> ReturnValue(const std::string&) {
        return std::vector<std::string>{""};
      }

      
      virtual bool UpdateHashValue(const std::string&,const std::string&) { return false; }

      virtual bool AddToParent(const std::string&,const std::string&) { return false; }

      virtual bool UpdateParent(const std::string&) { return false; }

      virtual bool scan(rapidjson::Value::MemberIterator,
                        rapidjson::Value::MemberIterator,
                        std::string="") { return false; }

    };



    /////////////////////
    // Mock class
    struct MockDataManager  : public DataManager
    {
      typedef  MockDataManager self_t;

      MockDataManager(const std::string&, const int&) { };

    };
    
    
    
  }
}

