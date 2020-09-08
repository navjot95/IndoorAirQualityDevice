/****************************************************************************
 Module
   ButtonService.c

 Description
   This is a simple service for debouncing the button. It helps to detect 
   long presses and short presses. 

 Notes
****************************************************************************/
/*----------------------------- Include Files -----------------------------*/
#include "ButtonService.h"
#include "ES_framework.h"

/*----------------------------- Module Defines ----------------------------*/
#define BT_PRESS 1
#define BT_RELEASE 0 
#define DB_TIMER_LEN 40  // length of debounce timer in ms
#define LG_BUTTON_LEN 1500  // time length to hold down button to trigger as long press
#define TIMEOUT_LEN 10000  

typedef enum{
  START_SM_STATE,
  WAIT_SM_STATE
} statusState_t; 



/*---------------------------- Module Functions ---------------------------*/


/*---------------------------- Module Variables ---------------------------*/
static uint8_t MyPriority;
static statusState_t currStatusState = START_SM_STATE;

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
bool InitButtonService(uint8_t Priority)
{
  MyPriority = Priority;

  return true; 
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
bool PostButtonService(ES_Event_t ThisEvent)
{
  ThisEvent.ServiceNum = MyPriority; 
  return ES_PostToService(ThisEvent);
}

/****************************************************************************
 Function
    RunButtonService

 Parameters
   ES_Event_t : the event to process

 Returns
   ES_Event, ES_NO_EVENT if no error ES_ERROR otherwise

 Description
   small state machine that helps for debouncing the button 
 Notes
****************************************************************************/
ES_Event_t RunButtonService(ES_Event_t ThisEvent)
{
  ES_Event_t ReturnEvent;
  ReturnEvent.EventType = ES_NO_EVENT; // assume no errors
  static unsigned long timeStamp = millis(); 

  switch (currStatusState)
  {
    case START_SM_STATE:
    {
      unsigned long elapsed_time = millis() - timeStamp; 
      if((elapsed_time > DB_TIMER_LEN) && ThisEvent.EventType == ES_HW_BUTTON_EVENT && ThisEvent.EventParam == BT_PRESS)
      {
        timeStamp = millis(); 
        currStatusState = WAIT_SM_STATE; 
      }
      break;
    }
    case WAIT_SM_STATE:
    {
      unsigned long elapsed_time = millis() - timeStamp; 
      if((elapsed_time > DB_TIMER_LEN) && (elapsed_time < LG_BUTTON_LEN) && ThisEvent.EventType == ES_HW_BUTTON_EVENT && ThisEvent.EventParam == BT_RELEASE)
      {
        ES_Event_t newEvent; 
        newEvent.EventType = ES_SW_BUTTON_PRESS; 
        newEvent.EventParam = SHORT_BT_PRESS;
        // printf("%lu\n", elapsed_time); 
        PostMainService(newEvent); 

        currStatusState = START_SM_STATE; 
        timeStamp = millis(); 
      }  
      else if((elapsed_time >= LG_BUTTON_LEN) && (elapsed_time < TIMEOUT_LEN) && ThisEvent.EventType == ES_HW_BUTTON_EVENT && ThisEvent.EventParam == BT_RELEASE)
      {
        ES_Event_t newEvent; 
        newEvent.EventType = ES_SW_BUTTON_PRESS; 
        newEvent.EventParam = LONG_BT_PRESS;
        PostMainService(newEvent); 
        // Sprintf("%lu\n", elapsed_time); 

        currStatusState = START_SM_STATE; 
        timeStamp = millis(); 
      }  
      else if(elapsed_time > TIMEOUT_LEN) // in case we ever get stuck here, user will just have to press again 
      {
        currStatusState = START_SM_STATE; 
        timeStamp = millis(); 
      }
      
      break;
    }
  }

  return ReturnEvent;
}


bool EventCheckerButton(){
  static uint8_t lastVal = 0; 
  uint8_t currVal = digitalRead(BUTTON_PIN);
  
  if(currVal != lastVal)
  {
    ES_Event_t newEvent;
    newEvent.EventType = ES_HW_BUTTON_EVENT; 

    if(currVal > lastVal) 
      newEvent.EventParam = BT_PRESS;  // button pressed 
    else if(currVal < lastVal)
      newEvent.EventParam = BT_RELEASE;  // button released 

    lastVal = currVal;
    PostButtonService(newEvent); 
    return true;  
  }

  return false; 
}

/***************************************************************************
 private functions
 ***************************************************************************/

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/

