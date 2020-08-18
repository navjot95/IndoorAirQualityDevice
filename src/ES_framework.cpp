/****************************************************************************
 Module
     EF_Framework.c
 Description
     source file for the core functions of the Events & Services framework. 
     The framework first runs through all the init functions of all services. 
     It then keep checking the eventChecker functions and the Queue to see 
     if there are any events. When there is an event, the appropriate run 
     function of a service is called. Currently, there is 1 queue so all events 
     are handeled in a LIFO manner without regard to priority of the service. 
 Notes

*****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "ES_Port.h"
#include "ES_framework.h"
#include "ES_Queue.h"
#include "ES_Timers.h"

#include <stdio.h>

/*----------------------------- Module Defines ----------------------------*/
typedef struct ES_service
{
  bool (*init_funct)(uint8_t);
  ES_Event_t (*run_funct)(ES_Event_t);
}ES_service_t; 

/*---------------------------- Module Functions ---------------------------*/
bool ES_ScanEventCheckers();

/*---------------------------- Module Variables ---------------------------*/
/****************************************************************************/
static bool (* const eventCheckerFuncts[])(void) = {EVENT_CHECKER_LIST};


// Main queue for all services
static ES_Event_t eventsList[QUEUE_SIZE];
static ES_Queue_t Queue; 


static ES_service_t const servicesList[NUM_SERVICES] = {
  {SERV_0_INIT, SERV_0_RUN}
#if NUM_SERVICES > 1
  ,{SERV_1_INIT, SERV_1_RUN}
#endif
#if NUM_SERVICES > 2
  ,{SERV_2_INIT, SERV_2_RUN}
#endif
#if NUM_SERVICES > 3
  ,{SERV_3_INIT, SERV_3_RUN}
#endif
#if NUM_SERVICES > 4
  ,{SERV_4_INIT, SERV_4_RUN}
#endif
#if NUM_SERVICES > 5
  ,{SERV_5_INIT, SERV_5_RUN}
#endif
#if NUM_SERVICES > 6
  ,{SERV_6_INIT, SERV_6_RUN}
#endif
#if NUM_SERVICES > 7
  ,{SERV_7_INIT, SERV_7_RUN}
#endif
#if NUM_SERVICES > 8
  ,{SERV_8_INIT, SERV_8_RUN}
#endif
#if NUM_SERVICES > 9
  ,{SERV_9_INIT, SERV_9_RUN}
#endif
#if NUM_SERVICES > 10
  ,{SERV_10_INIT, SERV_10_RUN}
#endif
#if NUM_SERVICES > 11
  ,{SERV_11_INIT, SERV_11_RUN}
#endif
#if NUM_SERVICES > 12
  ,{SERV_12_INIT, SERV_12_RUN}
#endif
#if NUM_SERVICES > 13
  ,{SERV_13_INIT, SERV_13_RUN}
#endif
#if NUM_SERVICES > 14
  ,{SERV_14_INIT, SERV_14_RUN}
#endif
#if NUM_SERVICES > 15
  ,{SERV_15_INIT, SERV_15_RUN}
#endif
};
/****************************************************************************/



/*------------------------------ Module Code ------------------------------*/
/****************************************************************************
 Function
   ES_Initialize
 Parameters
 Returns
   ES_Return_t : FailedPointer if any of the function pointers are NULL
                 FailedInit if any of the initialization functions failed
 Description
   Initialize all the services and tests for NULL pointers in the array
****************************************************************************/
ES_Return_t ES_Initialize(TimerRate_t Rate)
{
  if(QUEUE_SIZE < NUM_SERVICES){
    return FailedIndex;
  }

  ES_Timer_Init(Rate); 
  
  Queue.events_arr = eventsList; 
  Queue.capacity = QUEUE_SIZE; 
  ES_InitQueue(&Queue); 

  // first make sure all services have an init and run function that don't point to null
  for(uint8_t i=0; i<NUM_SERVICES; i++){
    if(!(servicesList[i].init_funct) || !(servicesList[i].run_funct)){
      return FailedPointer; 
    }

    bool returnVal = (servicesList[i].init_funct)(i);
    if(returnVal == false){
      return FailedInit; 
    }
  }

  return Success;
}

/****************************************************************************
 Function
   ES_Run
 Parameters
   None
 Returns
   ES_Return_t : FailedRun is any of the run functions failed during execution
 Description
   This is the main framework function. It calls the appropriate run function when 
   it detects the Queue isn't empty or one of the event checkers returned a true.
   Currently, there is 1 queue so all events are handeled in a LIFO manner without 
   regard to priority of the service.  
 Notes
   
****************************************************************************/

ES_Return_t ES_Run(void)
{
  static ES_Event_t ThisEvent = {.EventType=ES_NO_EVENT, .EventParam=0, .ServiceNum=0};
  static ES_Return_t returnEvent = Success; 

  _HW_Process_Pending_Ints();  // process framework hw timer

  // go through all event checkers until there's an event 
  if(ThisEvent.EventType != ES_ERROR && (ES_ScanEventCheckers() || !ES_isEmpty(&Queue)))
  {
    // if queue isn't empty, process all events there until it's empty 
    while(!ES_isEmpty(&Queue))
    {
      if(ES_DeQueue(&Queue, &ThisEvent) == false)
      {
        break;
      } 
      ThisEvent = servicesList[ThisEvent.ServiceNum].run_funct(ThisEvent);
      if(ThisEvent.EventType == ES_ERROR)
      {
        returnEvent = FailedRun;   
        return returnEvent; 
      }
    }
  }
  return returnEvent; 
}

/****************************************************************************
 Function
   ES_PostAll
 Parameters
   ES_Event : The Event to be posted
 Returns
   boolean : False if any of the post functions failed during execution
 Description
   posts to all of the services' queues
 Notes
****************************************************************************/
// TODO: IMPLEMENT THIS 
// bool ES_PostAll(ES_Event_t ThisEvent)  
// {
//   // add the new event to the queue
//   return ES_EnQueueEnd(&Queue, ThisEvent); 
// }

/****************************************************************************
 Function
   ES_PostToService
 Parameters
   uint8_t : Which service to post to (index into ServDescList)
   ES_Event : The Event to be posted
 Returns
   boolean : False if the post function failed during execution
 Description
   posts to one of the services' queues
 Notes
****************************************************************************/
bool ES_PostToService(ES_Event_t ThisEvent)
{
  return ES_EnQueueEnd(&Queue, ThisEvent);  
}


//*********************************
// private functions
//*********************************
/****************************************************************************
 Function
   ES_ScanEventCheckers
 Parameters
 Returns
   boolean : False if no events were registered 
 Description
   Run through all the event checker functions to look for any events. This function returns the first time
   an even checker returns true. The function then resumes again from the start of the list (rather than the next checker) 
 Notes
****************************************************************************/
bool ES_ScanEventCheckers(){
  for(size_t i=0; i < ARRAY_SIZE(eventCheckerFuncts); i++){
    if(eventCheckerFuncts[i]()){
      return 1; 
    }
  }
  return 0;   
}


// Same as event checker above but restarts from the next function rather than the start of the list
// bool ES_ScanEventCheckers2(){
//   static size_t i = 0; 

//   for(i; i < ARRAY_SIZE(eventCheckerFuncts); i++){
//     if(eventCheckerFuncts[i]()){
//       i++; 
//       if(i == ARRAY_SIZE(eventCheckerFuncts))
//         i = 0; 
//       return 1; 
//     }
//   }
//   i = 0; 
//   return 0;   
// }



/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/
