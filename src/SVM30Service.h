/****************************************************************************

  Header file for SVM30 sensor service

 ****************************************************************************/

#ifndef ServSVM30_H
#define ServSVM30_H

#include "ES_Event.h"
#include <stdbool.h>
#include <stdint.h>

// Public Function Prototypes
bool InitSVM30Service(uint8_t Priority);
bool PostSVM30Service(ES_Event_t ThisEvent);
ES_Event_t RunSVM30Service(ES_Event_t ThisEvent);

// Event checkers
bool EventChecker_SVM30(); 

#endif /* ServSVM30_H */
