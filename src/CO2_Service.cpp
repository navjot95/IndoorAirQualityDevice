/****************************************************************************
 Module
   CO2_Service.c

 Description
   This is a service to handle the communication with the MH-Z19B CO2 sensor

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "CO2_Service.h"
#include "ES_framework.h"
#include "ES_Timers.h"

/*----------------------------- Module Defines ----------------------------*/
// TODO: Add 2 min of warmup time before reading the sensor
typedef enum{
  SEND_STATE,
  START_SM_STATE,
  SENSOR_NUM_STATE, 
  SENSOR_HIGH_VAL_STATE,
  SENSOR_LOW_VAL_STATE,
  EMPTY_STATE,
  CHECKSUM_STATE
} statusState_t; 

// #define DEBUG_SENSOR

#define START_BYTE 0XFF
#define SENSOR_NUM 0X86
#define NUM_OF_EMPTY_BYTES 4
#define MAX_RETRY_READS 2

/*---------------------------- Module Functions ---------------------------*/
bool retryRead(uint8_t *retryAttempts, uint8_t *checkSumVal);
uint8_t getCheckSum(uint8_t readSum);
void clearSerial2Buffer();


/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
static statusState_t currStatusState = SEND_STATE;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitCO2Service

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes
****************************************************************************/
bool InitCO2Service(uint8_t Priority)
{
  MyPriority = Priority;

  Serial2.begin(9600);
  while (!Serial2) {
    ;
  } 
  
  return true; 
}

/****************************************************************************
 Function
     PostCO2Service

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes
****************************************************************************/
bool PostCO2Service(ES_Event_t ThisEvent)
{
  ThisEvent.ServiceNum = MyPriority; 
  return ES_PostToService(ThisEvent);
}

/****************************************************************************
 Function
    RunCO2Service

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   state machine that handles the serial communication with the CO2 sensor 
 Notes
****************************************************************************/
ES_Event_t RunCO2Service(ES_Event_t ThisEvent)
{
  // Serial.printf("CO2 event: %d %d\n", ThisEvent.EventType, ThisEvent.EventParam);
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  static uint8_t retryAttempts = 0; 
  static uint16_t sensorVal = 0; 
  static uint8_t checkSumVal = 0; 

  if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == CO2_TIMER_NUM && currStatusState != SEND_STATE)
  {
    retryRead(&retryAttempts, &checkSumVal);
    return ReturnEvent; 
  }

  #ifdef DEBUG_SENSOR
  if(ThisEvent.EventType == ES_SERIAL2)
  {
    printf("D: %x\n", ThisEvent.EventParam);
  }
  #endif

  switch (currStatusState)
  {
    case SEND_STATE:
    {
      if(ThisEvent.EventType == ES_READ_SENSOR || (ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == CO2_TIMER_NUM))
      {
        clearSerial2Buffer(); 
        Serial2.write(0xFF);
        Serial2.write(0x01); 
        Serial2.write(0x86);
        Serial2.write(0x79);

        currStatusState = START_SM_STATE; 
        uint16_t waitTime = 1000 * (0x01 << retryAttempts);  // wait exponentially longer each time 
        ES_Timer_InitTimer(CO2_TIMER_NUM, waitTime);
        checkSumVal = 0;  // just to be safe 
      }

      break;
    }
    case START_SM_STATE:
    {
      if(ThisEvent.EventType == ES_SERIAL2 && ThisEvent.EventParam == START_BYTE)
      {
        currStatusState = SENSOR_NUM_STATE; 
      }
      
      break;
    }
    case SENSOR_NUM_STATE:
    {
      if(ThisEvent.EventType == ES_SERIAL2 && ThisEvent.EventParam == SENSOR_NUM)
      {
        currStatusState = SENSOR_HIGH_VAL_STATE; 
        checkSumVal += (uint8_t)ThisEvent.EventParam; 
      }
      
      break;
    }
    case SENSOR_HIGH_VAL_STATE:
    {
      if(ThisEvent.EventType == ES_SERIAL2)
      {
        currStatusState = SENSOR_LOW_VAL_STATE;
        sensorVal = ThisEvent.EventParam << 8;  
        checkSumVal += (uint8_t)ThisEvent.EventParam; 
        // printf("High Data: %d ", (uint8_t)ThisEvent.EventParam);
      }
      
      break;
    }
    case SENSOR_LOW_VAL_STATE:
    {
      if(ThisEvent.EventType == ES_SERIAL2)
      {
        currStatusState = EMPTY_STATE;
        ThisEvent.EventParam &= 0xFF;  // clear out top byte just to be safe
        sensorVal |= ThisEvent.EventParam;  
        checkSumVal += (uint8_t)ThisEvent.EventParam; 
        // printf("Low Data: %d ", (uint8_t)ThisEvent.EventParam);
      }
      
      break;
    }
    case EMPTY_STATE:
    {
      static uint8_t counter = 0; 
      if(ThisEvent.EventType == ES_SERIAL2)
      {
        counter++; 
        checkSumVal += (uint8_t)ThisEvent.EventParam; 
        if(counter == NUM_OF_EMPTY_BYTES)
        {
          currStatusState = CHECKSUM_STATE; 
          counter = 0; 
        }
        else if(counter > NUM_OF_EMPTY_BYTES)
        {
          counter = 0; 
          printf("Too many empty bytes from CO2 reading");
          retryRead(&retryAttempts, &checkSumVal); 
        }
      }
      
      break;
    }

    case CHECKSUM_STATE:
    {
      if(ThisEvent.EventType == ES_SERIAL2)
      {
        if(ThisEvent.EventParam == getCheckSum(checkSumVal))
        {
          updateCO2Val(sensorVal); 
          printf("CO2 val: %d\n", sensorVal); 
          checkSumVal = 0; 
          retryAttempts = 0; 
          currStatusState = SEND_STATE; 
          // ES_Timer_StopTimer(CO2_TIMER_NUM);
          ES_Timer_InitTimer(CO2_TIMER_NUM, 1000); 
        }
        else
        {
          retryRead(&retryAttempts, &checkSumVal); 
        }
      }
      
      break;
    }
  }

  return ReturnEvent;
}


bool EventCheckerCO2(){
  if(Serial2.available())
  {
    ES_Event_t newEvent; 
    newEvent.EventType = ES_SERIAL2; 
    newEvent.EventParam = Serial2.read(); 
    PostCO2Service(newEvent); 

    return true; 
  }

  return false; 
}



/***************************************************************************
 private functions
 ***************************************************************************/

bool retryRead(uint8_t *retryAttempts, uint8_t *checkSumVal)
{
  if(retryAttempts == NULL || checkSumVal == NULL)
    return false; 

  *checkSumVal = 0; 
  currStatusState = SEND_STATE; 
  if(*retryAttempts >= MAX_RETRY_READS)
  {
    *retryAttempts = 0; 
    ES_Timer_StopTimer(CO2_TIMER_NUM);
    updateCO2Val(-1); 
    printf("Could not read from CO2 sensor\n");
    return false; 
  }

  printf("Retrying\n");
  *retryAttempts = *retryAttempts + 1; 
  ES_Event_t newEvent = {.EventType=ES_READ_SENSOR};
  PostCO2Service(newEvent);
  return true; 
}

uint8_t getCheckSum(uint8_t readSum)
{
  readSum = 0xFF - readSum; 
  return (readSum + 1);
}

void clearSerial2Buffer()
{
  while(Serial2.available())
  {
    printf("Cleared data CO2\n");
    Serial2.read(); 
  }
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

