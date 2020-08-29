/****************************************************************************

  Header file for template service

 ****************************************************************************/

#ifndef EPaperServ_H
#define EPaperServ_H

#include "ES_Event.h"
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


// Public Function Prototypes
bool InitePaperService(uint8_t Priority);
bool PostePaperService(ES_Event_t ThisEvent);
ES_Event_t RunePaperService(ES_Event_t ThisEvent);

void updateScreenSensorVals(IAQsensorVals_t newVals);
void ePaperPrintfAlert(const char * title, const char * line1, const char * line2); 
void ePaperChangeHdln(const char *txt, bool updateScrn);

#endif /* EPaperServ_H */

