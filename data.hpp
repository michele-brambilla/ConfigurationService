#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <redox.hpp>

#include<container.hpp>



namespace configuration {

  using namespace container;
  
  namespace data {
    

    struct DataManager {
      typedef DataManager self_t;

      DataManager() { };
      
      bool AddConfig(std::string conf) {
        rapidjson::Document t;
        t.Parse(conf.c_str());
        if( t.HasParseError() ) { throw std::runtime_error("Error: invalid configuration"); }
        return scan(t.MemberBegin(),t.MemberEnd());
      }
      
      std::vector<std::string> Query(const std::string& key) const {
        if( KeyExists(key) )
          return ReturnValue(key);
        std::cerr << "Key " << key << " doesn't exists" << std::endl;
        return MockContainer::value_type();
      }
      
      bool Update(const std::string& key, const std::string& value) {
        if( !KeyExists(key) ) {
          AddToHash(key,value);
          return UpdateParent(key);
        }
        else
          UpdateHashValue(key,value);
        
        updates.push_back(std::pair<std::string,std::string>(key,"u"));
        return true;
      }
      
      int Delete(const std::string& key) {
        if( !KeyExists(key) ) {
          std::cerr << "Key " << key << " doesn't exists" << std::endl;
          return true;
        }
        int deleted_keys = 0;
        if( IsSet(key) ) {
          
          // split parents and key item
          std::size_t found = key.find_last_of(":");

          // // removes item from set
          if ( !RemoveFromParent(key.substr(0,found),key.substr(found+1)) )
            return false;
          // remove key and children
          int n=RemoveChildren(key);
          if( n <= 0 ) return false;
          
          deleted_keys+=n;
          return deleted_keys > 0;
        }
        else {
          updates.push_back(std::pair<std::string,std::string>(key,"d"));
          return RemoveKey(key)==1;
        }
        return false;
      }

      template<typename CommunicatorManager>
      bool Notify(CommunicatorManager& comm) {
        comm.Send(updates);
        return updates.empty();
      };
       std::vector<std::pair<std::string,std::string> > const& UpdatesList()  {
        return updates;
      };
      
      bool IsValidString(std::string s) {
        return !rapidjson::Document().Parse(s.c_str()).HasParseError();
      }
      
    private:

      std::vector<std::pair<std::string,std::string> > updates;
      
      virtual bool KeyExists(const std::string& key) const { return false; }

      virtual bool IsSet(const std::string&) { return 0; }

      virtual void NotifyKeyNew(const std::string&) { };
      virtual void NotifyValueUpdate(const std::string&) { };

      virtual void AddToKeyList(const MockContainer::key_type&, const MockContainer::value_type&) { }
      virtual void AddToHash(const MockContainer::key_type&, const MockContainer::value_type&) { }
      virtual void AddToHash(const std::string&, const std::string&) { };

      virtual bool RemoveFromParent(const std::string&, const std::string&) { return false; }

      virtual int RemoveKey(const std::string&) { return 0; }

      virtual int RemoveChildren(const std::string&) { return 0; }

