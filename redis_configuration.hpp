#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <functional>
#include <algorithm>
#include <future>

#include <redis_utils.hpp>

#include <basic_communicator.hpp>
#include <basic_data.hpp>


namespace configuration {


  namespace data {
    
    /////////////////////
    // Redis data manager
    template<typename Communicator>
    struct RedisDataManager  : public DataManager
    {
      typedef RedisDataManager self_t;
      int connection_status;
      
      RedisDataManager(const std::string& redis_server,
                       const int& redis_port,
                       Communicator& communicator,
                       std::ostream& logger=std::cerr) : address(redis_server), port(redis_port),
        rdx(std::make_shared<redox::Redox>(std::cout,redox::log::Level::Fatal)),
        log(logger), updates(communicator) {
        
        auto f = std::bind(utils::redis_connection_callback,
                           std::placeholders::_1,
                           std::ref(connection_status));
        rdx->connect(address, port, f );
        
        if( connection_status != redox::Redox::CONNECTED ) {
          log << "Can't connect to REDIS server\n";
          throw std::runtime_error("Can't connect to REDIS server: error"+std::to_string(connection_status));
        }
        
      };
      
      ~RedisDataManager() {
        rdx->disconnect();
      }

      void Disconnect() {
        rdx->disconnect();
        if( connection_status != redox::Redox::DISCONNECTED ) {
          log << "Can't disconnect from REDIS server: error "+std::to_string(connection_status) << std::endl;
        }
      }

      void Dump(std::ostream& os=std::cout) {
        utils::typelist::KEYS_t result;
        std::string s = {"KEYS *"};
        if( utils::ExecRedisCmd<utils::typelist::KEYS_t>(*rdx,s,result) )
          for( auto r : result) {
            os << r << std::endl;
            for( auto v : ReturnValue(r) )
              os << "\t" << v;
            os << "\n";
          }
        else
          os << "No keys in database" << std::endl;
      }
      
      
      void Clear() override { rdx->command<std::string>({"FLUSHALL"}); }
      
      redox::Redox& redox() { return rdx; } 

    private:

      std::string address;
      int port;
      std::shared_ptr<redox::Redox> rdx;
      std::ostream& log;
      Communicator& updates;

      void Reconnect() {
        log << "Error: can't connect to redis database. Trying reconnect" << std::endl;
        auto f = std::bind(utils::redis_connection_callback,
                           std::placeholders::_1,
                           std::ref(connection_status));
        rdx = std::make_shared<redox::Redox>(std::cout,redox::log::Level::Fatal);
        rdx->connect(address, port, f);
      }

      
      bool KeyExists (const std::string& key) override {
        std::string s="KEYS "+key;
        utils::typelist::KEYS_t result;
        utils::ExecRedisCmd<utils::typelist::KEYS_t>(*rdx,s,result);
        return result.size()>0;
      }

      bool HExists(const std::string& hash,const std::string& key) {
    	std::string cmd = {"HEXISTS "+hash+" "+key};
    	int value = 0;
    	utils::ExecRedisCmd<int>(*rdx,cmd,value);
    	return value;
      }
      bool SIsMember(const std::string& hash,const std::string& key) {
    	std::string cmd = {"SISMEMBER "+hash+" "+key};
    	int value = 0;
    	utils::ExecRedisCmd<int>(*rdx,cmd,value);
    	return value;
      }

      bool IsHash(const std::string& key) { 
	std::string key_type;
	utils::ExecRedisCmd<std::string>(*rdx,std::string("TYPE ")+key,key_type);
	return key_type == "hash";
      }

      bool AddToHash(const std::string& key,
                     const std::vector<std::string>& value) override {
        bool is_ok = true;
        bool ok;
        for(auto& v : value) {
          ok = AddToHash(key,v);
          if( ok )
            updates.Publish(key,"a");
          is_ok &= ok;
        }
        return is_ok;
      }
      bool AddToHash(const std::string& key, const std::string& value) override {
        std::size_t found = key.find_last_of(":");
        updates.Publish(key,"a");
        return utils::ExecRedisCmd<int>(*rdx,std::string("HSET ")+key.substr(0,found)+" "+key.substr(found+1)+" "+value);
      }
      
