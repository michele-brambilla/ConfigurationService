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
	bool is_ok = true;
	for (auto& itr : t.GetObject()) {
	  is_ok &= redis_json_scan(itr,std::string());
	}
	return is_ok;
	//        return scan(t.MemberBegin(),t.MemberEnd());
      }

      std::vector<std::string> Query(const std::string& key) { return ReturnValue(key); }
      
      bool Update(const std::string& key, const std::string& value) {
	int found = key.find_last_of(":");
        if( !KeyExists(key) && !HExists(key.substr(0,found),key.substr(found+1))) {
          bool is_ok = AddToHash(key,value);
          return (is_ok & UpdateParent(key.substr(0,found)) );
        }
        else {
          return UpdateHashValue(key,value);
        }        

      }
      
      bool Delete(const std::string& key) {
	std::cout << key << std::endl;
	std::cout << KeyExists(key) << std::endl;
        if( KeyExists(key)) 
	  return RemoveKey(key);

	int found = key.find_last_of(":");
	std::cout << IsHash(key.substr(0,found)) << std::endl;
	if ( IsHash(key.substr(0,found)) && HExists(key.substr(0,found),key.substr(found+1) ) ) 
	  return RemoveHash(key.substr(0,found),key.substr(found+1));

	std::cerr << "Key " << key << " doesn't exists" << std::endl;
	return false;
      }

      bool IsValidString(std::string s) {
        return !rapidjson::Document().Parse(s.c_str()).HasParseError();
      }

      virtual void Clear() { }
    protected:

      virtual bool KeyExists(const std::string&) { return false; }
      virtual bool HExists(const std::string&,const std::string&) { return false; }

      virtual bool IsHash(const std::string&) { return false; }

      virtual void NotifyKeyNew(const std::string&) { };
      
      virtual void NotifyValueUpdate(const std::string&) { };

      virtual void AddToKeyList(const std::string&, const std::vector<std::string>&) { }

      virtual bool AddToHash(const key_type&, const value_type&) {return false; }

      virtual bool AddToHash(const std::string&, const std::string&) { return false; };

      virtual bool RemoveFromParent(const std::string&, const std::string&) { return false; }

      virtual bool RemoveKey(const std::string&) { return false; }
      virtual bool RemoveHash(const std::string&,const std::string&) { return false; }

      virtual int RemoveChildren(const std::string&) { return 0; }

      virtual std::vector<std::string> ReturnValue(const std::string&) {
        return std::vector<std::string>{""};
      }

      
      virtual bool UpdateHashValue(const std::string&,const std::string&) { return false; }

      virtual bool AddToParent(const std::string&,const std::string&) { return false; }

      virtual bool UpdateParent(const std::string&) { return false; }

      // virtual bool scan(rapidjson::Value::MemberIterator,
      //                   rapidjson::Value::MemberIterator,
      //                   std::string="") { return false; }

      typedef rapidjson::GenericMember<rapidjson::UTF8<>, rapidjson::MemoryPoolAllocator<> > GenericMemberIterator;
      virtual bool redis_json_scan(GenericMemberIterator&,std::string) { 
	return false; }
      
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

