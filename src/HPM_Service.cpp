/****************************************************************************
 Module
   HPM_Service.c

 Description
   This is a service to handle the communication with the Honeywell HPMA115S0 sensor

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "HPM_Service.h"
#include "ES_framework.h"
#include "ES_Timers.h"

/*----------------------------- Module Defines ----------------------------*/

typedef enum{
  WAIT_START_STATE,
  WAIT_STOP_AUTO_STATE,
  ACK_STATE,
  WARMUP_STATE,
  HEAD_STATE,
  LEN_STATE,
  CMD_STATE,
  PM10_DATA_HIGH_STATE,
  PM10_DATA_LOW_STATE,
  PM25_DATA_HIGH_STATE,
  PM25_DATA_LOW_STATE,
  CHECKSUM_STATE,
  WAIT_FOR_TMOUT_STATE
} statusState_t; 

typedef enum{
  CMD_WAIT_START_STATE,
  ACK1_STATE,
  ACK2_STATE
} commSMStatus_t; 

typedef enum{
  READ_FUNC=0,
  START_MSR_FUNC,
  STOP_MSR_FUNC,
  STOP_AUTO_FUNC
} HPMCommandFunc_t; 


// #define DEBUG_SENSOR


#define SEND_BYTE_HEAD 0x68
#define RECEIVE_BYTE_HEAD 0x40
#define POS_ACK 0xA5

#define START_MEASUREMENT_LEN 0x01
#define START_MEASUREMENT_CMD 0X01

#define STOP_MEASUREMENT_LEN 0x01
#define STOP_MEASUREMENT_CMD 0x02

#define READ_MEASUREMENT_LEN 0x01
#define READ_MEASUREMENT_CMD 0x04

#define AUTO_SEND_LEN 0x01
#define STOP_AUTO_SEND_CMD 0x20

// General config 
#define NUM_SAMPLES 16
#define SUB_COMM_RETRIES 2
#define MAX_RETRY_READS 2

//Timer lenghts
#define SAMPLE_PERIOD 2000
#define STOP_AUTO_WAIT_TIME 100
#define WARMUP_WAIT_TIME 20000


/*---------------------------- Module Functions ---------------------------*/
bool retryRead(uint8_t *retryAttempts, uint16_t *checkSumVal, bool skipWarmup);
void clearSerial1Buffer();
uint16_t getChecksum(uint16_t summedVals);
void readPMSensor(); 
void stopAutoSend();
void stopMeasurements(); 
void startMeasurements();
ES_Event_t Comm_StateMachine(ES_Event_t ThisEvent, void (*cmdFunc)());
bool retryComm(uint8_t *retryAttempts, void (*cmdFunc)());


/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
static statusState_t currStatusState = WAIT_START_STATE;
static commSMStatus_t currCommSMState = CMD_WAIT_START_STATE; 
static uint8_t numGoodReads = 0;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitHPMService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and does any
     other required initialization for this service
 Notes
