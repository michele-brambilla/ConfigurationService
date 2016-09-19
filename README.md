[![Build Status](https://travis-ci.org/michele-brambilla/ConfigurationService.svg?branch=master)](https://travis-ci.org/michele-brambilla/ConfigurationService)

ConfigurationService
=============

Stores and retrieve configuration information on a Redis db running on some
server. Users can subscribe to channel(s) (key) to be informed on updates.
By default the messages are stored and sent after a scheduled time, but
notifications can be forced.

Available actions:

    * AddConfig: submit a new configuration on the server. The configuration has
      to be in json format. If the configuration (or any subitem) exists,
      reports an error and do not submit
    * Query: returns the value of a key. It can be a single value (if there are
      no subkeys) or a set of values (else)
    * Update: update a key with a new value. If the key doesn't exists it is
      added and its parent is updated with the new value
    * Delete: delete a key and updates its parent
    * Notify: force notification of changes to subscribed clients
    * Subscribe: listen on a channel for related key changes

The command line executable *Service* can be run as a standalone configuration
manager or as an example.
Usage:
```
./Service [ -d <data address>[:<data port>] -c <communication address>[:<communication port>] ]
```
After execution it expects one ot the previous commands (eventually with the
key). Particular cases:

    * query: key "*" dumps all the configuration (key-value(s) )
    * update (u): requires <key>=<value>
    * notify: no arguments
    

Install
-----
Requires submodules:
    * rapidjson
    * redox
To clone correctly:
```
git submodule init
git submodule update
```
Requires hiredis, libev 
```
yum install hiredis-devel libev-devel
```

Building the submodules is part of the standard build system.
Building system uses cmake, no particular options required. The code makes use of c++11 standard. Works with gcc >= 4.8 and clang >= 3.5. 
For testing purposes Googletest is required.


Communicator
------------

Base classes contained in ``basic_communicator.hpp".

Actions available

    * Publish (topic,status)
    * Notify
    * Subscribe (topic) : arguments can be a
      * topic
      * topic + callback on message received
      * topic + callback on message + callback on subscribe + callback on error
    * Unsubscribe:  arguments can be a
      - string: topic
      - topic + callback on error

Opens a PUB/SUB communication channel to publish changes (NOT automatically
sent) and notify (send, actually) updates. After subscription to a channel any
update made by other clients is notified (and callbacks can trigger actions on event).

DataManager
------------

Base classes contained in ``basic_data.hpp".
Actions available:

    * AddConfig (config)
    * Update (key, new value)
    * Delete (key)
    * Query(key)

Connects to a REDIS database, allow to upload a new configuration, change the
value of a key or add a new one, delete a key or query the value (or set of
values) of a key.

Testing
------

Available tests for CommunicatorManager, DataManager and Configuration
manager. Requires *googletest-devel*.

    * Tests can be run using *ctest* or as standalone command line programs.
    * Tests can't be run using *gtester*

Note: a single test or a subset can be executed using
*--gtest_filter="<string>"* :

```./test/data_test --gtest_filter="*UpdateSet*"```

Test are not part of the Travis-ci building, due to timeout cause failure
