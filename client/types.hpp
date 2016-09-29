#pragma once

#include <vector>
#include <string>
#include <map>

typedef std::string key_type;
typedef std::vector<std::string> value_type;

namespace container {
  
  typedef typename std::multimap<key_type,value_type > container_t;
  typedef typename container_t::iterator iterator;
  typedef typename container_t::const_iterator const_iterator;
  typedef typename container_t::value_type value_type;

}
