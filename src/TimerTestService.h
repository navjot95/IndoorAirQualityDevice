/****************************************************************************

  Header file for timer test service

 ****************************************************************************/

#ifndef ServTimerTest_H
#define ServTimerTest_H

#include "ES_Event.h"
#include <stdbool.h>
#include <stdint.h>

// Public Function Prototypes
bool InitTimerTestService(uint8_t Priority);
bool PostTimerTestService(ES_Event_t ThisEvent);
ES_Event_t RunTimerTestService(ES_Event_t ThisEvent);

#endif /* ServTimerTest_H */

