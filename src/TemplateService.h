/****************************************************************************

  Header file for template service

 ****************************************************************************/

#ifndef ServTemplate_H
#define ServTemplate_H

#include "ES_Event.h"
#include <stdbool.h>
#include <stdint.h>

// Public Function Prototypes
bool InitTemplateService(uint8_t Priority);
bool PostTemplateService(ES_Event_t ThisEvent);
ES_Event_t RunTemplateService(ES_Event_t ThisEvent);

// Event checkers
bool EventCheckerTemplate(); 

#endif /* ServTemplate_H */

