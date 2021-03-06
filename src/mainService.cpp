/****************************************************************************
 Module
   MainService.c

 Description
   This is a main state machine for the device.

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "MainService.h"
#include "ePaperDriver.h"
#include "IAQ_util.h"
#include "ES_framework.h"
#include "ES_Timers.h"
#include "CloudService.h"
#include <WiFi.h>
#include <driver/adc.h>
#include <esp_wifi.h>
#include <esp_bt.h>


/*----------------------------- Module Defines ----------------------------*/
#define ALL_SENSORS_READ 0x07
#define BAT_LOW_THRES 3500  // voltage at which to go into hibernation mode

#define AUTO_MODE_TIMER_LEN 300000U  // Time in ms. Back up timer for timeouts 
#define STREAM_MODE_TIMER_LEN 6000U  // Time in ms. Screen update period
#define WARMUP_TIMER_LEN 197000U  // Time in ms 3mins for CO2 sensor + 16 secs for polling 16 vals
#define DEEP_SLEEP_TIME 900000000UL  // Length of time to go into deep sleep for in auto mode

#define WIFI_TIMEOUT_LEN  36000U  // ms
#define BAT_POLLING_PERIOD 1000  // updates battery run avg and low volt check at this interval 

#define CLOUD_COUNTER_LEN 50  // Cloud updates every this many screen refreshes (make 5+)   


typedef enum
{
  CO2Idx = 0,
  HPMIdx,
  SVM30Idx
}sensorIdx_t;

typedef enum
{
  START_STATE,
  AUTO_STATE,
  STREAM_STATE,
  STREAM_CLOUD_STATE
}statusState_t; 


/*---------------------------- Module Functions ---------------------------*/
void initPins();
void sensorsPwrEnable(bool turnOn);
void setFlagBit(sensorIdx_t sensorBitIdx);
void shutdownIAQ(bool timedShtdwn);
void shutdownBat();
void changeSensorsIAQMode(IAQmode_t currIAQMode);
void startSensorsSM();

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
static uint8_t sensorReads_flag = 0x00; 
static statusState_t currSMState = START_STATE; 
RTC_DATA_ATTR IAQsensorVals_t sensorReads = {.eCO2=-1, .tVOC=-1, .PM25=-1, .PM10=-1, .CO2=-1, .temp=-1, .rh=-1}; 
RTC_DATA_ATTR time_t lastUpdateTime = 0;  // last time screen sensor values were updated

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitMainService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes
****************************************************************************/
bool InitMainService(uint8_t Priority)
{
  ES_Event_t ThisEvent;
  MyPriority = Priority;
  
  initPins(); 
  initePaper();
  adc_power_on();
  btStop();  // Make sure bluetooth is off
  ES_Timer_InitTimer(BAT_TIMER_NUM, BAT_POLLING_PERIOD); 

  for(uint8_t i=0; i<RUN_AVG_BUFFER_LEN; i++)
  {
    getBatVolt();  // fill up running avg buffer to curr val 
  }
  if(getBatVolt() < BAT_LOW_THRES)
  {
    shutdownBat(); 
    return false; 
  }  

  setenv("TZ", "PST8PDT", 1); // set the correct timezone for time.h  
  tzset();

  char str[20]; 
  if(isTimeSynced())
    getCurrTime(str, 20, NULL); 
  else 
    strcpy(str, "Time not synced");

  IAQ_PRINTF("Start time: %s\n", str); 

  // post the initial transition event
  ThisEvent.EventType = ES_INIT;
  ThisEvent.ServiceNum = Priority;
  if (ES_PostToService(ThisEvent) == true)
  {
    return true;
  }
  else
  {
    return false;
  }
}

/****************************************************************************
 Function
     PostMainService

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes
****************************************************************************/
bool PostMainService(ES_Event_t ThisEvent)
{
  ThisEvent.ServiceNum = MyPriority; 
  return ES_PostToService(ThisEvent);
}

