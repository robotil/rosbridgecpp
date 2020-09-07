#include "rosbridge_ws_client.hpp"

#include <cstring>
#include <fstream>
#include <future>
#include <string>
#include <iostream>

#include "rapidjson/document.h" 

using namespace std;
using namespace rapidjson;

//RosbridgeWsClient rbc("simapi.easyrms.app:9090");
//RosbridgeWsClient rbc("192.168.17.100:9091");
RosbridgeWsClient rbc("simapi.easyrms.app:9091");

void advertiseServiceCallback(std::shared_ptr<WsClient::Connection> /*connection*/, std::shared_ptr<WsClient::InMessage> in_message)
{
  // message->string() is destructive, so we have to buffer it first
  std::string messagebuf = in_message->string();
  std::cout << "advertiseServiceCallback(): Message Received: " << messagebuf << std::endl;

  rapidjson::Document document;
  if (document.Parse(messagebuf.c_str()).HasParseError())
  {
    std::cerr << "advertiseServiceCallback(): Error in parsing service request message: " << messagebuf << std::endl;
    return;
  }

  rapidjson::Document values(rapidjson::kObjectType);
  rapidjson::Document::AllocatorType& allocator = values.GetAllocator();
  values.AddMember("success", document["args"]["data"].GetBool(), allocator);
  values.AddMember("message", "from advertiseServiceCallback", allocator);

  rbc.serviceResponse(document["service"].GetString(), document["id"].GetString(), true, values);
}

void callServiceCallback(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::InMessage> in_message)
{
  std::cout << "serviceResponseCallback(): Message Received: " << in_message->string() << std::endl;
  connection->send_close(1000);
}

void publisherThread(RosbridgeWsClient& rbc, const std::future<void>& futureObj)
{
  rbc.addClient("topic_publisher");

  rapidjson::Document d;
  d.SetObject();
  d.AddMember("data", "Test message from /ztopic", d.GetAllocator());

  while (futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
  {
    rbc.publish("/ztopic", d);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  std::cout << "publisherThread stops()" << std::endl;
}

void subscriberCallback(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::InMessage> in_message)
{
  std::cout << "subscriberCallback(): Drone mode " << in_message->string() << std::endl;
}


int main() {

  rbc.addClient("sub");
  rbc.subscribe("sub", "/control_api/drone/status", subscriberCallback);

  rapidjson::Document document1(rapidjson::kObjectType);
  document1.AddMember("type", "open", document1.GetAllocator());
  document1.AddMember("param1", 0, document1.GetAllocator());
  document1.AddMember("param2", 0, document1.GetAllocator());
  rbc.callService("/control_api/eg/command", callServiceCallback, document1);
  usleep(10*1e6);
  
  rapidjson::Document document2(rapidjson::kObjectType);
  document2.AddMember("min_pitch", 0, document2.GetAllocator());
  document2.AddMember("yaw", 0, document2.GetAllocator());
  document2.AddMember("altitude", 20, document2.GetAllocator());
  rbc.callService("/control_api/cmd/takeoff", callServiceCallback, document2);
  usleep(20*1e6);
  
  rapidjson::Document document3(rapidjson::kObjectType);
  document3.AddMember("min_pitch", 0, document3.GetAllocator());
  document3.AddMember("yaw", 0, document3.GetAllocator());
  document3.AddMember("altitude", 20, document3.GetAllocator());
  rbc.callService("/control_api/cmd/land", callServiceCallback, document3);
  usleep(30*1e6);

  rbc.removeClient("sub");
  std::cout << "Program terminated" << std::endl;
	

  
  //rbc.addClient("service_advertiser");
  //rbc.advertiseService("service_advertiser", "/zservice", "std_srvs/SetBool", advertiseServiceCallback);

  //rbc.addClient("topic_advertiser");
  //rbc.advertise("topic_advertiser", "/ztopic", "std_msgs/String");
  // Test calling a service
/*
  // Test creating and stopping a publisher
  {
    // Create a std::promise object
    std::promise<void> exitSignal;

    // Fetch std::future object associated with promise
    std::future<void> futureObj = exitSignal.get_future();

    // Starting Thread & move the future object in lambda function by reference
    std::thread th(&publisherThread, std::ref(rbc), std::cref(futureObj));

    // Wait for 10 sec
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "Asking publisherThread to Stop" << std::endl;

    // Set the value in promise
    exitSignal.set_value();

    // Wait for thread to join
    th.join();
  }

  // Test removing clients
  rbc.removeClient("service_advertiser");
  rbc.removeClient("topic_advertiser");
  rbc.removeClient("topic_subscriber");

  std::cout << "Program terminated" << std::endl;*/
}



