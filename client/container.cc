#include <container.hpp>

namespace container {
  std::ostream& operator<< (std::ostream& os, const MultimapContainer& container) {
    for( auto& m : container ) {
      os << m.first << "\t:\t";
      for(auto & i : m.second)
        os << i << ",";
      os << std::endl;
    }
    return os;
  }

}
