/****************************************************************************
 Module
   CloudService.c

 Description
   Handels communication with cloud influxdb 

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/

#include "CloudService.h"
#include "ES_framework.h"
#include "ES_Timers.h"
#include "IAQ_util.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <time.h>

/*----------------------------- Module Defines ----------------------------*/
#define DEVICE "ESP32"
#define MAX_WIFI_RETRIES 2
#define WIFI_POLLING_PERIOD 200  // ms
#define WIFI_RETRY_TMOUT 5000u  // ms

#define WIFI_SSID "PS Home"
#define WIFI_PASSWORD "13Waheguru"
#define INFLUXDB_URL "https://us-central1-1.gcp.cloud2.influxdata.com"
#define INFLUXDB_TOKEN "y8PN5i8FgMUx7ZeH9Ws6_edHdj7b91pfZzNuKk5OHGzFNR5TLYXehuAF6hYO9fCuh3kZAHGaR5rvJ0cBAaWTGQ=="
#define INFLUXDB_ORG "nav.singh1995@gmail.com"
#define INFLUXDB_BUCKET "IAQ"
#define TZ_INFO "PST8PDT"
#define WRITE_PRECISION WritePrecision::S
#define MAX_BATCH_SIZE 1
#define WRITE_BUFFER_SIZE 50
#define FLUSH_INTERVAL (60*10)  // max num seconds to retain data for cloud  

typedef enum
{
  START_CONNECTION,
  CONNECTING_STATE,
  SENDING_DATA_STATE
}statusState_t; 

/*---------------------------- Module Functions ---------------------------*/
void setupDataPt(); 
void publishDataPt();
void printLocalTime();

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
Point sensor("IAQ_Readings");  // Data point

// InfluxDB client instance with preconfigured InfluxCloud certificate
InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN, InfluxDbCloud2CACert);
statusState_t currSMState = START_CONNECTION;  

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitCloudService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes
****************************************************************************/
bool InitCloudService(uint8_t Priority)
{
  MyPriority = Priority;
  
  return true; 
}

/****************************************************************************
 Function
     PostCloudService

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine
 Notes
****************************************************************************/
bool PostCloudService(ES_Event_t ThisEvent)
{
  ThisEvent.ServiceNum = MyPriority; 
  return ES_PostToService(ThisEvent);
}

/****************************************************************************
 Function
    RunCloudService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
****************************************************************************/
ES_Event_t RunCloudService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  static uint8_t wifiRetries = 0; 

  switch (currSMState)
  {
    case START_CONNECTION: 
    {
      if(ThisEvent.EventType == ES_INIT)
      {
        if(wifiRetries <= MAX_WIFI_RETRIES)
        {
          // Setup wifi
          WiFi.setAutoConnect(false);
          WiFi.mode(WIFI_STA);
          // WiFi.setSleep(true); 
          WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
          esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
          printf("Connecting to wifi\n");
          ES_Timer_InitTimer(WIFI_TIMER_NUM, WIFI_POLLING_PERIOD); 
          currSMState = CONNECTING_STATE; 
        }
        else
        {
          wifiRetries = 0; 
          printf("Could not connect with Wifi\n");
        }
      }
      break;
    }

    case CONNECTING_STATE:
    {
      if (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == WIFI_TIMER_NUM)
      {
        static uint8_t tmoutCounts = 0; 
        if(WiFi.status() == WL_CONNECTED)
        {
          printf("Connected\n");
          tmoutCounts = 0; 
          wifiRetries = 0; 
          currSMState = START_CONNECTION; 

          client.setWriteOptions(WRITE_PRECISION, MAX_BATCH_SIZE, WRITE_BUFFER_SIZE, FLUSH_INTERVAL, false); 
          timeSync(TZ_INFO, "pool.ntp.org"); // TODO: make unblocking and don't need to do this every time. Maybe once every hour
          delay(500); 
          setupDataPt(); 
          publishDataPt(); // TODO: make into state machine so retry if failed once
          ES_Event_t NewEvent = {.EventType=CLOUD_UPDATED_EVENT};
          PostMainService(NewEvent); 
          WiFi.disconnect(true);
          WiFi.mode(WIFI_OFF);  

        }else
        { 
          tmoutCounts++; 
          if(tmoutCounts >= (WIFI_RETRY_TMOUT / WIFI_POLLING_PERIOD))
          {
            tmoutCounts = 0; 
            wifiRetries++; 
            currSMState = START_CONNECTION; 
            ES_Event_t newEvent = {.EventType=ES_INIT}; 
            PostCloudService(newEvent); 
            Serial.println("Retrying");
          }
          else
          {
            Serial.print(".");
            ES_Timer_InitTimer(WIFI_TIMER_NUM, WIFI_POLLING_PERIOD); 
          }
        }
      }
      
      break;
    }
    
    case SENDING_DATA_STATE:
    {
      if(ThisEvent.EventType == CLOUD_PUBLISH)
      {
        return ReturnEvent; 
      }

      break;
    }
  }
  return ReturnEvent;
}


void updateCloudSensorVals(IAQsensorVals_t *sensorReads)
{
  sensor.clearFields(); // Clear fields for reusing the point. Tags will remain untouched
  sensor.addField("eCO2", sensorReads->eCO2); 
  sensor.addField("CO2", sensorReads->CO2); 
  sensor.addField("PM2.5", sensorReads->PM25); 
  sensor.addField("PM10", sensorReads->PM10); 
  sensor.addField("tVOC", sensorReads->tVOC); 
  sensor.addField("tm", sensorReads->temp); 
  sensor.addField("rh", sensorReads->rh); 
  sensor.addField("bat", getBatVolt()); 
}

/***************************************************************************
 private functions
 ***************************************************************************/
void setupDataPt()
{
  // Add tags
  sensor.clearTags(); 
  char buff[25];
  sprintf(buff, "%llu",ESP.getEfuseMac()); 
  sensor.addTag("chipID", buff);

  // Check server connection
  if (client.validateConnection()) {
    Serial.print("Connected to InfluxDB: ");
    Serial.println(client.getServerUrl());
  } else {
    Serial.print("InfluxDB connection failed: ");
    Serial.println(client.getLastErrorMessage());
  }
}

void publishDataPt()
{
  // Print what are we exactly writing
  Serial.print("Writing: ");
  Serial.println(sensor.toLineProtocol());

  if ((WiFi.RSSI() == 0) && (WiFi.status() != WL_CONNECTED)) {
    Serial.println("Wifi connection lost");
  }

  // Write point
  if (!client.writePoint(sensor)) {
    Serial.print("InfluxDB write failed: ");
    Serial.println(client.getLastErrorMessage());
    delay(2000); 
    Serial.println("Retrying");
    if(!client.writePoint(sensor))
    {
      Serial.print("InfluxDB write failed: ");
      Serial.println(client.getLastErrorMessage());
    } 
    else
    {
      Serial.println("It worked 2nd time"); 
    }
  }

}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

