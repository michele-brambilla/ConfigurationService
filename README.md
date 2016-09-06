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

REDIS data manager
-----

Requires hiredis, libev for redox 
```
yum install hiredis libev
```

Testing
------

Run single test

./test/data_test --gtest_filter="*UpdateSet*"
