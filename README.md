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
Building system uses cmake, no particular options required. Caveat: the code makes use of c++11 standard. Works with gcc >= 4.8 and clang >= 3.5. So far it failed with gcc 4.7.

For testing purposes Googletest is required.


REDIS
-----

Requires hiredis, libev for redox
```
yum install hiredis-devel libev-devel
```
If not present in the repository:

    Download the latest epel-release rpm from
    http://dl.fedoraproject.org/pub/epel/6/i386/
    Install epel-release rpm:
    # rpm -Uvh epel-release*rpm
    Install hiredis rpm package:
    # yum install hiredis


Alternative: dowload it from pkg servers:
```
http://dl.fedoraproject.org/pub/epel/7/x86_64/h/hiredis-0.12.1-1.el7.x86_64.rpm
http://dl.fedoraproject.org/pub/epel/7/x86_64/h/hiredis-devel-0.12.1-1.el7.x86_64.rpm
```
and install
```
rpm -i hiredis-0.12.1-1.el7.x86_64.rpm
rpm -i hiredis-devel-0.12.1-1.el7.x86_64.rpm
```
(this worked for me on the vagrant machine)


Communicator
------------

Base classes contained in ``basic_communicator.hpp".
Actions available
    * Publish (topic,status)
    * Notify
    * Subscribe (topic) : arguments can be a
      - string: topic
      - string + std::function< void(const std::string &,const std::string &) >: topic +
      callback on message received
      > string + std::function< void(const std::string &,const std::string &) >+std::function< void(const std::string
      &,const std::string &) >+std::function< void(const std::string &,const
      std::string &) >: topic+callback on message+callback on subscribe+callback
      on error
    * Unsubscribe:  arguments can be a
      - string: topic
      - string +
      std::function< void(const std::string &,const std::string &) >: topic +
      callback on error


Testing
------

Run single test

./test/data_test --gtest_filter="*UpdateSet*"
