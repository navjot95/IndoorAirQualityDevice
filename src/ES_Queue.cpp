/****************************************************************************
 Module
     ES_Framework.c
 Description
     source file for the event queueing of the Events & Services framework
 Notes

*****************************************************************************/
//Potential improvement: convert array of events to array of pointers that point events


/*----------------------------- Include Files -----------------------------*/
#include "ES_framework.h"
#include "ES_Queue.h"
#include <stdio.h>


/*----------------------------- Module Defines ----------------------------*/



/*---------------------------- Module Functions ---------------------------*/
static bool ES_isFull(ES_Queue_t *thisQueue); 


/*---------------------------- Module Variables ---------------------------*/



/*------------------------------ Module Code ------------------------------*/
void ES_InitQueue(ES_Queue_t *thisQueue){
    thisQueue->front_idx = 0; 
    thisQueue->num_events = 0; 
}


bool ES_EnQueueEnd(ES_Queue_t *thisQueue, ES_Event_t newEvent){
    if(ES_isFull(thisQueue)){
        return false; 
    }
    uint8_t nextIdx = (thisQueue->front_idx + thisQueue->num_events) % thisQueue->capacity; 
    thisQueue->events_arr[nextIdx] = newEvent; 
    thisQueue->num_events++; 
    return true; 
}


bool ES_EnQueueFront(ES_Queue_t *thisQueue, ES_Event_t newEvent){
    if(ES_isFull(thisQueue)){
        return false; 
    }

    uint8_t nextIdx; 
    if(thisQueue->front_idx != 0){
        nextIdx = thisQueue->front_idx - 1;
    }
    else{
        nextIdx = thisQueue->capacity - 1; 
    }
    thisQueue->events_arr[nextIdx] = newEvent; 
    thisQueue->num_events++; 
    thisQueue->front_idx = nextIdx; 

    return true; 
}


bool ES_DeQueue(ES_Queue_t *thisQueue, ES_Event_t *topEvent){
    if(ES_isEmpty(thisQueue)){
        return false; 
    }

    *topEvent = thisQueue->events_arr[thisQueue->front_idx];
    thisQueue->front_idx = (thisQueue->front_idx + 1) % thisQueue->capacity; 
    thisQueue->num_events--; 
    return true;
}


bool ES_isEmpty(ES_Queue_t *thisQueue){
    return thisQueue->num_events == 0; 
}



//***************************************************************************
// private functions
//***************************************************************************
static bool ES_isFull(ES_Queue_t *thisQueue){
    return thisQueue->num_events >= thisQueue->capacity; 
}

/*------------------------------- Footnotes -------------------------------*/
/*------------------------------ End of file ------------------------------*/


