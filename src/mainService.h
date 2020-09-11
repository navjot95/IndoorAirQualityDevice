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

void updateCO2Val(int16_t CO2_newVal);
void updateHPMVal(int16_t PM25_newVal, int16_t PM10_newVal);
void updateSVM30Vals(int16_t eCO2_newVal, int16_t tVOC_newVal, int16_t tm_newVal, int16_t rh_newVal);

// Event checkers
bool EventCheckerBat(); 

#endif /* ServMain_H */

