#include "rosbridge_ws_client.hpp"

#include <cstring>
#include <fstream>
#include <future>
#include <string>
#include <iostream>
#include <time.h>

#include "rapidjson/document.h" 

using namespace std;
using namespace rapidjson;

//RosbridgeWsClient rbc("simapi.easyrms.app:9090");
//RosbridgeWsClient rbc("192.168.17.100:9091");
RosbridgeWsClient rbc("simapi.easyrms.app:9091");


FILE *Telemetry;


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

int  GetTimeFromMidnight()
{
  // double Total = 0.0;
  // time_t rawtime;
  // struct tm * timeinfo;

        // struct timeval tv;
        // struct timezone tz;
        // struct tm *tm;
        // gettimeofday(&tv,&tz);
        // tm=localtime(&tv.tv_sec);
        // printf("StartTime: %d:%02d:%02d %d \n", tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec);



  // time ( &rawtime );
  // timeinfo = localtime ( &rawtime );
  // Total = timeinfo->tm_sec + (timeinfo->tm_min * 60) + (timeinfo->tm_hour * 3600);
  // printf("\nCurrent local time and date: %f",Total);// asctime (timeinfo) );

//  auto start = std::chrono::system_clock::now();
//  unsigned int Time = 0;
//  time_t st;
//  ctime(&st);
//
//  Time = st->

    timespec time1;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);

    return ((int)time1.tv_nsec);


}
void telemetryCallback(std::shared_ptr<WsClient::Connection> connection, std::shared_ptr<WsClient::InMessage> in_message)
{
  int TimeMsec = 0;
  bool StartBrakeFlag = false;
  bool EndBrakeFlag = false;
  bool Go = true;
  int i = 0;
  int ComaCount = 0;
  int Index = 0;
  int DataIndex = 0;
  char MsgString[500] = {0};
  char MsgParam[50] = {0};
  char MsgData[50] = {0};
  float MsgDataFloat = 0.0;
  double vertical_speed = 0.0;
  double yaw = 0.0;
  double pitch = 0.0;
  double roll = 0.0;
  double speed = 0.0;
  double altitude = 0.0;
  double longitude = 0.0;
  double latitude = 0.0;
  double current = 0.0;
  double voltage = 0.0;

//  std::cout << "ZZZ telemetryCallback():  " << in_message->string() << std::endl;

//  strcpy(MsgString,(char *)in_message->string() );
  strcpy(MsgString,in_message->string().c_str());
//  printf("\n%s\n",MsgString);

  i=0;

  do
  {
    if(MsgString[i] == 125)
      Go = false;
    else
    {
      if(MsgString[i] == '{')
      {
        ComaCount++;
      }
      if(ComaCount == 2)
      {
        if(MsgString[i] == '"')
        {
          if(!StartBrakeFlag)
          {
            StartBrakeFlag = true;
            EndBrakeFlag = false;
            memset(MsgParam,0,50);
            memset(MsgData,0,50);
            Index = 0;
          }
          else
          {
            if(MsgString[i+1] == ':')
            {
              StartBrakeFlag = false;
              EndBrakeFlag = true;
              i++; // read ":"
            }
            else
            {
              StartBrakeFlag = true;
              EndBrakeFlag = false;
              memset(MsgParam,0,50);
              memset(MsgData,0,50);
              Index = 0;
            }
            
          }
            
        }
        else
        {
          // get the data
          if(StartBrakeFlag)
          {
            MsgParam[Index] = MsgString[i];
            Index++;
          }
          if(EndBrakeFlag)
          {
            EndBrakeFlag = false;
            MsgParam[Index] = 0;
            Index = 0;
            i++;
            DataIndex = 0;
            while((MsgString[i] != ',') && (MsgString[i] != 125)) // '}'
            {
              MsgData[DataIndex] = MsgString[i];
              i++;
              DataIndex++;
            }
    //        if(MsgString[i] == 125)
    //          Go = false;
    //        else
//            printf("\n%s",MsgData);
            MsgData[DataIndex] = 0;
            MsgDataFloat = atof(MsgData);
  //          std::cout << ">>  " << MsgData << std::endl;
            if(!strcmp(MsgParam,"vertical_speed"))
            {
              vertical_speed = MsgDataFloat;
              printf("\n\n vertical_speed %f",vertical_speed);
            }
            if(!strcmp(MsgParam,"yaw"))
            {
              yaw = MsgDataFloat;
              printf("\n yaw %f",yaw);
            }
            if(!strcmp(MsgParam,"pitch"))
            {
              pitch = MsgDataFloat;
              printf("\n pitch %f",pitch);
            }
            if(!strcmp(MsgParam,"roll"))
            {
              roll = MsgDataFloat;
              printf("\n roll %f",roll);
            }
            if(!strcmp(MsgParam,"speed"))
            {
              speed = MsgDataFloat;
              altitude = MsgDataFloat;
              printf("\n altitude %f",altitude);
            }
            if(!strcmp(MsgParam,"longitude"))
            {
              current = MsgDataFloat;
              printf("\n current %f",current);
            }
            if(!strcmp(MsgParam,"voltage"))
            {
              voltage = MsgDataFloat;
              printf("\n voltage %f",voltage);
            }
            if(!strcmp(MsgParam,"latitude"))
            {
              latitude = MsgDataFloat;
              printf("\n latitude %f",latitude);
            }
          }
        }
      }
    }
    i++;
  } while ((MsgString[i] != 0) && Go);
  if(Telemetry != nullptr)
  {
    TimeMsec = GetTimeFromMidnight();
    fprintf(Telemetry,"%d,%2.5f,%3.5f,%3.5f,%3.8f,%3.8f,%5.2f,%5.2f,%5.2f,%3.2f,%3.2f\n",
            TimeMsec,
            pitch,
            roll,
            yaw,
            latitude,
            longitude,
            altitude,
            vertical_speed,
            speed,
            current,
            voltage);
  }
//  std::cout << ">>>>>>>>  " << "voltage " << voltage << std::endl;
//  printf("\nlongitude = %f\n",longitude);

}

int main() {

  rbc.addClient("sub");
  rbc.subscribe("sub", "/control_api/drone/status", subscriberCallback);
  rbc.addClient("sub2");
  rbc.subscribe("sub2", "/control_api/drone/telemetry", telemetryCallback);
  
  Telemetry = fopen("Telemetry.txt", "wt");
  if(Telemetry != nullptr)
  {
    fprintf(Telemetry,"TimeMsec,pitch,roll,yaw,latitude,longitude,altitude,vertical_speed,speed,current,voltage\n");
  }

  
  rapidjson::Document document1(rapidjson::kObjectType);
  document1.AddMember("type", "open", document1.GetAllocator());
  document1.AddMember("param1", 0, document1.GetAllocator());
  document1.AddMember("param2", 0, document1.GetAllocator());
  rbc.callService("/control_api/eg/command", callServiceCallback, document1);
  usleep(10*1e6);
  
/*
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
*/
  rbc.removeClient("sub");
  std::cout << "Program terminated" << std::endl;
  if(Telemetry != nullptr)
    fclose(Telemetry);

  
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