/****************************************************************************
 Function
    RunMainService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   main state machine for the device
 Notes
****************************************************************************/
ES_Event_t RunMainService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  static uint8_t cloudUpdateCounter = 0; 

  if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == BAT_TIMER_NUM)
  {
    ES_Timer_InitTimer(BAT_TIMER_NUM, BAT_POLLING_PERIOD); 
    if(getBatVolt() < BAT_LOW_THRES)
    {
      shutdownBat(); 
      return ReturnEvent; 
    }    
  }
  if(ThisEvent.EventType == ES_SW_BUTTON_PRESS && ThisEvent.EventParam == LONG_BT_PRESS)
  {
    memset(&sensorReads,  -1, sizeof(sensorReads)); 
    lastUpdateTime = 1;  // No time will be shown at wakeup
    ePaperChangeHdln("Device OFF", NO_SCREEN_REFRESH, NO_MODE);
    updateEpaperTime(0); 
    updateScreenSensorVals(&sensorReads, true, false);
    shutdownIAQ(false); 
    return ReturnEvent; 
  }

  switch (currSMState)
  {
    case START_STATE: 
    {
      if(ThisEvent.EventType == ES_INIT)
      {
        sensorsPwrEnable(true); 
        uint32_t timerLen; 
        IAQmode_t currIAQMode = STREAM_MODE; 

        esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
        if(wakeup_reason == ESP_SLEEP_WAKEUP_TIMER)
        {
          currSMState = AUTO_STATE; 
          timerLen = AUTO_MODE_TIMER_LEN;  // backup timer 
          currIAQMode = AUTO_MODE;

          ePaperChangeHdln("Reading...", NO_SCREEN_REFRESH, currIAQMode);
          IAQ_PRINTF("Updating screen 1: %lu\n", lastUpdateTime);
          updateEpaperTime(lastUpdateTime); 
          updateScreenSensorVals(&sensorReads, false, false);  // load last sensor data
        } 
        else
        {
          currSMState = STREAM_STATE; 
          timerLen = WARMUP_TIMER_LEN;  // polling timer
          currIAQMode = STREAM_MODE; 
          ePaperChangeHdln("Warming up...", NO_SCREEN_REFRESH, currIAQMode);
          IAQ_PRINTF("Updating screen 2: %lu\n", lastUpdateTime);
          updateEpaperTime(lastUpdateTime); 
          updateScreenSensorVals(&sensorReads, true, false);
        }

        changeSensorsIAQMode(currIAQMode); 
        startSensorsSM(); 
        ES_TimerReturn_t returnVal = ES_Timer_InitTimer(MAIN_SERV_TIMER_NUM, timerLen);  // CO2 sensor has 3 min warmup time
        IAQ_PRINTF("Going into %s\n", (currIAQMode == STREAM_MODE)? "STREAM MODE" : "AUTO MODE"); 
        if(returnVal == ES_Timer_ERR)
        {
          IAQ_PRINTF("Main timer err\n");
        }
      }
      break;
    }

    case AUTO_STATE:
    {
      if(ThisEvent.EventType == SENSORS_READ_EVENT)  // Wait until have heard back from all sensors
      {
        IAQ_PRINTF("All sensors read\n"); 
        updateCloudSensorVals(&sensorReads);  // update values to be sent to cloud
        ES_Event_t newEvent = {.EventType=ES_INIT};
        PostCloudService(newEvent);
      }
      else if(ThisEvent.EventType == CLOUD_UPDATED_EVENT)
      {
        lastUpdateTime = updateEpaperTime(0);
        IAQ_PRINTF("Updating screen 3: %lu\n", lastUpdateTime);
        updateScreenSensorVals(&sensorReads, true, true);
        shutdownIAQ(true); 
      } 
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == MAIN_SERV_TIMER_NUM)
      {
        // 1+ sensor or cloud service didn't respond in time
        IAQ_PRINTF("Main backup timer timedout\n");
        shutdownIAQ(true); 
      }
      else if(ThisEvent.EventType == ES_SW_BUTTON_PRESS && ThisEvent.EventParam == SHORT_BT_PRESS)
      {
        IAQ_PRINTF("Changing to stream from auto\n");

        currSMState = STREAM_STATE; 
        changeSensorsIAQMode(STREAM_MODE); 

        if(sensorReads_flag != 0)
        {
          IAQ_PRINTF("Starting stream from beginning\n");
          ES_Timer_InitTimer(MAIN_SERV_TIMER_NUM, WARMUP_TIMER_LEN);
          startSensorsSM();  // one of the sensors finished, so restart its SM
        }
        else
        {
          IAQ_PRINTF("Starting stream not from beginning\n");
          ES_Timer_InitTimer(MAIN_SERV_TIMER_NUM, STREAM_MODE_TIMER_LEN);
        }
        ePaperChangeHdln("Warming up...", PARTIAL_SCREEN_REFRESH, STREAM_MODE);
      }
      break;
    }

    case STREAM_STATE: 
    {
      if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == MAIN_SERV_TIMER_NUM)
      {
        ES_Timer_InitTimer(MAIN_SERV_TIMER_NUM, STREAM_MODE_TIMER_LEN);  // screen update timer for stream
        // Poll the sensors for their latest values 
        getPMAvg(&(sensorReads.PM10), &(sensorReads.PM25));
        getSVM30Avg(&(sensorReads.eCO2), &(sensorReads.tVOC), &(sensorReads.temp), &(sensorReads.rh)); 
        getCO2Avg(&(sensorReads.CO2));

        if(isTimeSynced())
        {
          lastUpdateTime = updateEpaperTime(0);
          updateScreenSensorVals(&sensorReads, false, true);  
          if(cloudUpdateCounter % CLOUD_COUNTER_LEN == 0)
          {
            updateCloudSensorVals(&sensorReads); 
            ES_Event_t newEvent = {.EventType=ES_INIT};
            PostCloudService(newEvent);
          }
        }
        else
        {
          IAQ_PRINTF("Time not synced, so connecting to wifi\n");
          if(cloudUpdateCounter % CLOUD_COUNTER_LEN == 0)
          {
            updateCloudSensorVals(&sensorReads); 
            ES_Event_t newEvent = {.EventType=ES_INIT};
            PostCloudService(newEvent);
            currSMState = STREAM_CLOUD_STATE; 
            ES_Timer_InitTimer(MAIN_SERV_TIMER_NUM, WIFI_TIMEOUT_LEN);  // serves as backup timeout timer
          }else
          {
            lastUpdateTime = updateEpaperTime(0);
            updateScreenSensorVals(&sensorReads, false, true);
          } 
        }
        cloudUpdateCounter++; 
        if(cloudUpdateCounter % CLOUD_COUNTER_LEN == 0)
          cloudUpdateCounter = 0; 

      }else if(ThisEvent.EventType == ES_SW_BUTTON_PRESS && ThisEvent.EventParam == SHORT_BT_PRESS)
      {
        IAQ_PRINTF("Changing to Auto from stream, partial refresh\n");
        currSMState = AUTO_STATE; 
        changeSensorsIAQMode(AUTO_MODE); 
        ePaperChangeHdln("Reading...", PARTIAL_SCREEN_REFRESH, AUTO_MODE); 
        ES_Timer_InitTimer(MAIN_SERV_TIMER_NUM, AUTO_MODE_TIMER_LEN); 
      }
      
      break;  
    }

    case STREAM_CLOUD_STATE:
    {
      if(ThisEvent.EventType == CLOUD_UPDATED_EVENT || (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == MAIN_SERV_TIMER_NUM))
      {
        IAQ_PRINTF("Updating screen 6\n");
        lastUpdateTime = updateEpaperTime(0);
        updateScreenSensorVals(&sensorReads, false, true);
        currSMState = STREAM_STATE; 
        ES_Timer_InitTimer(MAIN_SERV_TIMER_NUM, STREAM_MODE_TIMER_LEN);
      }
      else if(ThisEvent.EventType == ES_SW_BUTTON_PRESS && ThisEvent.EventParam == SHORT_BT_PRESS)
      {
        IAQ_PRINTF("Changing to Auto from stream clould state, partial refresh\n");
        currSMState = AUTO_STATE; 
        changeSensorsIAQMode(AUTO_MODE); 
        ePaperChangeHdln("Reading...", PARTIAL_SCREEN_REFRESH, AUTO_MODE); 
        ES_Timer_InitTimer(MAIN_SERV_TIMER_NUM, AUTO_MODE_TIMER_LEN); 
      }
      break;
    }
  }
  
  return ReturnEvent;
}


