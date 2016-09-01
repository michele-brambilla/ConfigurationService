ConfigurationService
=============

API library to deal with configuration service.
Actions available:
    * UploadConfig
    * DumpConfig
    * SubscribeToKey
    * GetKeyValue
    * UpdateKeyValue
    

REDIS
-----

Requires hiredis, libev for redox 
```
yum install hiredis libev
git clone git@github.com:hmartiro/redox.git redox
```

Testing
------

Run single test

./test/data_test --gtest_filter="*UpdateSet*"
