#pragma once

#include <iostream>

#include <types.hpp>

namespace container {
  
  // struct MockContainer {
    
  //   iterator begin() { return container.begin(); }
  //   iterator end()   { return container.end();   }
  //   const_iterator begin() const { return container.begin(); }
  //   const_iterator end()   const { return container.end();   }
  
  //   void AddKeyValue(const std::string& key, const std::string& value) {
  //     container.insert(value_t(key,value));
  //   }
  
  //   std::string& operator[](const std::string& key) {
  //     // return container[key];
  //   }
    
  //   const_iterator find(const std::string& key) const { return container.find(key); }
  //   iterator find(const std::string& key) { return container.find(key); }
  //   int erase(const std::string& key) { return container.erase(key); }
  //   iterator erase(const_iterator first, const_iterator last) { return container.erase(first,last); }
  
  //   container_t container;
  // };
  // std::ostream& operator<< (std::ostream& os, const MockContainer& container) {
  //   for( auto& m : container ) {
  //     os << m.first << "\t:\t";
  //     std::copy(m.second.begin(), m.second.end(), std::ostream_iterator<std::string>(os, ","));
  //     os << std::endl;
  //   }
  //   return os;
  // }



  typedef typename std::multimap<std::string,std::string> MultimapContainer;
  std::ostream& operator<< (std::ostream&, const MultimapContainer&);
  // std::ostream& operator<< (std::ostream& os, const MultimapContainer& container) {
  //   for( auto& m : container ) {
  //     os << m.first << "\t:\t";
  //     for(auto & i : m.second)
  //       os << i << ",";
  //     //      std::copy(m.second.begin(), m.second.end(), std::ostream_iterator<std::string>(os, ","));
  //     os << std::endl;
  //   }
  //   return os;
  // }

  
}
