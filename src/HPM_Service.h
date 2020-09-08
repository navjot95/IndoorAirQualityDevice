/****************************************************************************

  Header file for Honeywell Particulate Matter (HPM) sensor service

 ****************************************************************************/

#ifndef ServHPM_H
#define ServHPM_H

#include "ES_Event.h"
#include "IAQ_util.h"
#include <stdbool.h>
#include <stdint.h>

// Public Function Prototypes
bool InitHPMService(uint8_t Priority);
bool PostHPMService(ES_Event_t ThisEvent);
ES_Event_t RunHPMService(ES_Event_t ThisEvent);

void HPMsetMode(IAQmode_t newMode);
void getPMAvg(int16_t *pm10Avg, int16_t *pm25Avg);
void stopHPMMeasurements(); 

// Event checkers
bool EventCheckerHPM(); 

#endif /* ServHPM_H */

