#pragma once

#include <vector>
#include <string>
#include <map>

typedef std::string key_type;
typedef std::vector<std::string> value_type;
typedef typename std::map<key_type,value_type >::iterator iterator;
typedef typename std::map<key_type,value_type >::const_iterator const_iterator;
