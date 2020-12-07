/****************************************************************************

  Header file for ePaper driver

 ****************************************************************************/

#ifndef EPaperDrv_H
#define EPaperDrv_H

#include "IAQ_util.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct IAQsensorVals
{
  int16_t eCO2; 
  int16_t tVOC; 
  int16_t PM25;
  int16_t PM10;
  int16_t CO2;
  int16_t temp;
  int16_t rh;
}IAQsensorVals_t;

typedef enum
{
  NO_SCREEN_REFRESH = 0, 
  PARTIAL_SCREEN_REFRESH, 
  FULL_SCREEN_REFRESH,
} IAQscreenRefresh_t; 

bool initePaper(); 
void updateScreenSensorVals(IAQsensorVals_t *newVals, bool forceFullRefresh, bool clearHdln);
void ePaperPrintfAlert(const char * title, const char * line1, const char * line2); 
void ePaperChangeHdln(const char *txt, IAQscreenRefresh_t scrnRefreshType, IAQmode_t newMode);
void ePaperChangeMode(IAQmode_t currMode);
time_t updateEpaperTime(const time_t timeToPrint);

#endif /* EPaperDrv_H */

