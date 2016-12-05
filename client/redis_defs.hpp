#include<string>
#include<vector>

namespace configuration {
  namespace utils {
    struct typelist
    {
      typedef int SADD_t; //reply of type 3
      typedef int DEL_t; //reply of type 3
      
      typedef std::string TYPE_t; //reply of type 2
      
      typedef std::string GET_t; //reply of type 2
      typedef std::string SET_t; //reply of type 2
      typedef std::string VALUE_t; //reply of type 2
      
      typedef std::vector<std::string> KEYS_t; //reply of type 1 (or 5?)
      typedef std::vector<std::string> LIST_t; //output of ExecRedisCmd
      
    };
  }
}
