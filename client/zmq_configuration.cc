#include <zmq.hpp>

#include <zmq_configuration.hpp>

configuration::communicator::ZmqCommunicator::ZmqCommunicator(const std::string& server,
                                                              const int& publisher_port,
                                                              const int& subscriber_port,                      
                                                              std::ostream& logger) 
  : keep_listening(true),
    context(std::make_shared<zmq::context_t>(1)),
    publisher(std::make_shared<zmq::socket_t>(*context, ZMQ_PUB)),
    subscriber(std::make_shared<zmq::socket_t>(*context, ZMQ_SUB)),
    log(logger),
    last_notify_time(std::chrono::system_clock::now()),
    device_server(server),
    zmq_publisher_port(publisher_port),
    zmq_subscriber_port(subscriber_port) {
  
  std::string connect_to=(std::string("tcp://")+device_server+":"+std::to_string(zmq_publisher_port) );
  try{
    publisher->connect(connect_to);
  }
  catch (std::exception& e) {
    std::cout << e.what() << std::endl;
  }
  
  connect_to=(std::string("tcp://")+device_server+":"+std::to_string(zmq_subscriber_port) );
  std::cout << connect_to << std::endl;
  try{
    subscriber->connect(connect_to);
  }
  catch (std::exception& e) {
    std::cout << e.what() << std::endl;
  }
  result_subscriber = std::async(std::launch::async,
                                 &ZmqCommunicator::Listener,this,
                                 default_got_message,
                                 default_got_error,
                                 default_unsubscribed 
                                 );
}



void configuration::communicator::ZmqCommunicator::Listener( std::function<void(const std::string&,
                                                                                const std::string&)>&& f=default_got_message,
                                                             std::function<void(const std::string&,
                                                                                const int&)> g = default_got_error,
                                                             std::function<void(const std::string&)> h = default_unsubscribed
                                                             ) {
  zmq::message_t srequest;
  while(keep_listening) {
    try {
      subscriber->recv(&srequest);
      f("Received",std::string((char*)srequest.data()).substr(0,srequest.size()));
    }
    catch (std::exception& e){
      g(e.what(),-1);
    }
  }
};



bool configuration::communicator::ZmqCommunicator::Notify() {
  zmq::message_t prequest (1024);
  bool is_ok = true;
  for(auto& msg : updates) {
    try {
      std::string s=msg.first;
      s+=" : "+msg.second;
      memcpy ((void *) prequest.data (), s.c_str(), s.size());
      publisher->send(prequest);
    }
    catch (std::exception& e){
      std::cout << "publisher failure: " << e.what() << std::endl;
      is_ok = false;
    }
  }
  
  last_notify_time = std::chrono::system_clock::now();
  updates.clear();
  return is_ok;
};



bool configuration::communicator::ZmqCommunicator::Subscribe(const std::string& filter) {
  if( subscriptions.find(filter) == subscriptions.end())
    subscriber->setsockopt(ZMQ_SUBSCRIBE,filter.c_str(), filter.length());
  else {
    log << "Already listening for " << filter << ": command ignored" << std::endl;
    return false;
  }
  return true;
};

bool configuration::communicator::ZmqCommunicator::Subscribe(const std::string& filter,
               std::function<void(const std::string&,const std::string&)>, // got message
               std::function<void(const std::string&,const int&)>,
               std::function<void(const std::string&)>
               ) {
  if( subscriptions.find(filter) == subscriptions.end())
    subscriber->setsockopt(ZMQ_SUBSCRIBE,filter.c_str(), filter.length());
  else {
    log << "Already listening for " << filter << ": command ignored" << std::endl;
    return false;
  }
  return true;
};


bool configuration::communicator::ZmqCommunicator::Unsubscribe(const std::string& filter,
                 std::function<void(const std::string&,const int&)> got_error
                 ) {
  if( subscriptions.find(filter) != subscriptions.end())
    subscriber->setsockopt(ZMQ_UNSUBSCRIBE,filter.c_str(), filter.length());
  else {
    log << "Not subscribed to " << filter << ": command ignored" << std::endl;
    return false;
  }
  return true;
};