void updateCO2Val(int16_t CO2_newVal)
{
  sensorReads.CO2 = CO2_newVal; 
  setFlagBit(CO2Idx); 
}

void updateHPMVal(int16_t PM25_newVal, int16_t PM10_newVal)
{
  sensorReads.PM25 = PM25_newVal;
  sensorReads.PM10 = PM10_newVal;  
  setFlagBit(HPMIdx); 
}

void updateSVM30Vals(int16_t eCO2_newVal, int16_t tVOC_newVal, int16_t tm_newVal, int16_t rh_newVal)
{
  sensorReads.eCO2 = eCO2_newVal;
  sensorReads.tVOC = tVOC_newVal; 
  sensorReads.temp = tm_newVal; 
  sensorReads.rh = rh_newVal; 
  setFlagBit(SVM30Idx); 
} 

bool mainSMinStreamMode()
{
  if(currSMState == STREAM_STATE)
    return true; 
  else 
    return false; 
}



/***************************************************************************
 private functions
 ***************************************************************************/
//shut down the sensors and go into deep sleep
// timedShtdwn as true means timed sleep 
void shutdownIAQ(bool timedShtdwn)
{
  IAQ_PRINTF("Going into deep sleep\n");
  char str[20]; 
  getCurrTime(str, 20, NULL); 
  IAQ_PRINTF(str); 

  stopHPMMeasurements();  // turns off HPM fan
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  adc_power_off();

  sensorsPwrEnable(false);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_26,1); //1 = High, 0 = Low 
  if(timedShtdwn)
    esp_deep_sleep(DEEP_SLEEP_TIME);
  else 
    esp_deep_sleep_start(); 

}


