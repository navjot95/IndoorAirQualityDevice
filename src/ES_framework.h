
/**************************************************************************** 
 Module
    ES_Framework.h
 Description
    header file for use with the top level functions of the EF Event Framework
 Notes
    This framework is influenced by the ME218 Events&Services Framework written Ed Carryer
*****************************************************************************/

#ifndef ES_Framework_H
#define ES_Framework_H


#include "ES_Event.h"
#include "ES_ServicesHeaders.h"
#include <Arduino.h>
#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  Success     = 0,
  FailedPost  = 1,
  FailedRun,
  FailedPointer,
  FailedIndex,
  FailedInit
}ES_Return_t;


ES_Return_t ES_Initialize();
ES_Return_t ES_Run(void);
bool ES_PostToService(ES_Event_t ThisEvent);

// bool ES_PostAll(ES_Event_t ThisEvent);
// bool ES_PostToServiceLIFO(uint8_t WhichService, ES_Event_t TheEvent);

#endif   // ES_Framework_H