      bool RemoveFromParent(const std::string& parent,const std::string& name) override {
        bool is_ok = true;
        bool ok;
        ok = utils::ExecRedisCmd<utils::typelist::DEL_t>(*rdx,std::string("SREM ")+parent+" "+name);
        if( ok )
          updates.Publish(parent,"u");
        is_ok &= ok;
        // if parents gets empty, delete
        int nelem;
        utils::ExecRedisCmd<int>(*rdx,std::string("SCARD ")+parent,&nelem);
        if (nelem == 0) 
          is_ok &=  (RemoveKey(parent) > 0) ;
	return is_ok;
      }
      int RemoveChildren(const std::string& key) override {
        utils::typelist::KEYS_t children_list;
        std::string cmd="KEYS "+key+":*";
        utils::ExecRedisCmd<utils::typelist::KEYS_t>(*rdx,cmd,children_list);
        for( auto& c : children_list ) {
          if ( utils::ExecRedisCmd<utils::typelist::DEL_t>(*rdx,"DEL "+c) )
            updates.Publish(c,"d");
        }          
        return children_list.size();
      }


      bool RemoveKey(const std::string& key) override {
        updates.Publish(key,"d");
        std::string key_type;
	bool is_ok = utils::ExecRedisCmd<std::string>(*rdx,std::string("TYPE ")+key,key_type);
        bool ok;

        if(key_type == "set") {
          // remove children
          int nelem;
          utils::ExecRedisCmd<int>(*rdx,std::string("SCARD ")+key,&nelem);
          is_ok &= ( (RemoveChildren(key)>0 && nelem >0) ? true : false);
        }
	
        // remove from parent
        std::size_t found = key.find_last_of(":");
        is_ok &= RemoveFromParent(key.substr(0,found),key.substr(found+1));
        ok = utils::ExecRedisCmd<int>(*rdx,std::string("DEL ")+key);
        if(ok)  updates.Publish(key,"d");

        return (is_ok && ok);
      }
      
      bool RemoveHash(const std::string& hash,const std::string& key) override {
        if(utils::ExecRedisCmd<int>(*rdx,std::string("HDEL ")+hash+" "+key)) {
	  updates.Publish(key,"d");
	  return true;
	}
        return false;
      }
      
      bool UpdateHashValue(const std::string& key,const std::string& value) override {
        std::size_t found = key.find_last_of(":");
        if (found == std::string::npos ) 
	  return false;
        std::string cmd = std::string("HSET ")+key.substr(0,found)+" "+key.substr(found+1)+" "+value;
        updates.Publish(key,"u");
        return utils::ExecRedisCmd<int>(*rdx,cmd);
      }


      bool AddToParent(const std::string& key,const std::string& value) override {
        std::string cmd=std::string("SADD ")+key+" "+value;
        updates.Publish(key,"u");
        return utils::ExecRedisCmd<int>(*rdx,cmd);;
      }

      bool UpdateParent(const std::string& key) override {
        log << "Update parents: " << key << std::endl;
        bool is_ok = true;
        std::size_t found = key.find_last_of(":");
        if (found == std::string::npos ) return true;
        std::string parent_key=key.substr(0,found);
        std::string parent_value=key.substr(found+1);

        is_ok &= AddToParent(parent_key,parent_value);
        if( KeyExists(parent_key) ) {
          is_ok &= UpdateParent(parent_key);
        }
        return is_ok;
      }

      
      utils::typelist::LIST_t ReturnValue(const std::string& key) override {
        utils::typelist::LIST_t result;
        std::string key_type;
	std::string tmp;
        
	if( !KeyExists(key) ) { //must be a key /in hash
	  std::size_t found = key.find_last_of(":");
	  utils::ExecRedisCmd<std::string>(*rdx,std::string("TYPE ")+key.substr(0,found),key_type);
	  if(key_type != "hash" || (utils::ExecRedisCmd<utils::typelist::VALUE_t>(*rdx,std::string("HGET ")+key.substr(0,found)+" "+key.substr(found+1),tmp) == 0 ) )
	    return result; //error
	  result.push_back(tmp);
        }
	else {
	  utils::ExecRedisCmd<std::string>(*rdx,std::string("TYPE ")+key,key_type);	  
	  if(key_type == "set") {
	    utils::ExecRedisCmd<utils::typelist::LIST_t>(*rdx,std::string("SMEMBERS ")+key,result);
	    // removes eventual empty items
	    //	    result.erase( std::remove( result.begin(), result.end(), " " ), result.end() );
	  }
          else {
	    if( key_type == "hash") {
	      utils::ExecRedisCmd<utils::typelist::LIST_t>(*rdx,std::string("HKEYS ")+key,result);
	    }
	    else {
	      if(key_type == "string") {
		utils::ExecRedisCmd<utils::typelist::VALUE_t>(*rdx,std::string("GET ")+key,tmp);
		result.push_back(tmp);
	      }
	      else {
		//            throw std::runtime_error("Type "+key_type+" not supported");
		log << "Type "+key_type+" not supported";
	      }
	    }
	  }
	}
	return result;
      }
    

      // bool scan(rapidjson::Value::MemberIterator first,
      //           rapidjson::Value::MemberIterator last,
      //           std::string prefix="") override {
  
