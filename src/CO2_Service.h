/****************************************************************************

  Header file for template service

 ****************************************************************************/

#ifndef ServCO2_H
#define ServCO2_H

#include "ES_Event.h"
#include "IAQ_util.h"
#include <stdbool.h>
#include <stdint.h>


// Public Function Prototypes
bool InitCO2Service(uint8_t Priority);
bool PostCO2Service(ES_Event_t ThisEvent);
ES_Event_t RunCO2Service(ES_Event_t ThisEvent);

// Event checkers
bool EventCheckerCO2(); 
void setModeCO2(IAQmode_t newMode);
void getCO2Avg(int16_t *CO2Avg);

#endif /* ServCO2_H */