void setFlagBit(sensorIdx_t sensorBitIdx)
{
  if(sensorBitIdx > 7)
    return; 
  
  sensorReads_flag |= 0x01 << sensorBitIdx; 

  if(sensorReads_flag == ALL_SENSORS_READ)
  {
    ES_Timer_StopTimer(MAIN_SERV_TIMER_NUM);
    sensorReads_flag = 0x00; 
    ES_Event_t newEvent = {.EventType=SENSORS_READ_EVENT};
    RunMainService(newEvent); 
  }
}

void changeSensorsIAQMode(IAQmode_t currIAQMode)
{
  sensorReads_flag = 0; 
  setModeHPM(currIAQMode); 
  setModeSVM30(currIAQMode); 
  setModeCO2(currIAQMode); 
}

void startSensorsSM()
{
  ES_Event_t NewEvent = {.EventType=ES_READ_SENSOR};
  PostHPMService(NewEvent); 
  PostSVM30Service(NewEvent);  // Worst case senario, could hang for 60 secs 
  PostCO2Service(NewEvent); 
}

void initPins()
{
  pinMode(PWR_EN_PIN, OUTPUT); 
  digitalWrite(PWR_EN_PIN, HIGH); 

  pinMode(BUTTON_PIN, INPUT); 
}

void sensorsPwrEnable(bool turnOn)
{
  if(turnOn)
    digitalWrite(PWR_EN_PIN, HIGH);
  else
    digitalWrite(PWR_EN_PIN, LOW);  
}

// shuts down device because battery voltage too low
void shutdownBat()
{
  IAQ_PRINTF("Low bat. Going to sleep.\n");
  ePaperChangeHdln("Device OFF", NO_SCREEN_REFRESH, NO_MODE);
  ePaperPrintfAlert("Low Battery", "Please plug in and press", "button when charged.");
  delay(1000); 
  shutdownIAQ(true);  // check back in periodically
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

