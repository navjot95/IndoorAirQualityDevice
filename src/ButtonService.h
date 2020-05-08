/****************************************************************************

  Header file for template service

 ****************************************************************************/

#ifndef ServButton_H
#define ServButton_H

#include "ES_Event.h"
#include <stdbool.h>
#include <stdint.h>

// Public Function Prototypes
bool InitButtonService(uint8_t Priority);
bool PostButtonService(ES_Event_t ThisEvent);
ES_Event_t RunButtonService(ES_Event_t ThisEvent);

// Event checkers
bool EventCheckerButton(); 

#endif /* ServButton_H */