      //   bool is_ok = true;
      //   std::string separator="";
      //   if(prefix!="")
      //     separator=":";
  
      //   for( auto& it = first; it != last; ++it) {
      //     is_ok &= (!KeyExists(prefix+separator+std::string(it->name.GetString())) );
      //     if(it->value.IsObject()) {
      //       std::string s = "SADD "+ prefix+separator+std::string(it->name.GetString())+std::string(" ");
      //       for( auto v = it->value.MemberBegin();
      //            v != it->value.MemberEnd(); ++v)
      //         is_ok &= utils::ExecRedisCmd<utils::typelist::SADD_t>(*rdx,s+v->name.GetString());
            
      //       is_ok &= scan(it->value.MemberBegin(),
      //                     it->value.MemberEnd(),
      //                     prefix+separator+std::string(it->name.GetString()));
      //     }
      //     else {
      //       std::string s = "SET "+prefix+separator+std::string(it->name.GetString())+std::string(" ");
      //       s+= it->value.GetString();
      //       if(is_ok)
      //         utils::ExecRedisCmd<utils::typelist::SET_t>(*rdx,s);
      //       else
      //         log << "Key " << std::string(it->value.GetString()) << " exists, not added\n";

      //     }
      //   }
      //   return is_ok;
      // }



      
      bool redis_json_scan(GenericMemberIterator& member,std::string prefix) {

    	bool is_ok = true;
    	if( prefix.size() == 0)
    	  prefix=std::string(member.name.GetString());
    	else
    	  prefix+=":"+std::string(member.name.GetString());
	
    	for (auto& next : member.value.GetObject()) {
    	  if( next.value.IsObject() ) {
    	    if( !SIsMember(prefix,next.name.GetString())) {
    	      std::string cmd = {"SADD "+prefix+" "+next.name.GetString()};
    	      is_ok &= utils::ExecRedisCmd<int>(*rdx,cmd);
    	    }
    	    else {
	      is_ok = false;
    	      log << "\tprefix"   << " " 
		  << next.name.GetString() << " exists"
		  << std::endl;
    	    }
    	    redis_json_scan(next,prefix);
    	  }
    	  else {
    	    if( !HExists(prefix,next.name.GetString()) ) {
	      is_ok = false;
    	      std::string cmd = {"HSET "+prefix+
    				 " "+next.name.GetString()+
    				 " "+next.value.GetString()};
    	      is_ok &= utils::ExecRedisCmd<int>(*rdx,cmd);
    	    }
    	    else {
	      is_ok = false;
    	      log << "\tprefix"   << " " 
		  << next.name.GetString() << " " 
		  << next.value.GetString()  << " exists"
		  << std::endl;
	    }

    	  }
    	}
    	return is_ok;
      }

    };

  }




  namespace communicator {

    
    struct RedisCommunicator : public Communicator {
      static int MaxStoredMessages;
      int publisher_connection_status;
      int subscriber_connection_status;
      
      RedisCommunicator(const std::string& server,
                        const int& port=6379,
                        std::ostream& logger=std::cerr,
                        redox::log::Level redox_loglevel=redox::log::Level::Fatal) 
        : publisher(std::make_shared<redox::Redox>(logger,redox::log::Level::Fatal)),
          subscriber(std::make_shared<redox::Subscriber>(logger,redox::log::Level::Fatal)),
          log(logger),
          redis_server(server),
          redis_port(port)
      {
        publisher->connect(redis_server, redis_port,
                           std::bind(utils::redis_connection_callback,
                                     std::placeholders::_1,
                                     std::ref(publisher_connection_status)));
        subscriber->connect(redis_server, redis_port,
                            std::bind(utils::redis_connection_callback,
                                      std::placeholders::_1,
                                      std::ref(subscriber_connection_status)));
        
        if( (publisher_connection_status != redox::Redox::CONNECTED) ||
            (subscriber_connection_status != redox::Redox::CONNECTED) ) {
          log << "Communicator can't connect to REDIS\n";
          throw std::runtime_error("Can't connect to REDIS server");
        }
        t=std::move(std::thread(&RedisCommunicator::AutoNotification,this));
        
      }

      ~RedisCommunicator() {
        subscriber->disconnect();
        publisher->disconnect();
        keep_counting = false;
        t.join();
      }


      void Disconnect() {
        publisher->disconnect();
        subscriber->disconnect();
        if(  (publisher_connection_status != redox::Redox::DISCONNECTED) ||
             (subscriber_connection_status != redox::Redox::DISCONNECTED) ) {
          throw std::runtime_error("Can't disconnect from REDIS server: error "+
                                   std::to_string(publisher_connection_status)+","+
                                   std::to_string(subscriber_connection_status) );
        }
      }
      
