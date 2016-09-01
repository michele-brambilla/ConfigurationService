#pragma once

#include <iostream>
#include <fstream>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <stdexcept>

namespace configuration {

  namespace data {
    // static const char* kTypeNames[] = 
    //   { "Null", "False", "True", "Object", "Array", "String", "Number" };

    
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



    struct DataManager {
      DataManager() { };
    };
    
    struct MockDataManager : DataManager {
      typedef  MockDataManager self_t;

      MockDataManager() { };
      void Dump(std::ostream& os=std::cout) { os << container << std::endl; }
      
      self_t& AddConfig(std::string conf) {
        rapidjson::Document t;
        t.Parse(conf.c_str());
        if( t.HasParseError() ) { throw std::runtime_error("Error: invalid configuration"); }
        scan(t.MemberBegin(),t.MemberEnd());
        return *this;
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
        //        std::cout << prefix << std::endl;

        // for( auto& it = first; it != last; ++it) {
        //   std::cout << std::string(it->name.GetString()) << std::endl;
        //   if( KeyExists(it->name.GetString()) ) {
        //     throw std::runtime_error("configuration exists");
        //   }
        // }
        //        return false;
        
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
        return true;
      }
    };
    
    

  }
}