****************************************************************************/
bool InitHPMService(uint8_t Priority)
{
  ES_Event_t ThisEvent;
  MyPriority = Priority;

  Serial1.begin(9600);
  while (!Serial1) {
    ;
  } 

  ThisEvent.EventType = ES_READ_HPM;
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
     PostHPMService

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes
****************************************************************************/
bool PostHPMService(ES_Event_t ThisEvent)
{
  ThisEvent.ServiceNum = MyPriority; 
  return ES_PostToService(ThisEvent);
}

/****************************************************************************
 Function
    RunHPMService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   state machine that handles the serial communication with the HPM sensor. SM reads from
   HPM sensor every 1 second until it has averaged 3 data points or has made too many 
   attempts. 
 Notes
****************************************************************************/
ES_Event_t RunHPMService(ES_Event_t ThisEvent)
{
  // Serial.printf("HPM event: %d %d\n", ThisEvent.EventType, ThisEvent.EventParam);
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  static uint8_t retryAttempts = 0; 
  static uint16_t pm25Val, pm10Val = 0; 
  static float avgPM25, avgPM10 = 0.0; 
  static uint16_t checkSumVal = 0; 

  #ifdef DEBUG_SENSOR
  if(ThisEvent.EventType == ES_SERIAL1)
  {
    printf("D: %x\n", ThisEvent.EventParam);
  }
  #endif

  switch (currStatusState)
  {
    case WAIT_START_STATE:
    {
      if(ThisEvent.EventType == ES_READ_HPM)
      {
        #ifdef DEBUG_SENSOR
        printf("Starting\n");
        #endif

        stopAutoSend(); 
        currStatusState = WAIT_STOP_AUTO_STATE; 
        ES_Timer_InitTimer(HPM_TIMER_NUM, STOP_AUTO_WAIT_TIME); 
      }
      else if(ThisEvent.EventType == COMMSM_SEND || ThisEvent.EventType == ES_TIMEOUT || ThisEvent.EventType == ES_SERIAL1)
      {
        #ifdef DEBUG_SENSOR
        printf("Comm at start: ");
        #endif
        Comm_StateMachine(ThisEvent, startMeasurements); 
      }

      break;
    }

    case WAIT_STOP_AUTO_STATE: 
    {
      if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
      {
        #ifdef DEBUG_SENSOR
        printf("0: ");
        #endif

        ES_Event_t startCommEvent = {.EventType=COMMSM_SEND}; 
        Comm_StateMachine(startCommEvent, startMeasurements); 
        currStatusState = ACK_STATE; 
        ES_Timer_InitTimer(HPM_TIMER_NUM, 2000);  // serves as a back-up in case lower SM gets stuck, should never have a timeout, make at least 3x comm timer len
      }

      break;
    }

    case ACK_STATE:
    {
      if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
      {
        currCommSMState = CMD_WAIT_START_STATE; 
        retryRead(&retryAttempts, &checkSumVal, false); 
        printf("Err: Comm SM didn't respond\n");
      }
      else
      {
        #ifdef DEBUG_SENSOR
        printf("1: ");
        #endif
        ES_Event_t commReturnEvent = Comm_StateMachine(ThisEvent, startMeasurements); 
        if(commReturnEvent.EventType == ES_FAIL)
        {
          retryRead(&retryAttempts, &checkSumVal, false); 
        }
        else if(commReturnEvent.EventType == ES_SUCCESS)
        {
          currStatusState = WARMUP_STATE; 
          ES_Timer_InitTimer(HPM_TIMER_NUM, WARMUP_WAIT_TIME); 
        } 
      }

      break;
    }
    case WARMUP_STATE: 
    {
      if((ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
        || ThisEvent.EventType == ES_READ_HPM)
      {
        #ifdef DEBUG_SENSOR
        printf("2: ");
        #endif
    
        clearSerial1Buffer(); 
        readPMSensor(); 
        ES_Timer_InitTimer(HPM_TIMER_NUM, SAMPLE_PERIOD);
        currStatusState = HEAD_STATE; 
        checkSumVal = 0; 
      }
      break;
    }
    case HEAD_STATE:
    {
      if(ThisEvent.EventType == ES_SERIAL1 && ThisEvent.EventParam == RECEIVE_BYTE_HEAD)
      {
        currStatusState = LEN_STATE; 
        checkSumVal += ThisEvent.EventParam; 
        #ifdef DEBUG_SENSOR
        printf("3: ");
        #endif
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
      {
        retryRead(&retryAttempts, &checkSumVal, true);
        return ReturnEvent; 
      }
      
      break;
    }

    case LEN_STATE: 
    {
      if(ThisEvent.EventType == ES_SERIAL1)
      {
        currStatusState = CMD_STATE; 
        checkSumVal += ThisEvent.EventParam; 
        #ifdef DEBUG_SENSOR
        printf("4: ");
        #endif
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
      {
        retryRead(&retryAttempts, &checkSumVal, true);
        return ReturnEvent; 
      }

      break;
    }

    case CMD_STATE: 
    {
      if(ThisEvent.EventType == ES_SERIAL1 && ThisEvent.EventParam == READ_MEASUREMENT_CMD)
      {
        currStatusState = PM25_DATA_HIGH_STATE; 
        checkSumVal += ThisEvent.EventParam; 
        #ifdef DEBUG_SENSOR
        printf("5: ");
        #endif
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
      {
        retryRead(&retryAttempts, &checkSumVal, true);
        return ReturnEvent; 
      }

      break;
    }

    case PM25_DATA_HIGH_STATE:
    {
      if(ThisEvent.EventType == ES_SERIAL1)
      {
        checkSumVal += ThisEvent.EventParam; 
        pm25Val = ThisEvent.EventParam; 
        pm25Val <<= 8; 

        currStatusState = PM25_DATA_LOW_STATE;
        #ifdef DEBUG_SENSOR
        printf("6: ");
        #endif
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
      {
        retryRead(&retryAttempts, &checkSumVal, true);
        return ReturnEvent; 
      }
      
      break;
    }

    case PM25_DATA_LOW_STATE: 
    {
      if(ThisEvent.EventType == ES_SERIAL1)
      {
        checkSumVal += ThisEvent.EventParam; 
        ThisEvent.EventParam &= 0xFF; 
        pm25Val += ThisEvent.EventParam; 

        currStatusState = PM10_DATA_HIGH_STATE;
        #ifdef DEBUG_SENSOR
        printf("7: ");
        #endif
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
      {
        retryRead(&retryAttempts, &checkSumVal, true);
        return ReturnEvent; 
      }

      break;
    }

    case PM10_DATA_HIGH_STATE:
    {
      if(ThisEvent.EventType == ES_SERIAL1)
      {
        checkSumVal += ThisEvent.EventParam; 
        pm10Val = ThisEvent.EventParam; 
        pm10Val <<= 8; 

        currStatusState = PM10_DATA_LOW_STATE;
        #ifdef DEBUG_SENSOR
        printf("8: ");
        #endif
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
      {
        retryRead(&retryAttempts, &checkSumVal, true);
        return ReturnEvent; 
      }
      
      break;
    }

    case PM10_DATA_LOW_STATE: 
    {
      if(ThisEvent.EventType == ES_SERIAL1)
      {
        checkSumVal += ThisEvent.EventParam; 
        ThisEvent.EventParam &= 0xFF; 
        pm10Val += ThisEvent.EventParam; 

        currStatusState = CHECKSUM_STATE; 
        #ifdef DEBUG_SENSOR
        printf("9: ");
        #endif
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
      {
        retryRead(&retryAttempts, &checkSumVal, true);
        return ReturnEvent; 
      }

      break;
    }

    case CHECKSUM_STATE:
    {
      if(ThisEvent.EventType == ES_SERIAL1 && ThisEvent.EventParam == getChecksum(checkSumVal))
      {
        if(numGoodReads == 0)
        {
          avgPM25 = 0.0; 
          avgPM10 = 0.0; 
        }
        printf("HPM 2.5: %d\n", pm25Val);
        printf("HPM 10: %d\n", pm10Val); 
        avgPM25 += (float)pm25Val / (float)NUM_SAMPLES; 
        avgPM10 += (float)pm10Val / (float)NUM_SAMPLES; 

        checkSumVal = 0; 
        retryAttempts = 0; 
        currStatusState = WAIT_FOR_TMOUT_STATE; 
      }
      else if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
      {
        retryRead(&retryAttempts, &checkSumVal, true);
        return ReturnEvent; 
      }
      
      break;
    }

    case WAIT_FOR_TMOUT_STATE: 
    {
      if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == HPM_TIMER_NUM)
      {

        numGoodReads++; 
        if(numGoodReads < NUM_SAMPLES)
        {
          #ifdef DEBUG_SENSOR
          printf("Again: \n");
          #endif
          currStatusState = WARMUP_STATE;
          ES_Event_t newEvent = {.EventType=ES_READ_HPM};
          PostHPMService(newEvent);
        }
        else
        {
          currStatusState = WAIT_START_STATE;
          numGoodReads = 0; 
          printf("Avg PM 2.5: %f\n", avgPM25); 
          printf("Avg PM 10: %f\n", avgPM10); 
          avgPM25 = 0.0; 
          avgPM10 = 0.0; 
          #ifdef DEBUG_SENSOR
          printf("Done reading\n");
          #endif

          //stop the fan now
          ES_Event_t startCommEvent = {.EventType=COMMSM_SEND}; 
          Comm_StateMachine(startCommEvent, stopMeasurements); 
        }
      }
      break;
    }
  }

  return ReturnEvent;
}


bool EventCheckerHPM(){
  if(Serial1.available())
  {
    ES_Event_t newEvent; 
    newEvent.EventType = ES_SERIAL1; 
    newEvent.EventParam = Serial1.read(); 
    PostHPMService(newEvent); 

    return true; 
  }

  return false; 
}



/***************************************************************************
 private functions
 ***************************************************************************/

bool retryRead(uint8_t *retryAttempts, uint16_t *checkSumVal, bool skipWarmup)
{
  *checkSumVal = 0; 
  ES_Timer_StopTimer(HPM_TIMER_NUM);  // in case any timer is running

  if(skipWarmup)
    currStatusState = WARMUP_STATE;  // Fan has already started. No need to retry this
  else
    currStatusState = WAIT_START_STATE;

  if(*retryAttempts >= MAX_RETRY_READS)
  {
    numGoodReads = 0; 
    *retryAttempts = 0; 
    printf("Could not read from HPM sensor\n");
    return false; 
  }

  #ifdef DEBUG_SENSOR
  printf("Retrying: %d\n", *retryAttempts);
  #endif
  *retryAttempts = *retryAttempts + 1; 
  ES_Event_t newEvent = {.EventType=ES_READ_HPM};
  PostHPMService(newEvent);
  return true; 
}

void clearSerial1Buffer()
{
  if(Serial1.available())
    printf("Cleared data\n");

  while(Serial1.available())
  {
    Serial1.read(); 
  }
}

void stopAutoSend()
{
  #ifdef DEBUG_SENSOR
  printf("Stopping auto send\n");
  #endif
  Serial1.write(SEND_BYTE_HEAD); 
  Serial1.write(AUTO_SEND_LEN);  
  Serial1.write(STOP_AUTO_SEND_CMD); 
  Serial1.write(0x77);  // Checksum 
}

void stopMeasurements()
{
  #ifdef DEBUG_SENSOR
  printf("Stopping measurements\n");
  #endif
  Serial1.write(SEND_BYTE_HEAD); 
  Serial1.write(STOP_MEASUREMENT_LEN);  
  Serial1.write(STOP_MEASUREMENT_CMD); 
  Serial1.write(0x95);  // Checksum 
}

void startMeasurements()
{
  #ifdef DEBUG_SENSOR
  printf("Starting measurements\n");
  #endif
  Serial1.write(SEND_BYTE_HEAD); 
  Serial1.write(START_MEASUREMENT_LEN); 
  Serial1.write(START_MEASUREMENT_CMD); 
  Serial1.write(0x96); 
}

void readPMSensor()
{
  #ifdef DEBUG_SENSOR
  printf("Reading sensor\n");
  #endif
  Serial1.write(SEND_BYTE_HEAD); 
  Serial1.write(READ_MEASUREMENT_LEN); 
  Serial1.write(READ_MEASUREMENT_CMD); 
  Serial1.write(0x93); 
}

uint16_t getChecksum(uint16_t summedVals)
{
  return (65536 - (uint32_t)summedVals) % 256; 
}



//Lower level state machine used to handle simple pos/neg ack communicataion 
//with the HPM sensor 
ES_Event_t Comm_StateMachine(ES_Event_t ThisEvent, void (*cmdFunc)())
{
  #ifdef DEBUG_SENSOR
  printf("Comm event: %d %d\n", ThisEvent.EventType, ThisEvent.EventParam);
  #endif
  ES_Event_t returnEvent = {.EventType=ES_NO_EVENT};
  static uint8_t retryAttempts = 0; 

  if(cmdFunc == NULL)
  {
    returnEvent.EventType = ES_FAIL; 
    return returnEvent; 
  }

  switch (currCommSMState)
  {
    case CMD_WAIT_START_STATE:
    {
      #ifdef DEBUG_SENSOR
      printf("Comm 1: \n");
      #endif
      if(ThisEvent.EventType == COMMSM_SEND)
      {
        clearSerial1Buffer(); 
        cmdFunc(); 
        currCommSMState = ACK1_STATE; 
        ES_Timer_InitTimer(COMMSM_TIMER_NUM, 500);
      }
      break;
    }

    case ACK1_STATE: 
    {
      #ifdef DEBUG_SENSOR
      printf("Comm 2: \n");
      #endif
      if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == COMMSM_TIMER_NUM)
      {
        if(!retryComm(&retryAttempts, cmdFunc))
        {
          returnEvent.EventType = ES_FAIL; 
        }
      }
      else if(ThisEvent.EventType == ES_SERIAL1 && ThisEvent.EventParam == POS_ACK)
      {
        currCommSMState = ACK2_STATE; 
      }
      break;
    }

    case ACK2_STATE: 
    {
      #ifdef DEBUG_SENSOR
      printf("Comm 3: \n");
      #endif
      if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == COMMSM_TIMER_NUM)
      {
        if(!retryComm(&retryAttempts, cmdFunc))
        {
          returnEvent.EventType = ES_FAIL; 
          printf("Comm fail\n");
        }
      }
      else if(ThisEvent.EventType == ES_SERIAL1 && ThisEvent.EventParam == POS_ACK)
      {
        currCommSMState = CMD_WAIT_START_STATE;
        returnEvent.EventType = ES_SUCCESS;  
        retryAttempts = 0; 
        ES_Timer_StopTimer(COMMSM_TIMER_NUM); 
        #ifdef DEBUG_SENSOR
        printf("Comm success\n");
        #endif
      }
      break;
    }
  }

  return returnEvent;
}


bool retryComm(uint8_t *retryAttempts, void (*cmdFunc)())
{
  currCommSMState = CMD_WAIT_START_STATE; 
  if(*retryAttempts >= SUB_COMM_RETRIES)
  {
    *retryAttempts = 0; 
    ES_Timer_StopTimer(COMMSM_TIMER_NUM);
    printf("Failed sub comm SM\n");
    return false; 
  }

  #ifdef DEBUG_SENSOR
  printf("Subcomm retry %d\n", *retryAttempts);
  #endif
  *retryAttempts = *retryAttempts + 1; 
  ES_Event_t newEvent = {.EventType=COMMSM_SEND};
  Comm_StateMachine(newEvent, cmdFunc);
  return true; 
}
/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

