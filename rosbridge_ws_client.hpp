/*
 *  Created on: Apr 16, 2018
 *      Author: Poom Pianpak
 */

#ifndef ROSBRIDGECPP_ROSBRIDGE_WS_CLIENT_HPP_
#define ROSBRIDGECPP_ROSBRIDGE_WS_CLIENT_HPP_

#include "Simple-WebSocket-Server/client_ws.hpp"
//#include "Simple-WebSocket-Server/client_wss.hpp"

#include "rapidjson/include/rapidjson/document.h"
#include "rapidjson/include/rapidjson/writer.h"
#include "rapidjson/include/rapidjson/stringbuffer.h"

#include <chrono>
#include <functional>
#include <thread>

#include <bits/stdc++.h>
#include "ros/ros.h"
#include "rosauth/Authentication.h"
#include <openssl/sha.h>
#include <fstream>
#include <stdio.h>
#include <curl/curl.h>



using namespace std;

using WsClient = SimpleWeb::SocketClient<SimpleWeb::WS>;
//using WsClient = SimpleWeb::SocketClient<SimpleWeb::WSS>;
using InMessage =  std::function<void(std::shared_ptr<WsClient::Connection>, std::shared_ptr<WsClient::InMessage>)>;

string glob_mac_secret;
#define DEBUG
size_t WriteCallback(char *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string*)userp)->append((char*)contents, size * nmemb);
	string str = contents;
	if (str.find("true")==string::npos)
		printf("ERROR: Secret code not read succesfully.");
	else
		glob_mac_secret = str.substr(26, 16);
	printf("\nsecret: %s\n",glob_mac_secret.c_str());
    return size * nmemb;
}

string get_auth(string client_name){	
		
	string mac_secret = glob_mac_secret;
	
	string mac_rand = ""; 
	const int MAX_SIZE = 62;
	char letters[MAX_SIZE] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9'};
	for (int i=0;i<10;i++) mac_rand += letters[rand() % MAX_SIZE];
	  
	struct timespec mac_time;
	timespec_get(&mac_time, TIME_UTC);	
	
	string mac_level 	= "admin";
	//string mac_dest 	= "wss://simapi.easyrms.app:9090";
	string mac_dest 	= "ws://simapi.easyrms.app:9091";
	//string mac_dest 	= "ws://192.168.17.100:9091";
	string mac_client 	= "192.168.18.1";
	
	stringstream ss;
	ss << mac_secret << mac_client << mac_dest << mac_rand << mac_time.tv_sec << mac_level << mac_time.tv_sec+10000;
	int len = ss.str().length();

		unsigned char sha512_hash[SHA512_DIGEST_LENGTH];
		SHA512((unsigned char *)ss.str().c_str(), ss.str().length(), sha512_hash); 
		char hex[SHA512_DIGEST_LENGTH * 2];
		for (int i = 0; i < SHA512_DIGEST_LENGTH; i++)
			sprintf(&hex[i * 2], "%02x", sha512_hash[i]);

	stringstream ss1;           
	ss1 << "\"op\":\"auth\", \"mac\":\"" << hex << "\", \"client\":\"" << mac_client;
	ss1 << "\", \"dest\":\"" << mac_dest << "\", \"rand\":\"" << mac_rand << "\", \"t\":" << mac_time.tv_sec;
	ss1 << ", \"level\":\"" << mac_level << "\", \"end\":" << (mac_time.tv_sec+10000);                      
	std::string msg = ss1.str(); 
	msg = "{" + msg + "}";
	return msg;
}

class RosbridgeWsClient
{
  std::string server_port_path;
  std::unordered_map<std::string, std::shared_ptr<WsClient>> client_map;

