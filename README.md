ConfigurationService
=============

API library to deal with configuration service.
Actions available:
    * UploadConfig
    * DumpConfig
    * SubscribeToKey
    * GetKeyValue
    * UpdateKeyValue
    
Install
-----
Some libraries are submodules:
    * rapidjson
    * redox
To clone correctly:
```
git submodule init
git submodule update
```
Building the submodules is part of the standard build system.
Building system uses cmake, no particular options required


REDIS 
-----

Requires hiredis, libev for redox 
```
yum install hiredis libev
```

Communicator
------------

Base classes contained in ``basic_communicator.hpp". 
Actions available
    * Publish (topic,status)
    * Notify
    * Subscribe (topic) : arguments can be a string (topic), string +
      std::function< void(const std::string &,const std::string &) > topic +
      callback on message received, string + std::function< void(const
      std::string &,const std::string &) >+std::function< void(const std::string
      &,const std::string &) >+std::function< void(const std::string &,const
      std::string &) > topic+callback on message+callback on subscribe+callback
      on error
    * Unsubscribe:  arguments can be a string (topic), string +
      std::function< void(const std::string &,const std::string &) > topic +
      callback on error
    

Testing
------

Run single test

./test/data_test --gtest_filter="*UpdateSet*"
