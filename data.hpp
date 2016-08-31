#pragma once

#include <iostream>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <stdexcept>

namespace configuration {

  namespace data {
    static const char* kTypeNames[] = 
      { "Null", "False", "True", "Object", "Array", "String", "Number" };
    
    struct DataManager {
      DataManager() { };
      
      std::string AddConfig(std::string);
      
      bool NotifyNewConfig(std::string);

      bool IsValidString(std::string);
    };
    
    struct MockContainer {
      typedef std::string key_type;
      typedef std::vector<std::string> value_type;
      typedef typename std::map<key_type,value_type >::iterator iterator;
      typedef typename std::map<key_type,value_type >::const_iterator const_iterator;
      
      iterator begin() { return container.begin(); }
      iterator end()   { return container.end();   }
      const_iterator begin() const { return container.begin(); }
      const_iterator end()   const { return container.end();   }

      void AddKeyValue(const key_type& key, const value_type& value) {
        container[key] = value;
      }
      
      value_type& operator[](const key_type& key) {
        return container[key];
      }
      const_iterator find(const key_type& key) const { return container.find(key); }
      iterator find(const key_type& key) { return container.find(key); }
      int erase(const key_type& key) { return container.erase(key); }
      iterator erase(const_iterator first, const_iterator last) { return container.erase(first,last); }
      
      std::map<key_type,value_type > container;
    };
    std::ostream& operator<< (std::ostream& os, const MockContainer& container) {
      for( auto& m : container ) {
        os << m.first << "\t:\t";
        std::copy(m.second.begin(), m.second.end(), std::ostream_iterator<std::string>(os, ","));
        os << std::endl;
      }
      return os;
    }

        
    struct MockDataManager : DataManager {
      typedef  MockDataManager self_t;

      MockDataManager() { };
      void Dump(std::ostream& os=std::cout) { os << container << std::endl; }
      
      self_t& AddConfig(std::string config) {
        rapidjson::Document t;
        t.Parse(config.c_str());
        if( t.HasParseError() ) { throw std::runtime_error("Error: invalid configuration"); }
        scan(t.MemberBegin(),t.MemberEnd());
        return *this;
      }
      
      MockContainer::value_type Query(const std::string& key) const {
        if( KeyExists(key) )
          return ReturnValue(key);
        std::cerr << "Key " << key << " doesn't exists" << std::endl;
        return MockContainer::value_type();
      }
      
      bool Update(const std::string& key, const std::string& value) {
        if( !KeyExists(key) )
          return false;
        if( IsSet(key) ) {
          //          container[key].push_back(value);

          std::size_t found = key.find_last_of(":");
          std::cout << key.substr(0,found) << " : " << key.substr(found+1) << std::endl;

          throw std::runtime_error("Set update not implemented");
        }
        else {
          container[key][0] = value;
        }
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
          // if ( !RemoveFromParent(key.substr(0,found),key.substr(found+1)) )
          //   return false;
          // remove key and children
          int n=RemoveChildren(key);
          if( n <= 0 )
            return false;

          deleted_keys+=n;
          //          throw std::runtime_error("Set delete not implemented");
          return deleted_keys > 0;
        }
        else {
          updates.push_back(std::pair<std::string,std::string>(key,"d"));
          //          return container.erase(key) == 1;
          return RemoveKey(key)==1;
        }
        return false;
      }
      
      bool Notify() {
        std::cout << "Configuration changes:\n";
        while (!updates.empty()) {
          std::cout << "\t("     << updates.back().second
                    << ")\t" << updates.back().first
                    << std::endl;
          updates.pop_back();
        }
        return updates.empty();
      };
       std::vector<std::pair<std::string,std::string> > const& UpdatesList()  {
        return updates;
      };
      
      bool IsValidString(std::string s) {
        return !rapidjson::Document().Parse(s.c_str()).HasParseError();
      }
      
    private:
      
      MockContainer container;
      std::vector<std::pair<std::string,std::string> > updates;
      
      // bool ConfigAlreadyPresent(const std::string& key) {
      //   return false;//d.HasMember(key.c_str());
      // }
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

      bool RemoveFromParent(const std::string& key,const std::string& value) {
        // auto& l = container.find(key)->second;
        // for( auto v = l.begin(); v != l.end(); ++v) {
        //   std::cout << (*v) << "\t";
        //   if ( (*v) == key ) {
        //     l.erase(v);
        //     return true;
        //   }
        // }
        // std::cout << "\n";
        // return false;
        // std::cout << "\n";
        // // if parent key is not found return false
        // MockContainer::value_type::const_iterator it=std::find(l.begin(),l.end(),value);
        // std::cout << *it << std::endl;
        // if( it<l.end() ) {
        //   container[key].erase(it->first);
        //   updates.push_back(std::pair<std::string,std::string>(key,"u"));
        //   std::cout << "rimosso!" << std::endl;
        //   return true;
        // }
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
          //          std::cout << last->first << std::endl;
          ++last;
          ++nelem;
        }
        container.erase(first,last);
        return nelem;
      }

      MockContainer::value_type ReturnValue(const std::string& key) const {
        return container.find(key)->second;
      }
      
      void scan(rapidjson::Value::MemberIterator first,
                rapidjson::Value::MemberIterator last,
                std::string prefix="") {

        std::vector<std::string> values;
        std::string separator="";
        if(prefix!="")
          separator=":";
        
        for( auto& it = first; it != last; ++it) {
          values.push_back(std::string(it->name.GetString()));
          if(it->value.IsObject())  // if value is object create a  "set"
            scan( it->value.MemberBegin(),
                  it->value.MemberEnd(),
                  prefix+separator+it->name.GetString());
          else {
            if(it->value.IsString()) { // if value is a string create a "hash-value"
              std::vector<std::string> tmp;
              tmp.push_back(std::string(it->value.GetString()));
              AddToHash(prefix+separator+it->name.GetString(),tmp);
            }
            else
              throw std::runtime_error("Error parsing configuration: value is neither an object nor a string");
          }
        }
        if (prefix=="") { 
          AddToKeyList("source",values);
        }
        else {
          AddToKeyList(prefix,values);
        }
      }
    };
    
    

  }
}