  void start(const std::string& client_name, std::shared_ptr<WsClient> client, /*const*/ std::string& message)
  {
	  
    if (!client->on_open)
    {
		
#ifdef DEBUG
      client->on_open = [client_name, message](std::shared_ptr<WsClient::Connection> connection) {
#else
      client->on_open = [message](std::shared_ptr<WsClient::Connection> connection) {
#endif

#ifdef DEBUG

        std::cout << client_name << ": Opened connection" << std::endl;
        std::cout << client_name << ": Sending message: " << message << std::endl;
#endif

		string msg = get_auth(client_name);
		connection->send(msg);
        connection->send(message);
      };
    }

#ifdef DEBUG
    if (!client->on_message)
    {
      client->on_message = [client_name](std::shared_ptr<WsClient::Connection> /*connection*/, std::shared_ptr<WsClient::InMessage> in_message) {
        std::cout << client_name << ": Message received: " << in_message->string() << std::endl;
      };
    }
	
	if (!client->on_close)
    {
      client->on_close = [client_name](std::shared_ptr<WsClient::Connection> /*connection*/, int status, const std::string & /*reason*/) {
        std::cout << client_name << ": Closed connection with status code " << status << std::endl;
      };
    }

    if (!client->on_error)
    {
		// See http://www.boost.org/doc/libs/1_55_0/doc/html/boost_asio/reference.html, Error Codes for error code meanings
      client->on_error = [client_name](std::shared_ptr<WsClient::Connection> /*connection*/, const SimpleWeb::error_code &ec) {
        std::cout << client_name << ": Error: " << ec << ", error message: " << ec.message() << std::endl;
      };
    }
#endif

#ifdef DEBUG
    std::thread client_thread([client_name, client]() {
#else
    std::thread client_thread([client]() {
#endif
      client->start();

#ifdef DEBUG
      std::cout << client_name << ": Terminated" << std::endl;
#endif
      client->on_open = NULL;
      client->on_message = NULL;
      client->on_close = NULL;
      client->on_error = NULL;
    });

    client_thread.detach();

    // This is to make sure that the thread got fully launched before we do anything to it (e.g. remove)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }


public:
  RosbridgeWsClient(const std::string& server_port_path)
  {
    this->server_port_path = server_port_path;
	
	CURL *curl;
	CURLcode res;
	curl_global_init(CURL_GLOBAL_ALL);
	curl = curl_easy_init();
	std::string readBuffer;
	if(curl) 
	{
		printf("Authenticating...\n");
		//curl_easy_setopt(curl, CURLOPT_URL, "https://simapi.easyrms.app/control_api/token");
		//curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "username=test&password=test&port=9090");
		//curl_easy_setopt(curl, CURLOPT_URL, "http://192.168.17.100:34001/control_api/token");
		curl_easy_setopt(curl, CURLOPT_URL, "https://simapi.easyrms.app/control_api/token");
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "username=test&password=test&port=9091");
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
		
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();	
  }

  ~RosbridgeWsClient()
  {
    for (auto& client : client_map)
    {
      client.second->stop();
      client.second.reset();
    }
  }

  void addClient(const std::string& client_name)
  {
    std::unordered_map<std::string, std::shared_ptr<WsClient>>::iterator it = client_map.find(client_name);
    if (it == client_map.end())
    {
      client_map[client_name] = std::make_shared<WsClient>(server_port_path);
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << " has already been created" << std::endl;
    }
#endif
  }

  std::shared_ptr<WsClient> getClient(const std::string& client_name)
  {
    std::unordered_map<std::string, std::shared_ptr<WsClient>>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      return it->second;
    }
    return NULL;
  }

  void stopClient(const std::string& client_name)
  {
    std::unordered_map<std::string, std::shared_ptr<WsClient>>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      it->second->stop();
      it->second->on_open = NULL;
      it->second->on_message = NULL;
      it->second->on_close = NULL;
      it->second->on_error = NULL;
#ifdef DEBUG
      std::cout << client_name << " has been stopped" << std::endl;
#endif
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << " has not been created" << std::endl;
    }
#endif
  }

  void removeClient(const std::string& client_name)
  {
    std::unordered_map<std::string, std::shared_ptr<WsClient>>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      it->second->stop();
      it->second.reset();
      client_map.erase(it);
#ifdef DEBUG
      std::cout << client_name << " has been removed" << std::endl;
#endif
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << " has not been created" << std::endl;
    }
#endif
  }
/*
  void auth(const std::string& client_name)
  { 
  
	// parameters
	string mac_secret;
	std::ifstream file ("/home/rms/easyaerial_ws/user_data/websocket_secret/secret9091.txt");
	if(file)
		if(file.is_open())
			getline(file,mac_secret);
	else
		std::cerr << client_name << " secret file not found" << std::endl;
	 
	string mac_rand = ""; 
	const int MAX_SIZE = 62;
	char letters[MAX_SIZE] = {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9'};
	for (int i=0;i<10;i++) mac_rand += letters[rand() % MAX_SIZE];
	  
	struct timespec mac_time;
	timespec_get(&mac_time, TIME_UTC);	
	
	string mac_level 	= "admin";
	string mac_dest 	= "wss://simapi.easyrms.app:9091";
	string mac_client 	= "192.168.18.1";
	
	// sha512
	stringstream ss;
	ss << mac_secret << mac_client << mac_dest << mac_rand << mac_time.tv_sec << mac_level << mac_time.tv_sec+10000;
	
	//const string mac = sha512(ss.str());
	unsigned char sha512_hash[SHA512_DIGEST_LENGTH];
    SHA512((unsigned char *)ss.str().c_str(), ss.str().length(), sha512_hash); 
    // convert to a hex string to compare
    char hex[SHA512_DIGEST_LENGTH * 2];
    for (int i = 0; i < SHA512_DIGEST_LENGTH; i++)
		sprintf(&hex[i * 2], "%02x", sha512_hash[i]);

	std::unordered_map<std::string, std::shared_ptr<WsClient>>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
	  stringstream ss1;           
	  ss1 << "\"op\":\"auth\", \"mac\":\"" << hex << "\", \"client\":\"" << mac_client;
	  ss1 << "\", \"dest\":\"" << mac_dest << "\", \"rand\":\"" << mac_rand << "\", \"t\":" << mac_time.tv_sec;
      ss1 << ", \"level\":\"" << mac_level << "\", \"end\":" << (mac_time.tv_sec+10000);                      
      std::string msg = ss1.str(); 
      msg = "{" + msg + "}";
      start(client_name, it->second, msg);
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }
*/
  void advertise(const std::string& client_name, const std::string& topic, const std::string& type, const std::string& id = "")
  {
    std::unordered_map<std::string, std::shared_ptr<WsClient>>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      std::string message = "\"op\":\"advertise\", \"topic\":\"" + topic + "\", \"type\":\"" + type + "\"";

      if (id.compare("") != 0)
      {
        message += ", \"id\":\"" + id + "\"";
      }
      message = "{" + message + "}";

      start(client_name, it->second, message);
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }

  void publish(const std::string& topic, const rapidjson::Document& msg, const std::string& id = "")
  {
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    msg.Accept(writer);

    std::string message = "\"op\":\"publish\", \"topic\":\"" + topic + "\", \"msg\":" + strbuf.GetString();

    if (id.compare("") != 0)
    {
      message += ", \"id\":\"" + id + "\"";
    }
    message = "{" + message + "}";

    std::shared_ptr<WsClient> publish_client = std::make_shared<WsClient>(server_port_path);

    publish_client->on_open = [message](std::shared_ptr<WsClient::Connection> connection) {
#ifdef DEBUG
      std::cout << "publish_client: Opened connection" << std::endl;
      std::cout << "publish_client: Sending message: " << message << std::endl;
#endif
      connection->send(message);

      // TODO: This could be improved by creating a thread to keep publishing the message instead of closing it right away
    };

    start("publish_client", publish_client, message);
  }

  void subscribe(const std::string& client_name, const std::string& topic, const InMessage& callback, const std::string& id = "", const std::string& type = "", int throttle_rate = -1, int queue_length = -1, int fragment_size = -1, const std::string& compression = "")
  {  
    std::unordered_map<std::string, std::shared_ptr<WsClient>>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      std::string message = "\"op\":\"subscribe\", \"topic\":\"" + topic + "\"";

      if (id.compare("") != 0)
      {
        message += ", \"id\":\"" + id + "\"";
      }
      if (type.compare("") != 0)
      {
        message += ", \"type\":\"" + type + "\"";
      }
      if (throttle_rate > -1)
      {
        message += ", \"throttle_rate\":" + std::to_string(throttle_rate);
      }
      if (queue_length > -1)
      {
        message += ", \"queue_length\":" + std::to_string(queue_length);
      }
      if (fragment_size > -1)
      {
        message += ", \"fragment_size\":" + std::to_string(fragment_size);
      }
      if (compression.compare("none") == 0 || compression.compare("png") == 0)
      {
        message += ", \"compression\":\"" + compression + "\"";
      }
      message = "{" + message + "}";

      it->second->on_message = callback;
      start(client_name, it->second, message);
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }

  void advertiseService(const std::string& client_name, const std::string& service, const std::string& type, const InMessage& callback)
  {
    std::unordered_map<std::string, std::shared_ptr<WsClient>>::iterator it = client_map.find(client_name);
    if (it != client_map.end())
    {
      std::string message = "{\"op\":\"advertise_service\", \"service\":\"" + service + "\", \"type\":\"" + type + "\"}";

      it->second->on_message = callback;
      start(client_name, it->second, message);
    }
#ifdef DEBUG
    else
    {
      std::cerr << client_name << "has not been created" << std::endl;
    }
#endif
  }

  void serviceResponse(const std::string& service, const std::string& id, bool result, const rapidjson::Document& values = {})
  {
    std::string message = "\"op\":\"service_response\", \"service\":\"" + service + "\", \"result\":" + ((result)? "true" : "false");

    // Rosbridge somehow does not allow service_response to be sent without id and values
    // , so we cannot omit them even though the documentation says they are optional.
    message += ", \"id\":\"" + id + "\"";

    // Convert JSON document to string
    rapidjson::StringBuffer strbuf;
    rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
    values.Accept(writer);

    message += ", \"values\":" + std::string(strbuf.GetString());
    message = "{" + message + "}";

    std::shared_ptr<WsClient> service_response_client = std::make_shared<WsClient>(server_port_path);

    service_response_client->on_open = [message](std::shared_ptr<WsClient::Connection> connection) {
#ifdef DEBUG
      std::cout << "service_response_client: Opened connection" << std::endl;
      std::cout << "service_response_client: Sending message: " << message << std::endl;
#endif
      connection->send(message);

      connection->send_close(1000);
    };

    start("service_response_client", service_response_client, message);
  }

  void callService(const std::string& service, const InMessage& callback, const rapidjson::Document& args = {}, const std::string& id = "", int fragment_size = -1, const std::string& compression = "")
  {
    std::string message = "\"op\":\"call_service\", \"service\":\"" + service + "\"";

    if (!args.IsNull())
    {
      rapidjson::StringBuffer strbuf;
      rapidjson::Writer<rapidjson::StringBuffer> writer(strbuf);
      args.Accept(writer);

      message += ", \"args\":" + std::string(strbuf.GetString());
    }
    if (id.compare("") != 0)
    {
      message += ", \"id\":\"" + id + "\"";
    }
    if (fragment_size > -1)
    {
      message += ", \"fragment_size\":" + std::to_string(fragment_size);
    }
    if (compression.compare("none") == 0 || compression.compare("png") == 0)
    {
      message += ", \"compression\":\"" + compression + "\"";
    }
    message = "{" + message + "}";

    std::shared_ptr<WsClient> call_service_client = std::make_shared<WsClient>(server_port_path);

    if (callback)
    {
      call_service_client->on_message = callback;
    }
    else
    {
      call_service_client->on_message = [](std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::InMessage> in_message) {
#ifdef DEBUG
        std::cout << "call_service_client: Message received: " << in_message->string() << std::endl;
        std::cout << "call_service_client: Sending close connection" << std::endl;
#endif
        connection->send_close(1000);
      };
    }

    start("call_service_client", call_service_client, message);
  }
};

#endif
