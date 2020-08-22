/****************************************************************************

  Header file for template service

 ****************************************************************************/

#ifndef EPaperServ_H
#define EPaperServ_H

#include "ES_Event.h"
#include <stdbool.h>
#include <stdint.h>

// Public Function Prototypes
bool InitePaperService(uint8_t Priority);
bool PostePaperService(ES_Event_t ThisEvent);
ES_Event_t RunePaperService(ES_Event_t ThisEvent);

#endif /* EPaperServ_H */

