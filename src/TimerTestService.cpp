/****************************************************************************
 Module
   TimerTestService.c

 Description
   This is a test file for implementing a software timer from ES_Timers.cpp 

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "TimerTestService.h"
#include "ES_framework.h"
#include "ES_Timers.h"

/*----------------------------- Module Defines ----------------------------*/
#define TIMER_NUM 15
#define TIMER_LEN 60000  // in ms


/*---------------------------- Module Functions ---------------------------*/


/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitTimerTestService

 Parameters
     uint8_t : the priorty of this service

 Returns
     bool, false if error in initialization, true otherwise

 Description
     Saves away the priority, and starts up the timer

 Notes
****************************************************************************/
bool InitTimerTestService(uint8_t Priority)
{
  // ES_Event_t ThisEvent;
  MyPriority = Priority;

  // pinMode(TIMER_TEST_PIN, OUTPUT); 
  // ES_TimerReturn_t returnVal = ES_Timer_InitTimer(TIMER_NUM, TIMER_LEN); 

  // if(returnVal == ES_Timer_ERR)
  // {
  //   Serial.println("Timer err at init\n");
  // }
 
  // post the initial transition event
  // ThisEvent.EventType = ES_INIT;
  // ThisEvent.ServiceNum = Priority; 
  // if (ES_PostToService(ThisEvent) == true)
  // {
  //   return true;
  // }
  // else
  // {
  //   return false;
  // }
  return true; 
}

/****************************************************************************
 Function
     PostTimerTestService

 Parameters
     EF_Event_t ThisEvent ,the event to post to the queue

 Returns
     bool false if the Enqueue operation failed, true otherwise

 Description
     Posts an event to this state machine's queue
 Notes
****************************************************************************/
bool PostTimerTestService(ES_Event_t ThisEvent)
{
  ThisEvent.ServiceNum = MyPriority; 
  return ES_PostToService(ThisEvent);
}

/****************************************************************************
 Function
    RunTimerTestService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   toggles I/O pin when the software timer times out and resets it.
 Notes
****************************************************************************/
ES_Event_t RunTimerTestService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  static bool pinState = false; 
  
  if(ThisEvent.EventType == ES_TIMEOUT && ThisEvent.EventParam == TIMER_NUM)
  {
    pinState ^= 0x01; 
    // digitalWrite(TIMER_TEST_PIN, pinState); 
    // ES_Event_t NewEvent = {.EventType=ES_READ_SENSOR};
    // PostCO2Service(NewEvent); 
    ES_Event_t NewEvent = {.EventType=ES_READ_SENSOR}; 
    PostHPMService(NewEvent); 

    ES_TimerReturn_t returnVal = ES_Timer_InitTimer(TIMER_NUM, TIMER_LEN); 
    if(returnVal == ES_Timer_ERR)
    {
      Serial.println("Timer err at run\n");
    } 
  }
  else if(ThisEvent.EventType != ES_INIT)
  {
    Serial.printf("Unknown event: %d %d\n", ThisEvent.EventType, ThisEvent.EventParam);
  }

  return ReturnEvent;
}


/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

