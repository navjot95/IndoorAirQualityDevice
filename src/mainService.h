/****************************************************************************

  Header file for main state machine service

 ****************************************************************************/

#ifndef ServMain_H
#define ServMain_H

#include "ES_Event.h"
#include <stdbool.h>
#include <stdint.h>

// Public Function Prototypes
bool InitMainService(uint8_t Priority);
bool PostMainService(ES_Event_t ThisEvent);
ES_Event_t RunMainService(ES_Event_t ThisEvent);


#endif /* ServMain_H */

