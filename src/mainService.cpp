/****************************************************************************
 Module
   MainService.c

 Description
   This is a main state machine for the device.

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "MainService.h"
#include "ES_framework.h"
#include "ES_Timers.h"

/*----------------------------- Module Defines ----------------------------*/
#define LOOP_TIMER_LEN 60000
#define ALL_SENSORS_READ 0X07
#define BAT_LOW_THRES 3500

typedef enum
{
  CO2Idx = 0,
  HPMIdx,
  SVM30Idx
}sensorIdx_t;

/*---------------------------- Module Functions ---------------------------*/
void initPins();
void sensorsPwrEnable(bool turnOn);
void setFlagBit(sensorIdx_t sensorBitIdx);
void shutdownIAQ();

/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
static uint8_t sensorReads_flag = 0x00; 
static IAQsensorVals_t sensorReads = {.eCO2=-1, .tVOC=-1, .PM25=-1, .PM10=-1, .CO2=-1, .temp=-1, .rh=-1};

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

  if((ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == MAIN_SERV_TIMER_NUM) || ThisEvent.EventType == ES_INIT)
  {
    ES_TimerReturn_t returnVal = ES_Timer_InitTimer(MAIN_SERV_TIMER_NUM, LOOP_TIMER_LEN);  //restart timer
    if(returnVal == ES_Timer_ERR)
      Serial.println("Main timer err\n");

    uint16_t batV = analogRead(BAT_PIN) * 2; 
    if(batV < BAT_LOW_THRES)
    {
      ePaperChangeHdln("Device OFF", false);
      ePaperPrintfAlert("Low Battery", "Please plug in and press", "button when charged.");
      shutdownIAQ();
      return ReturnEvent; 
    }

    // Read values from sensors 
    sensorsPwrEnable(true); 
    ES_Event_t NewEvent = {.EventType=ES_READ_SENSOR}; 
    PostHPMService(NewEvent); 
    PostCO2Service(NewEvent); 
    PostSVM30Service(NewEvent);  // Worst case senario, could hang for 60 secs
    ePaperChangeHdln("Reading..."); 

  }else if(ThisEvent.EventType == UPDATES_DONE_EVENT)
  {
    //every hour package data and send to cloud 
    //go to sleep 
    if(sensorReads_flag != ALL_SENSORS_READ)
    {
      printf("False flag set\n");
    }
    else
    {
      printf("Recording val for cloud\n");
    }
    
    

  }else if(ThisEvent.EventType == ES_SW_BUTTON_PRESS && ThisEvent.EventParam == LONG_BT_PRESS)
  {
    printf("Going into deep sleep\n");
    shutdownIAQ(); 
  }
  
  return ReturnEvent;
}


void updateCO2Val(int16_t CO2_newVal)
{
  sensorReads.CO2 = CO2_newVal; 
  sensorIdx_t thisSensor = CO2Idx; 
  setFlagBit(thisSensor); 
}

void updateHPMVal(int16_t PM25_newVal, int16_t PM10_newVal)
{
  sensorReads.PM25 = PM25_newVal;
  sensorReads.PM10 = PM10_newVal;  
  sensorIdx_t thisSensor = HPMIdx; 
  setFlagBit(thisSensor); 
}

void updateSVM30Vals(int16_t eCO2_newVal, int16_t tVOC_newVal, int16_t tm_newVal, int16_t rh_newVal)
{
  sensorReads.eCO2 = eCO2_newVal;
  sensorReads.tVOC = tVOC_newVal; 
  sensorReads.temp = tm_newVal; 
  sensorReads.rh = rh_newVal; 
  sensorIdx_t thisSensor = SVM30Idx;
  setFlagBit(thisSensor); 
} 



/***************************************************************************
 private functions
 ***************************************************************************/
//shut down the sensors and go into deep sleep
void shutdownIAQ()
{
  //TODO: shut down escreen 
  sensorsPwrEnable(false);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_26,1); //1 = High, 0 = Low
  esp_deep_sleep_start();
}


void setFlagBit(sensorIdx_t sensorBitIdx)
{
  if(sensorBitIdx > 7)
    return; 
  
  sensorReads_flag |= 0x01 << sensorBitIdx; 

  if(sensorReads_flag == ALL_SENSORS_READ)
  {
    // ES_Event_t newEvent = {.EventType=UPDATES_DONE_EVENT};
    // PostMainService(newEvent); 
    sensorReads_flag = 0x00; 
    updateScreenSensorVals(sensorReads);
    //TODO: post to cloud 
  }
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


/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

