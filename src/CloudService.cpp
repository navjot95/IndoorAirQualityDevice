/****************************************************************************
 Module
   CloudService.c

 Description
   Handels wifi connection and communication with cloud influxdb. If main SM is in stream mode, 
   then values are uploaded in batches of 3. In auto mode, values are uploaded
   as they come in.  

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/

#include "CloudService.h"
#include "ES_framework.h"
#include "ES_Timers.h"
#include "IAQ_util.h"
#include "credentials.h"  // Add your login credentials here or below 
#include <WiFi.h>
#include <esp_wifi.h>
#include <InfluxDbClient.h>
#include <InfluxDbCloud.h>
#include <time.h>

/*----------------------------- Module Defines ----------------------------*/
#define DEVICE "ESP32"
#define IAQ_SENSOR_LOCATION "Bedroom"
#define MAX_WIFI_RETRIES 2
#define WIFI_POLLING_PERIOD 200  // ms
#define WIFI_RETRY_TMOUT 5000U  // ms
#define TIME_SYNC_POLLING_PERIOD 200  // ms
#define TIME_SYNC_TMOUT 15000U  // ms
#define PUB_DELAY_PERIOD 500  // ms
#define TM_RESYNC_PERIOD 3600L  // sec

#define WRITE_PRECISION WritePrecision::S
#define WRITE_BUFFER_SIZE 15
#define STREAM_BATCH_SIZE 3
#define FLUSH_INTERVAL (60*35)  // max num seconds to retain data for cloud  

/***********FILL IN (or place in credentials.h)**************
#define WIFI_SSID 
#define WIFI_PASSWORD 
#define INFLUXDB_URL 
#define INFLUXDB_TOKEN 
#define INFLUXDB_ORG 
#define INFLUXDB_BUCKET 
#define TZ_INFO 
***********************************************************/

typedef enum
{
  START_CONNECTION,
  CONNECTING_STATE,
  TIME_SYNCING_STATE,
  PUBLISHING_STATE
}statusState_t; 

/*---------------------------- Module Functions ---------------------------*/
void setupDataPt(); 
bool publishDataPt();
bool timeForResync();
void stopCloudSM();

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
Point sensor("IAQ_Readings");  // Data point
RTC_DATA_ATTR time_t lastTmStamp = 0;  

