/****************************************************************************
 Module
    ES_Queue.h
 Description
    header file for use with the top level functions of the ES Event Framework
 Notes
   
*****************************************************************************/
#ifndef ES_QUEUE_H
#define ES_QUEUE_H

#include <stdbool.h>

// main structure for holding all events 
typedef struct ES_Queue 
{ 
    uint8_t front_idx, num_events, capacity; 
    ES_Event_t* events_arr; 
}ES_Queue_t; 


void ES_InitQueue(ES_Queue_t *thisQueue); 
bool ES_EnQueueEnd(ES_Queue_t *thisQueue, ES_Event_t newEvent);
bool ES_EnQueueFront(ES_Queue_t *thisQueue, ES_Event_t newEvent); 
bool ES_DeQueue(ES_Queue_t *thisQueue, ES_Event_t *topEvent);
bool ES_isEmpty(ES_Queue_t *thisQueue); 

#endif
