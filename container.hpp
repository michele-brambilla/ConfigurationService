#pragma once

#include <iostream>

#include <types.hpp>

namespace container {
  
  struct MockContainer {
    
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

}
