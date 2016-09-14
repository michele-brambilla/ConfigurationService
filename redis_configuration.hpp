#pragma once

#include <iostream>
#include <fstream>
#include <stdexcept>
#include <map>
#include <functional>
#include <algorithm>

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

      RedisDataManager(const std::string& redis_server,
                       const int& redis_port,
                       Communicator& communicator,
                       std::ostream& logger=std::cerr) : rdx(std::cout,redox::log::Level::Fatal), log(logger), updates(communicator) {
        if( !rdx.connect(redis_server, redis_port) ) {
          log << "Can't connect to REDIS server\n";
          throw std::runtime_error("Can't connect to REDIS server");
        }
      };

      void Dump(std::ostream& os=std::cout) {
        std::cout << "DUMP " << std::endl;
        utils::typelist::KEYS_t result;
        std::string s = {"KEYS *"};
        if( utils::ExecRedisCmd<utils::typelist::KEYS_t>(rdx,s,result) )
          for( auto r : result) {
            os << r << std::endl;
            for( auto v : ReturnValue(r) )
              os << "\t" << v;
            os << "\n";
          }
        else
          throw std::runtime_error("No keys in db");
      }

      
      void Clear() override { rdx.command<std::string>({"FLUSHALL"}); }

      redox::Redox& redox() { return rdx; } 
     
    private:

      redox::Redox rdx;
      std::ostream& log;
      Communicator& updates;
      

      bool KeyExists (const std::string& key) override {
        std::string s="KEYS "+key;
        utils::typelist::KEYS_t result;
        utils::ExecRedisCmd<utils::typelist::KEYS_t>(rdx,s,result);
        return result.size()>0;
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
        //        updates.push_back(std::pair<std::string,std::string>(key,"a"));
        updates.Publish(key,"a");
        return rdx.set(key,value);
      }
      
      bool RemoveFromParent(const std::string& parent,const std::string& name) override {
        bool is_ok = true;
        bool ok;
        ok = utils::ExecRedisCmd<utils::typelist::DEL_t>(rdx,std::string("SREM ")+parent+" "+name);
        if( ok )
          updates.Publish(parent,"u");
        is_ok &= ok;
        // if parents gets empty, delete
        int nelem;
        utils::ExecRedisCmd<int>(rdx,std::string("SCARD ")+parent,&nelem);
        if (nelem == 0) 
          is_ok &=  (RemoveKey(parent) > 0) ;
        
        return is_ok;
      }
      int RemoveChildren(const std::string& key) override {
        utils::typelist::KEYS_t children_list;
        std::string cmd="KEYS "+key+":*";
        utils::ExecRedisCmd<utils::typelist::KEYS_t>(rdx,cmd,children_list);
        for( auto& c : children_list ) {
          if ( utils::ExecRedisCmd<utils::typelist::DEL_t>(rdx,"DEL "+c) )
            updates.Publish(c,"d");
        }          
        return children_list.size();
      }

      bool RemoveKey(const std::string& key) override {
        //updates.push_back(std::pair<std::string,std::string>(key,"d"));   
        updates.Publish(key,"d");
        std::string key_type;
        utils::ExecRedisCmd<std::string>(rdx,std::string("TYPE ")+key,key_type);
        bool is_ok = true;
        
        if(key_type == "set") {
          // remove children
          int nelem;
          utils::ExecRedisCmd<int>(rdx,std::string("SCARD ")+key,&nelem);
          is_ok &= ( (RemoveChildren(key)>0 && nelem >0) ? true : false);
        }
        
        // remove from parent
        std::size_t found = key.find_last_of(":");
        // std::string parent_key=key.substr(0,found);
        // std::string parent_value=key.substr(found+1);
        is_ok &= RemoveFromParent(key.substr(0,found),key.substr(found+1));
        bool ok;
        is_ok &= (ok = utils::ExecRedisCmd<int>(rdx,std::string("DEL ")+key) );
        if(ok) {
          updates.Publish(key,"d");
        }
        return is_ok;
        
      }

      
      bool UpdateHashValue(const std::string& key,const std::string& value) override {
        std::string cmd = std::string("SET ")+key+" "+value;
        updates.Publish(key,"u");
        return utils::ExecRedisCmd<std::string>(rdx,cmd);
      }


      bool AddToParent(const std::string& key,const std::string& value) override {
        std::string cmd=std::string("SADD ")+key+" "+value;
        updates.Publish(key,"u");
        return utils::ExecRedisCmd<int>(rdx,cmd);;
      }

      bool UpdateParent(const std::string& key) override {
        bool is_ok = true;
        std::size_t found = key.find_last_of(":");
        std::string parent_key=key.substr(0,found);
        std::string parent_value=key.substr(found+1);
        if( !KeyExists(parent_key) ) {
          is_ok &= UpdateParent(parent_key);
          is_ok &= AddToParent(parent_key,parent_value);
          return is_ok;
        }

        return AddToParent(parent_key,parent_value);
      }

      
      utils::typelist::LIST_t ReturnValue(const std::string& key) override {
        utils::typelist::LIST_t result;
        if( !KeyExists(key) ) {
          log << "Key " << key << " doesn't exists\n";
          return result;
        }
        
        std::string key_type;
        utils::ExecRedisCmd<std::string>(rdx,std::string("TYPE ")+key,key_type);

        if(key_type == "set") {
          utils::ExecRedisCmd<utils::typelist::LIST_t>(rdx,std::string("SMEMBERS ")+key,result);
          // removes eventual empty items
          result.erase( std::remove( result.begin(), result.end(), " " ), result.end() );
        }
        else
          if(key_type == "string") {
            std::string tmp;
            utils::ExecRedisCmd<utils::typelist::VALUE_t>(rdx,std::string("GET ")+key,tmp);
            result.push_back(tmp);
          }
          else {
            //            throw std::runtime_error("Type "+key_type+" not supported");
            log << "Type "+key_type+" not supported";
          }
            
        return result;
      }


      bool scan(rapidjson::Value::MemberIterator first,
                rapidjson::Value::MemberIterator last,
                std::string prefix="") override {
  
        bool is_ok = true;
        std::string separator="";
        if(prefix!="")
          separator=":";
  
        for( auto& it = first; it != last; ++it) {
          is_ok &= (!KeyExists(prefix+separator+std::string(it->name.GetString())) );
          if(it->value.IsObject()) {
            std::string s = "SADD "+ prefix+separator+std::string(it->name.GetString())+std::string(" ");
            for( auto v = it->value.MemberBegin();
                 v != it->value.MemberEnd(); ++v)
              is_ok &= utils::ExecRedisCmd<utils::typelist::SADD_t>(rdx,s+v->name.GetString());
            
            is_ok &= scan(it->value.MemberBegin(),
                          it->value.MemberEnd(),
                          prefix+separator+std::string(it->name.GetString()));
          }
          else {
            std::string s = "SET "+prefix+separator+std::string(it->name.GetString())+std::string(" ");
            s+= it->value.GetString();
            if(is_ok)
              utils::ExecRedisCmd<utils::typelist::SET_t>(rdx,s);
            else
              log << "Key " << std::string(it->value.GetString()) << " exists, not added\n";

          }
        }
        return is_ok;
      }

    };

  }




  namespace communicator {

    
    struct RedisCommunicator : public Communicator {
      static int MaxStoredMessages;
      static int NotificationTimeout;
      
      RedisCommunicator(const std::string& redis_server,
                        const int& redis_port=6379,
                        std::ostream& logger=std::cerr,
                        redox::log::Level redox_loglevel=redox::log::Level::Fatal) 
        : publisher(logger,redox_loglevel), log(logger),
          last_notify_time(std::chrono::system_clock::now())
          //          t(std::thread(&RedisCommunicator::TimedNotification,this))
      {
        if( !publisher.connect(redis_server, redis_port) ) {
          log << "Publisher can't connect to REDIS\n";
          throw std::runtime_error("Can't connect to REDIS server");
        }
        if( !subscriber.connect(redis_server, redis_port) ) {
          log << "Subscriber can't connect to REDIS\n";
          throw std::runtime_error("Can't connect to REDIS server");
        }
      }

      ~RedisCommunicator() {
        keep_counting = false;
        if(t.joinable())
          t.join();
      }

      bool Notify() override {
        std::string cmd = "PUBLISH ";
        int nclients;
        bool is_ok = true;
        for(auto& msg : updates) {
          is_ok &= utils::ExecRedisCmd<int>(publisher,
                                            cmd+msg.first+" "+msg.second,
                                            &nclients);
          if( nclients == 0 )
            log << msg.first << ": no connected clients\n";
        }
        last_notify_time = std::chrono::system_clock::now();
        updates.clear();
        return is_ok;
      }

      bool Publish(const std::string& key,const std::string& status) override {
        updates[key]=status;
        if(updates.size() > MaxStoredMessages) {
          log << "Max number of stored messages ("+std::to_string(MaxStoredMessages)+") reached, proceed to notify"
              << std::endl;
          return Notify();
        }
        total_num_messages++;
        return true;
      }
      

      bool Subscribe(const std::string& key) override {
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
          subscriber.psubscribe(key, got_message, subscribed, unsubscribed, got_error);
        else
          subscriber.subscribe(key, got_message, subscribed, unsubscribed, got_error);
        
        return is_ok;
      }

      bool Subscribe(const std::string& key,
                     std::function<void(const std::string&,const std::string&)> got_message
                     ) override {

        bool is_ok = true;

        auto subscribed = [&](const std::string& topic) {
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
          subscriber.psubscribe(key, got_message, subscribed, unsubscribed, got_error);
        else
          subscriber.subscribe(key, got_message, subscribed, unsubscribed, got_error);
        
        return is_ok;
      }

      bool Subscribe(const std::string& key,
                     std::function<void(const std::string&,const std::string&)> got_message,
                     std::function<void(const std::string&,const int&)> got_error,
                     std::function<void(const std::string&)> unsubscribed
                     ) override {

        auto subscribed = [&](const std::string& topic) {
          this->log << "> Subscribed to " << topic << std::endl;
        };

        if ( (key).find("*")!=std::string::npos)
          subscriber.psubscribe(key, got_message, subscribed, unsubscribed, got_error);
        else
          subscriber.subscribe(key, got_message, subscribed, unsubscribed, got_error);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        std::set<std::string> topic_list = subscriber.subscribedTopics();	
        if( topic_list.find(key) == topic_list.end() )
          return false;
        
        return true;
      }

      bool Unsubscribe(const std::string& key) override {
        bool is_ok = true;
        auto got_error = [&](const std::string& topic, const int& id_error) {
          this->log << "> Subscription topic " << topic << " error: " << id_error << std::endl;
          is_ok = false;
        };

        //////////
        // more cases to be considered...
        if ( key.find("*")!=std::string::npos) {
          subscriber.punsubscribe(key,got_error);
          is_ok = false;
        }
        else {
          subscriber.unsubscribe(key,got_error);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          // test not subscribed to topic anymore
          for( auto& s : subscriber.subscribedTopics())
            if( s == key )
              is_ok = false;
        }
        
        return is_ok;
      }
      
      bool Unsubscribe(const std::string& key,
                       std::function<void(const std::string&,const int&)> got_error
                       ) override {
        bool is_ok = true;
        std::size_t found = key.find("*");

        if ( found != std::string::npos) {
          std::string short_key(key);
          short_key.pop_back();
          auto list = subscriber.psubscribedTopics();
          if ( list.find(short_key) == list.end() )
            return false;
          subscriber.punsubscribe(short_key.substr(0,found-1),got_error);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          list = subscriber.psubscribedTopics();
          if ( list.find(short_key) == list.end() )
            return false;

        }
        else {
          subscriber.unsubscribe(key,got_error);
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          for( auto& s : subscriber.subscribedTopics())
            if( s == key )
              is_ok = false;
        }

        return is_ok;
      }

      std::set<std::string> ListTopics() {
        std::set<std::string> s = subscriber.subscribedTopics();
        s.insert(subscriber.psubscribedTopics().begin(),
                 subscriber.psubscribedTopics().end()
                 );
        return s;
      }

      int NumMessages() const { return updates.size(); }
      int NumRecvMessages() const { return total_recv_messages; }
      bool keep_counting = true;
    private:
      redox::Redox publisher;
      redox::Subscriber subscriber;
      
      std::ostream& log;
      unsigned long int total_num_messages = 0;
      unsigned long int total_recv_messages = 0;

      std::chrono::system_clock::time_point last_notify_time;
      std::thread t;
      
      void TimedNotification() {
        while(this->keep_counting) {
          std::this_thread::sleep_for(
                                      std::chrono::seconds(NotificationTimeout));
          // std::this_thread::sleep_until(last_notify_time+
          //                               std::chrono::seconds(NotificationTimeout));
          log << NotificationTimeout << "s elapsed, auto-notification will occur\n";
          //          log << "value of keep counting = " << keep_counting << "\n";
          Notify();
        }
        log << "TimedNotification terminated\n";
      }
      
    };
    
  }
}
  
