#include <redis_configuration.hpp>
#include <redis_utils.hpp>

int configuration::communicator::RedisCommunicator::NotificationTimeout = 2;

configuration::data::RedisDataManager::RedisDataManager(const std::string& redis_server,
                                                                                  const int& redis_port,
                                                                                  container::MultimapContainer& u,
                                                                                  std::ostream& logger=std::cerr) : address(redis_server), port(redis_port),
                                                                                                                    rdx(std::make_shared<redox::Redox>(std::cout,redox::log::Level::Fatal)),
                                                                                                                    log(logger), updates(u) {
  
  auto f = std::bind(utils::redis_connection_callback,
                     std::placeholders::_1,
                     std::ref(connection_status));
  rdx->connect(address, port, f );
  
  if( connection_status != redox::Redox::CONNECTED ) {
    log << "Can't connect to REDIS server\n";
    throw std::runtime_error("Can't connect to REDIS server: error"+std::to_string(connection_status));
  }
  
};

configuration::data::RedisDataManager::~RedisDataManager() {
  rdx->disconnect();
}

void configuration::data::RedisDataManager::Disconnect() {
  rdx->disconnect();
  if( connection_status != redox::Redox::DISCONNECTED ) {
    log << "Can't disconnect from REDIS server: error "+std::to_string(connection_status) << std::endl;
  }
}

void configuration::data::RedisDataManager::Dump(std::ostream& os) {
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


void configuration::data::RedisDataManager::Clear() {
  rdx->command<std::string>({"FLUSHALL"});
}

redox::Redox& configuration::data::RedisDataManager::redox() {
  return (*rdx);
} 

void configuration::data::RedisDataManager::Reconnect() {
  log << "Error: can't connect to redis database. Trying reconnect" << std::endl;
  auto f = std::bind(utils::redis_connection_callback,
                     std::placeholders::_1,
                     std::ref(connection_status));
  rdx = std::make_shared<redox::Redox>(std::cout,redox::log::Level::Fatal);
  rdx->connect(address, port, f);
}


bool configuration::data::RedisDataManager::KeyExists (const std::string& key) {
  std::string s="KEYS "+key;
  utils::typelist::KEYS_t result;
  utils::ExecRedisCmd<utils::typelist::KEYS_t>(*rdx,s,result);
  return result.size()>0;
}

bool configuration::data::RedisDataManager::HExists(const std::string& hash,const std::string& key) {
  std::string cmd = {"HEXISTS "+hash+" "+key};
  int value = 0;
  utils::ExecRedisCmd<int>(*rdx,cmd,value);
  return value;
}
bool configuration::data::RedisDataManager::SIsMember(const std::string& hash,
                                                                                 const std::string& key) {
  std::string cmd = {"SISMEMBER "+hash+" "+key};
  int value = 0;
  utils::ExecRedisCmd<int>(*rdx,cmd,value);
  return value;
}

bool configuration::data::RedisDataManager::IsHash(const std::string& key) { 
  std::string key_type;
  utils::ExecRedisCmd<std::string>(*rdx,std::string("TYPE ")+key,key_type);
  return key_type == "hash";
}

bool configuration::data::RedisDataManager::AddToHash(const std::string& key,
                                                                                 const std::vector<std::string>& value) {
  bool is_ok = true;
  bool ok;
  for(auto& v : value) {
    ok = AddToHash(key,v);
    if( ok )
      updates.insert(std::pair<std::string,std::string>(key,"a") );
    is_ok &= ok;
  }
  return is_ok;
}

bool configuration::data::RedisDataManager::AddToHash(const std::string& key,
                                                                                 const std::string& value) {
  std::size_t found = key.find_last_of(":");
  updates.insert(std::pair<std::string,std::string>(key,"a") );
  //        updates.Publish(key,"a");
  return utils::ExecRedisCmd<int>(*rdx,std::string("HSET ")+key.substr(0,found)+" "+key.substr(found+1)+" "+value);
}

bool configuration::data::RedisDataManager::RemoveFromParent(const std::string& parent,
                                                                                        const std::string& name) {
  bool is_ok = true;
  bool ok;
  ok = utils::ExecRedisCmd<utils::typelist::DEL_t>(*rdx,std::string("SREM ")+parent+" "+name);
  if( ok )
    updates.insert(std::pair<std::string,std::string>(parent,"u") );
  //          updates.Publish(parent,"u");
  is_ok &= ok;
  // if parents gets empty, delete
  int nelem;
  utils::ExecRedisCmd<int>(*rdx,std::string("SCARD ")+parent,&nelem);
  if (nelem == 0) 
    is_ok &=  (RemoveKey(parent) > 0) ;
  return is_ok;
}

int configuration::data::RedisDataManager::RemoveChildren(const std::string& key) {
  utils::typelist::KEYS_t children_list;
  std::string cmd="KEYS "+key+":*";
  utils::ExecRedisCmd<utils::typelist::KEYS_t>(*rdx,cmd,children_list);
  for( auto& c : children_list ) {
    if ( utils::ExecRedisCmd<utils::typelist::DEL_t>(*rdx,"DEL "+c) )
      updates.insert(std::pair<std::string,std::string>(c,"d") );
    //            updates.Publish(c,"d");
  }          
  return children_list.size();
}


bool configuration::data::RedisDataManager::RemoveKey(const std::string& key) {
  updates.insert(std::pair<std::string,std::string>(key,"d") );
  //        updates.Publish(key,"d");
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
  if(ok)  {
    updates.insert(std::pair<std::string,std::string>(key,"d") );
    //updates.Publish(key,"d");
  }
  return (is_ok && ok);
}

bool configuration::data::RedisDataManager::RemoveHash(const std::string& hash,const std::string& key) {
  if(utils::ExecRedisCmd<int>(*rdx,std::string("HDEL ")+hash+" "+key)) {
    updates.insert(std::pair<std::string,std::string>(key,"d") );
    //	  updates.Publish(key,"d");
    return true;
  }
  return false;
}

bool configuration::data::RedisDataManager::UpdateHashValue(const std::string& key,const std::string& value) {
  std::size_t found = key.find_last_of(":");
  if (found == std::string::npos ) 
    return false;
  std::string cmd = std::string("HSET ")+key.substr(0,found)+" "+key.substr(found+1)+" "+value;
  updates.insert(std::pair<std::string,std::string>(key,"u") );
  //updates.Publish(key,"u");
  return utils::ExecRedisCmd<int>(*rdx,cmd);
}


bool configuration::data::RedisDataManager::AddToParent(const std::string& key,const std::string& value) {
  std::string cmd=std::string("SADD ")+key+" "+value;
  updates.insert(std::pair<std::string,std::string>(key,"u") );
  //updates.Publish(key,"u");
  return utils::ExecRedisCmd<int>(*rdx,cmd);;
}

bool configuration::data::RedisDataManager::UpdateParent(const std::string& key) {
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


configuration::utils::typelist::LIST_t configuration::data::RedisDataManager::ReturnValue(const std::string& key) {
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
          log << "Type "+key_type+" not supported";
        }
      }
    }
  }
  return result;
}
    

bool configuration::data::RedisDataManager::redis_json_scan(GenericMemberIterator& member,
                                                                                       std::string prefix) {

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

