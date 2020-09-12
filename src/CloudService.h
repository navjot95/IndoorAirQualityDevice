
#ifndef CloudServ_H
#define CloudServ_H

#include "ES_Event.h"
#include "ePaperDriver.h"
#include <stdbool.h>

// Public Function Prototypes
bool InitCloudService(uint8_t Priority);
bool PostCloudService(ES_Event_t ThisEvent);
ES_Event_t RunCloudService(ES_Event_t ThisEvent);

void updateCloudSensorVals(IAQsensorVals_t *sensorReads);

#endif /* CloudServ_H */

