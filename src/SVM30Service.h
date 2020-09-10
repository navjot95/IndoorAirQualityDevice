/****************************************************************************

  Header file for SVM30 sensor service

 ****************************************************************************/

#ifndef ServSVM30_H
#define ServSVM30_H

#include "ES_Event.h"
#include "IAQ_util.h"
#include <stdbool.h>
#include <stdint.h>

// Public Function Prototypes
bool InitSVM30Service(uint8_t Priority);
bool PostSVM30Service(ES_Event_t ThisEvent);
ES_Event_t RunSVM30Service(ES_Event_t ThisEvent);

bool EventChecker_SVM30();  // Event checker
void setModeSVM30(IAQmode_t newMode);
void getSVM30Avg(int16_t *eCO2Avg, int16_t *tVOCAvg, int16_t *tpAvg, int16_t *rhAvg);


#endif /* ServSVM30_H */
