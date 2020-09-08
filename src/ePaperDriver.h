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

bool initePaper(); 
void updateScreenSensorVals(IAQsensorVals_t *newVals, bool forceFullRefresh);
void ePaperPrintfAlert(const char * title, const char * line1, const char * line2); 
void ePaperChangeHdln(const char *txt, bool updateScrn);
void ePaperChangeMode(IAQmode_t currMode);
bool updateEpaperTime();  // TODO: Don't think this needs to be a public function

#endif /* EPaperDrv_H */