// InfluxDB client instance with preconfigured InfluxCloud certificate
// InfluxDBClient client(INFLUXDB_URL, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
InfluxDBClient client; 
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

  // configure client
  client.setConnectionParamsV1(INFLUXDB_URL, INFLUXDB_DB_NAME, INFLUXDB_USER, INFLUXDB_PWD, nullptr);

  // Add tags to cloud data
  sensor.clearTags(); 
  sensor.addTag("Location", IAQ_SENSOR_LOCATION);
  
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
  static uint8_t influxPubCntr = 0;  // used to turn on wifi only when publishing data

  switch (currSMState)
  {
    case START_CONNECTION: 
    {
      if(ThisEvent.EventType == ES_INIT)
      {
        if(mainSMinStreamMode() && influxPubCntr != 0)
        {
          // skip wifi and just write to influx buffer
          IAQ_PRINTF("Skipping wifi\n"); 
          currSMState = PUBLISHING_STATE; 
          ES_Event_t newEvent = {.EventType=CLOUD_PUB_EVENT}; 
          RunCloudService(newEvent); 
        }
        else
        {
          // connect to wifi 
          if(wifiRetries <= MAX_WIFI_RETRIES)
          {
            // Setup wifi
            WiFi.setAutoConnect(false);
            WiFi.mode(WIFI_STA);
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD); 
            esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
            IAQ_PRINTF("Connecting to wifi ");
            ES_Timer_InitTimer(WIFI_TIMER_NUM, WIFI_POLLING_PERIOD); 
            currSMState = CONNECTING_STATE; 
          }
          else
          {
            wifiRetries = 0; 
            stopCloudSM();
            IAQ_PRINTF("Could not connect to Wifi\n");
          }
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
          IAQ_PRINTF("Connected\n");
          tmoutCounts = 0; 
          wifiRetries = 0; 

          if(timeForResync())  
          {
            IAQ_PRINTF("Syncing time"); 
            configTzTime(TZ_INFO, "pool.ntp.org");
            currSMState = TIME_SYNCING_STATE; 
            ES_Timer_InitTimer(WIFI_TIMER_NUM, TIME_SYNC_POLLING_PERIOD); 
          }
          else
          {
            //skipping time sync
            currSMState = PUBLISHING_STATE; 
            ES_Event_t newEvent = {.EventType=CLOUD_PUB_EVENT}; 
            RunCloudService(newEvent);
          }
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
            IAQ_PRINTF("Retrying\n");
          }
          else
          {
            IAQ_PRINTF(".");
            ES_Timer_InitTimer(WIFI_TIMER_NUM, WIFI_POLLING_PERIOD); 
          }
        }
      }
      
      break;
    }
    
    case TIME_SYNCING_STATE:
    {
      if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == WIFI_TIMER_NUM)
      {
        static uint8_t tmCount = 0; 
        if(time(nullptr) < 1599000000UL)
        {
          // time not synced yet 
          tmCount++; 
          if(tmCount >= (TIME_SYNC_TMOUT / TIME_SYNC_POLLING_PERIOD))
          {
            // have waited too long
            tmCount = 0; 
            IAQ_PRINTF("Could not update time\n");
            currSMState = START_CONNECTION; 
            stopCloudSM(); 
          }else
          {
            IAQ_PRINTF("."); 
            ES_Timer_InitTimer(WIFI_TIMER_NUM, TIME_SYNC_POLLING_PERIOD);  
          }
        } else
        {
          lastTmStamp = time(nullptr);

          // Show time
          tmCount = 0; 
          IAQ_PRINTF("\nSynchronized time: ");
          IAQ_PRINTF(ctime(&lastTmStamp));
          currSMState = PUBLISHING_STATE; 
          ES_Timer_InitTimer(WIFI_TIMER_NUM, PUB_DELAY_PERIOD); 
        }
      }
      break;
    }

    case PUBLISHING_STATE:
    {
      static uint8_t pubRetry = 0;  
      if((ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == WIFI_TIMER_NUM) || ThisEvent.EventType == CLOUD_PUB_EVENT)
      {
        setupDataPt(); 
        if(!publishDataPt() && pubRetry == 0)
        {
          pubRetry++; 
          IAQ_PRINTF("Retrying influxdb pub\n");
          ES_Timer_InitTimer(WIFI_TIMER_NUM, PUB_DELAY_PERIOD*4); 
        } 
        else
        {
          currSMState = START_CONNECTION; 
          pubRetry = 0; 

          influxPubCntr++; 
          if(influxPubCntr == STREAM_BATCH_SIZE)
            influxPubCntr = 0; 
          stopCloudSM(); 
        }
        
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

void stopCloudSM()
{
  currSMState = START_CONNECTION; 
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);  
  ES_Event_t NewEvent = {.EventType=CLOUD_UPDATED_EVENT};
  PostMainService(NewEvent); 
}

void setupDataPt()
{
  uint16_t batchSize = 1; 
  if(mainSMinStreamMode())
    batchSize = 3; 
    
  client.setWriteOptions(WRITE_PRECISION, batchSize, WRITE_BUFFER_SIZE, FLUSH_INTERVAL, false); 
}

bool publishDataPt()
{
  time_t tnow = time(nullptr);
  sensor.setTime(tnow);  
  IAQ_PRINTF("Writing to influx: ");
  // Serial.println(sensor.toLineProtocol()); // Print what are we exactly writing
  IAQ_PRINTF(ctime(&tnow));
  if (client.writePoint(sensor)) {
    return true; 
  }
  IAQ_PRINTF("InfluxDB write failed: ");
  // Serial.println(client.getLastErrorMessage()); 
  return false; 
}

bool timeForResync()
{
  time_t now = time(nullptr); 
  if(!isTimeSynced() || difftime(now, lastTmStamp) >=  TM_RESYNC_PERIOD)
  {
    lastTmStamp = now; 
    return true; 
  }
  return false; 
}


/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