      virtual std::vector<std::string> ReturnValue(const std::string&) const {
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

      MockDataManager() { };
      void Dump(std::ostream& os=std::cout) { os << container << std::endl; }
      
    private:
      
      MockContainer container;
      std::vector<std::pair<std::string,std::string> > updates;
      
      bool KeyExists(const std::string& key) const {
        return (container.find(key) != container.end());
      }
      
      bool IsSet(const std::string& key) { return container[key].size() > 1; }
      
      void NotifyKeyNew(const std::string& key) { }; // new key is added to the configuration
      void NotifyValueUpdate(const std::string& key) { }; // the value of the key has been updated

      void AddToKeyList(const MockContainer::key_type& key, const MockContainer::value_type& value) {
        container.AddKeyValue(key,value);
        updates.push_back(std::pair<std::string,std::string>(key,"a"));
      }
      void AddToHash(const MockContainer::key_type& key, const MockContainer::value_type& value) {
        container.AddKeyValue(key,value);
        updates.push_back(std::pair<std::string,std::string>(key,"a"));
      }
      void AddToHash(const std::string& key, const std::string& value) {
        container[key].push_back(value);
        updates.push_back(std::pair<std::string,std::string>(key,"a"));
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

      std::vector<std::string> ReturnValue(const std::string& key) const {
        return container.find(key)->second;
      }

      bool UpdateHashValue(const std::string& key,const std::string& value) {
        container[key][0]=value;
        return true;
      }


      bool AddToParent(const std::string& key,const std::string& value) {
        container[key].push_back(value);
        return true;
      }

      bool UpdateParent(const std::string& key) {
        std::size_t found = key.find_last_of(":");
        std::string parent_key=key.substr(0,found);
        std::string parent_value=key.substr(found+1);
        if( !KeyExists(parent_key) ) {
          std::cerr << "Parent key " << parent_key << " doesn't exists" << std::endl;
          UpdateParent(parent_key);
          container[parent_key].push_back(parent_value);
          return true;
        }
        AddToParent(parent_key,parent_value);
        return true;
      }

      bool scan(rapidjson::Value::MemberIterator first,
                rapidjson::Value::MemberIterator last,
                std::string prefix="") {
        
        bool scan_ok = true;
        for( auto it = first; it != last; ++it) {
          if( KeyExists(it->name.GetString()) ) {
            std::cerr << "Error: configuration " << it->name.GetString() << " exists, nothing to do"<< std::endl;
            return false;
          }
        }
        
        std::vector<std::string> values;
        std::string separator="";
        if(prefix!="")
          separator=":";
        
        for( auto& it = first; it != last; ++it) {
          values.push_back(std::string(it->name.GetString()));
          if(it->value.IsObject())  // if value is object create a  "set"
            scan_ok &= scan( it->value.MemberBegin(),
                             it->value.MemberEnd(),
                             prefix+separator+it->name.GetString());
          else {
            if(it->value.IsString()) { // if value is a string create a "hash-value"
              std::vector<std::string> tmp;
              tmp.push_back(std::string(it->value.GetString()));
              // if key doesn't exists add to config, else erro message
              if( !KeyExists(prefix+separator+it->name.GetString()) )
                AddToHash(prefix+separator+it->name.GetString(),tmp);
              else {
                std::cerr << "key " << prefix+separator+it->name.GetString() << " exists, not added to configuration" << std::endl;
                return false;
              }
            }
            else
              throw std::runtime_error("Error parsing configuration: value is neither an object nor a string");
          }
        }

        if (prefix=="") {
          AddToKeyList("main",values);
        }
        else {
          AddToKeyList(prefix,values);
        }
        return scan_ok;
      }
    };
    
    


    /////////////////////
    // Redis data manager
    struct RedisDataManager  : public DataManager
    {
      typedef RedisDataManager self_t;

      RedisDataManager(const std::string& redis_server, const int& redis_port) {
        if( !rdx.connect(redis_server, redis_port) )
          throw std::runtime_error("Can't connect to REDIS server");
      };

      
      void Dump(std::ostream& os=std::cout) { os << container << std::endl; }
      
    private:
      
      redox::Redox rdx;
      MockContainer container;
      std::vector<std::pair<std::string,std::string> > updates;
      
      bool KeyExists(const std::string& key) const {
        return (container.find(key) != container.end());
      }
      
      bool IsSet(const std::string& key) { return container[key].size() > 1; }
      
      void NotifyKeyNew(const std::string& key) { }; // new key is added to the configuration
      void NotifyValueUpdate(const std::string& key) { }; // the value of the key has been updated

      void AddToKeyList(const MockContainer::key_type& key, const MockContainer::value_type& value) {
        container.AddKeyValue(key,value);
        updates.push_back(std::pair<std::string,std::string>(key,"a"));
      }
      void AddToHash(const MockContainer::key_type& key, const MockContainer::value_type& value) {
        container.AddKeyValue(key,value);
        updates.push_back(std::pair<std::string,std::string>(key,"a"));
      }
      void AddToHash(const std::string& key, const std::string& value) {
        container[key].push_back(value);
        updates.push_back(std::pair<std::string,std::string>(key,"a"));
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

      std::vector<std::string> ReturnValue(const std::string& key) const {
        return container.find(key)->second;
      }

      bool UpdateHashValue(const std::string& key,const std::string& value) {
        container[key][0]=value;
        return true;
      }


      bool AddToParent(const std::string& key,const std::string& value) {
        container[key].push_back(value);
        return true;
      }

      bool UpdateParent(const std::string& key) {
        std::size_t found = key.find_last_of(":");
        std::string parent_key=key.substr(0,found);
        std::string parent_value=key.substr(found+1);
        if( !KeyExists(parent_key) ) {
          std::cerr << "Parent key " << parent_key << " doesn't exists" << std::endl;
          UpdateParent(parent_key);
          container[parent_key].push_back(parent_value);
          return true;
        }
        AddToParent(parent_key,parent_value);
        return true;
      }

      bool scan(rapidjson::Value::MemberIterator first,
                rapidjson::Value::MemberIterator last,
                std::string prefix="") {
        
        bool scan_ok = true;
        for( auto it = first; it != last; ++it) {
          if( KeyExists(it->name.GetString()) ) {
            std::cerr << "Error: configuration " << it->name.GetString() << " exists, nothing to do"<< std::endl;
            return false;
          }
        }
        
        std::vector<std::string> values;
        std::string separator="";
        if(prefix!="")
          separator=":";
        
        for( auto& it = first; it != last; ++it) {
          values.push_back(std::string(it->name.GetString()));
          if(it->value.IsObject())  // if value is object create a  "set"
            scan_ok &= scan( it->value.MemberBegin(),
                             it->value.MemberEnd(),
                             prefix+separator+it->name.GetString());
          else {
            if(it->value.IsString()) { // if value is a string create a "hash-value"
              std::vector<std::string> tmp;
              tmp.push_back(std::string(it->value.GetString()));
              // if key doesn't exists add to config, else erro message
              if( !KeyExists(prefix+separator+it->name.GetString()) )
                AddToHash(prefix+separator+it->name.GetString(),tmp);
              else {
                std::cerr << "key " << prefix+separator+it->name.GetString() << " exists, not added to configuration" << std::endl;
                return false;
              }
            }
            else
              throw std::runtime_error("Error parsing configuration: value is neither an object nor a string");
          }
        }

        if (prefix=="") {
          AddToKeyList("main",values);
        }
        else {
          AddToKeyList(prefix,values);
        }
        return scan_ok;
      }
    };

    
  }
}

