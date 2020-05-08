/****************************************************************************
 Module
    ES_Event.h
 Description
    header file for use with the top level functions of the ES Event Framework
 Notes
    
*****************************************************************************/
#ifndef ES_Event_H
#define ES_Event_H

#include "ES_Configure.h"
#include <stdint.h>


//includes event type and event param
typedef struct ES_Event
{
  ES_EventType_t EventType;      // what kind of event?
  uint16_t EventParam;          // parameter value for use w/ this event
  uint8_t ServiceNum;           // Just call the right post function and the framework handels this 
}ES_Event_t;


#endif