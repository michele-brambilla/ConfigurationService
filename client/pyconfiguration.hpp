#include <boost/python.hpp>
using namespace boost::python;

BOOST_PYTHON_MODULE(configuration)
{
  class_<ConfigurationManager>("ConfigurationManager", init<const char*,const int&, const char*,const int&,std::ostream&>())
    .add_property("AddConfig", &World::AddConfig)
    .add_property("Update", &World::Update);
}