      bool Notify() override {
        std::string cmd = "PUBLISH ";
        int nclients;
        bool is_ok = true;
        for(auto& msg : updates) {
          is_ok &= utils::ExecRedisCmd<int>(*publisher,
                                            cmd+msg.first+" "+msg.second,
                                            &nclients);
          if( nclients == 0 )
            log << msg.first << ": no connected clients\n";
        }
        updates.clear();
        return is_ok;
      }

      bool Subscribe(const std::string& key) {
        bool is_ok = true;
        
        auto got_message = [&](const std::string& topic, const std::string& msg) {
          total_recv_messages++;
          this->log << topic << ": " << msg << std::endl;
        };
        auto  subscribed = [&](const std::string& topic) {
          this->log << "> Subscribed to " << topic << std::endl;
        };
        auto unsubscribed = [&](const std::string& topic) {
          this->log << "> Unsubscribed from " << topic << std::endl;
        };
        auto got_error = [&](const std::string& topic, const int& id_error) {
          this->log << "> Subscription topic " << topic << " error: " << id_error << std::endl;
          is_ok = false;
        };
        
        if ( (key).find("*")!=std::string::npos)
          subscriber->psubscribe(key, got_message, subscribed, unsubscribed, got_error);
        else
          subscriber->subscribe(key, got_message, subscribed, unsubscribed, got_error);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        return is_ok;
      }
      
      bool Subscribe(const std::string& key,
                     std::function<void(const std::string&,const std::string&)> got_message,
                     std::function<void(const std::string&,const int&)> got_error = default_got_error,
                     std::function<void(const std::string&)> unsubscribed = default_unsubscribed
                     ) override {

        log << "called subscribe (3) " << std::endl;
        auto subscribed = [&](const std::string& topic) {
          this->log << "> Subscribed to " << topic << std::endl;
        };

        if ( (key).find("*")!=std::string::npos) {
          log << "called psubscribe " << std::endl;
          subscriber->psubscribe(key, got_message, subscribed, unsubscribed, got_error);
        }
        else {
          subscriber->subscribe(key, got_message, subscribed, unsubscribed, got_error);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          std::set<std::string> topic_list = subscriber->subscribedTopics();	
          if( topic_list.find(key) == topic_list.end() )
            return false;
        }
        
        return true;
      }

      
      bool Unsubscribe(const std::string& key,
                       std::function<void(const std::string&,const int&)> got_error = default_got_error
                       ) override {
        bool is_ok = true;
        std::size_t found = key.find("*");

        if ( found != std::string::npos) {
          std::string short_key(key);
          short_key.pop_back();
          auto list = subscriber->psubscribedTopics();
          if ( list.find(short_key) == list.end() )
            return false;
          subscriber->punsubscribe(short_key.substr(0,found-1),got_error);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          list = subscriber->psubscribedTopics();
          if ( list.find(short_key) == list.end() )
            return false;

        }
        else {
          subscriber->unsubscribe(key,got_error);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          for( auto& s : subscriber->subscribedTopics())
            if( s == key )
              is_ok = false;
        }

        return is_ok;
      }

      std::set<std::string> ListTopics() {
        std::set<std::string> s = subscriber->subscribedTopics();
        s.insert(subscriber->psubscribedTopics().begin(),
                 subscriber->psubscribedTopics().end()
                 );
        return s;
      }

      bool Reconnect() {

        publisher->connect(redis_server, redis_port,
                          std::bind(utils::redis_connection_callback,
                                    std::placeholders::_1,
                                    std::ref(publisher_connection_status)));
        subscriber->connect(redis_server, redis_port,
                           std::bind(utils::redis_connection_callback,
                                     std::placeholders::_1,
                                     std::ref(subscriber_connection_status)));

        return ( (publisher_connection_status != redox::Redox::CONNECTED) &&
                 (subscriber_connection_status != redox::Redox::CONNECTED) ) ;
      }
      
      bool keep_counting = true;      
    private:
      std::shared_ptr<redox::Redox> publisher;
      std::shared_ptr<redox::Subscriber> subscriber;
      
      std::ostream& log;

      std::string redis_server;
      int redis_port;
      std::thread t;
      
      void AutoNotification() {
        while(this->keep_counting) {
          std::this_thread::sleep_for(std::chrono::seconds(NotificationTimeout));
          log << NotificationTimeout << "s elapsed, auto-notification will occur\n";
          Notify();
        }
        log << "TimedNotification terminated\n";
      }

      
      void count_got_message(const std::string & t,const std::string & c)  {
        std::cerr << "== "<< t << " : " << c <<" ==" << std::endl;
        this->total_recv_messages++;
      } 
      
    };
    
  }
}
  
