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

/*----------------------------- Module Defines ----------------------------*/

typedef enum{
  INIT_STATE,
  WAIT_SM_STATE
} statusState_t; 



/*---------------------------- Module Functions ---------------------------*/


/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
static statusState_t currStatusState = INIT_STATE;

/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
     InitButtonService

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
  ES_Event_t ThisEvent;

  MyPriority = Priority;
  
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
     PostButtonService

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
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors

  switch (currStatusState)
  {
    case INIT_STATE:
    {
      

      break;
    }
    case WAIT_SM_STATE:
    {
      
      
      break;
    }
  }

  return ReturnEvent;
}


bool EventCheckerCO2(){
  

  return false; 
}

/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

